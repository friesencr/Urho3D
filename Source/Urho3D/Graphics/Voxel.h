#pragma once

#include "../Math/Vector3.h"
#include "../Core/Object.h"
#include <STB/stb_voxel_render.h>
#include "../Math/Color.h"
#include "Texture2DArray.h"

namespace Urho3D {

// Shapes of blocks that aren't always cubes
enum VoxelGeometryType
{
	VOXEL_TYPE_EMPTY,
	VOXEL_TYPE_KNOCKOUT,  // CREATES A HOLE IN THE MESH
	VOXEL_TYPE_SOLID,
	VOXEL_TYPE_TRANSP,    // SOLID GEOMETRY, BUT TRANSPARENT CONTENTS SO NEIGHBORS GENERATE NORMALLY, UNLESS SAME BLOCKTYPE

	// FO_TYPELLOWING 4 CAN BE REPRESENTED BY VHEIGHT AS WELL
	VOXEL_TYPE_SLAB_UPPER,
	VOXEL_TYPE_SLAB_LOWER,
	VOXEL_TYPE_FLOOR_SLOPE_NORTH_IS_TOP,
	VOXEL_TYPE_CEIL_SLOPE_NORTH_IS_BOTTOM,

	VOXEL_TYPE_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED,   // SAME AS FLOOR_SLOPE ABOVE, BUT USES WALL'S TEXTURE & TEXTURE PROJECTION
	VOXEL_TYPE_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED,
	VOXEL_TYPE_CROSSED_PAIR,    // CORNER-TO-CORNER PAIRS, WITH NORMAL VECTOR BUMPED UPWARDS
	VOXEL_TYPE_FORCE,           // LIKE GEOM_TRANSP, BUT FACES VISIBLE EVEN IF NEIGHBOR IS SAME TYPE, E.G. MINECRAFT FANCY LEAVES

	// TH_TYPEESE ACCESS VHEIGHT INPUT
	VOXEL_TYPE_FLOOR_VHEIGHT_03 = 12,  // DIAGONAL IS SW-NE
	VOXEL_TYPE_FLOOR_VHEIGHT_12,       // DIAGONAL IS SE-NW
	VOXEL_TYPE_CEIL_VHEIGHT_03,
	VOXEL_TYPE_CEIL_VHEIGHT_12,

	VOXEL_TYPE_COUNT, // NUMBER OF GEOM CASES
};

enum VoxelRotation
{
	VOXEL_FACE_EAST,
	VOXEL_FACE_NORTH,
	VOXEL_FACE_WEST,
	VOXEL_FACE_SOUTH,
	VOXEL_FACE_UP,
	VOXEL_FACE_DOWN,
	VOXEL_FACE_COUNT,
};

enum VoxelHeight
{
	VOXEL_HEIGHT_0,
	VOXEL_HEIGHT_HALF,
	VOXEL_HEIGHT_1,
	VOXEL_HEIGHT_ONE_AND_HALF,
};

unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled);

unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast);

unsigned char VoxelEncodeGeometry(VoxelGeometryType type, VoxelRotation rot, VoxelHeight height);

unsigned char VoxelEncodeGeometry(VoxelGeometryType type, VoxelRotation rot);

unsigned char VoxelEncodeGeometry(VoxelGeometryType type);

class URHO3D_API VoxelBlocktypeMap : public Object {
	OBJECT(VoxelBlocktypeMap);
	
public:
	PODVector<unsigned char> blockColor;
	PODVector<unsigned char> blockGeometry;
	PODVector<unsigned char> blockVHeight;
	PODVector<unsigned char> blockTex1;
	PODVector<unsigned char> blockTex2;
	PODVector<unsigned char[6]> blockTex1Face;
	PODVector<unsigned char[6]> blockTex2Face;
	SharedPtr<Texture2DArray> diffuse1Textures;
	SharedPtr<Texture2DArray> diffuse2Textures;

	VoxelBlocktypeMap(Context* context) : Object(context) { }	~VoxelBlocktypeMap() { }
};

class URHO3D_API VoxelMap : public Object {
	OBJECT(VoxelMap);

public:
	SharedPtr<VoxelBlocktypeMap> blocktypeMap;
	//SharedPtr<Texture2DArray> diffuse1Textures;
	//SharedPtr<Texture2DArray> diffuse2Textures;
	PODVector<unsigned char> blocktype;
	PODVector<unsigned char> color;
	PODVector<unsigned char> geometry;
	PODVector<unsigned char> vHeight;
	unsigned height_;
	unsigned width_;
	unsigned depth_;
	unsigned size_;
	unsigned xStride;
	unsigned zStride;
	float ambientLightFactor;

	VoxelMap(Context* context) : Object(context) 
	{
		ambientLightFactor = 0.5;
	}

	~VoxelMap() { }
	

	inline unsigned char GetBlocktype(int x, int y, int z)
	{
		return blocktype[GetIndex(x, y, z)];
	}

	inline unsigned char GetVheight(int x, int y, int z)
	{
		return vHeight[GetIndex(x, y, z)];
	}

	inline unsigned GetIndex(int x, int y, int z)
	{
		return (y + 1) + ((z + 1) * zStride) + ((x + 1) * xStride);
	}

	void SetSize(unsigned width, unsigned height, unsigned depth)
	{
		height_ = height;
		width_ = width;
		depth_ = depth;
		zStride = height + 2;
		xStride = (height + 2) * (depth + 2);
		size_ = (width_ + 2)*(height_ + 2)*(depth_ + 2);
	}

	void InitializeBlocktype()
	{
		blocktype.Resize(size_);
		memset(&blocktype.Front(), 0, sizeof(char) * size_);
	}

	void InitializeVHeight()
	{
		vHeight.Resize(size_);
		memset(&vHeight.Front(), 0, sizeof(char) * size_);
	}

	inline void SetBlocktype(int x, int y, int z, unsigned char val)
	{
		blocktype[GetIndex(x, y, z)] = val;
	}

	inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
	{
		vHeight[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
	}

};

}
