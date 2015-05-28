#pragma once

#include "../Math/Vector3.h"
#include "../Core/Object.h"
#include <STB/stb_voxel_render.h>

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

class URHO3D_API VoxelDefinition : public Object {
	OBJECT(VoxelDefinition);

public:
	VoxelDefinition(Context* context) : Object(context) { }
	~VoxelDefinition(){}
	PODVector<unsigned char> blocktype;
	PODVector<unsigned char> blockGeometry;
	PODVector<unsigned char> blockVHeight;
	PODVector<unsigned char> blockTex1;
	PODVector<unsigned char> blockTex2;
	PODVector<unsigned char> color;
	PODVector<unsigned char[6]> blockTex1Face;
	PODVector<unsigned char[6]> blockTex2Face;
	PODVector<unsigned char> geometry;
	unsigned height_;
	unsigned width_;
	unsigned depth_;
	unsigned xStride;
	unsigned zStride;
	

	static unsigned char EncodeBlockTypeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast)
	{
		return STBVOX_MAKE_VHEIGHT(southWest, southEast, northWest, northEast);
	}
	static unsigned char EncodeGeometry(VoxelGeometryType type, VoxelRotation rot, VoxelHeight height)
	{
		return STBVOX_MAKE_GEOMETRY(type, rot, height);
	}
	static unsigned char EncodeGeometry(VoxelGeometryType type, VoxelRotation rot)
	{
		return STBVOX_MAKE_GEOMETRY(type, rot, 0);
	}
	static unsigned char EncodeGeometry(VoxelGeometryType type)
	{
		return STBVOX_MAKE_GEOMETRY(type, VOXEL_FACE_EAST,0);
	}

	unsigned char GetBlocktype(unsigned x, unsigned y, unsigned z)
	{
		return GetAddress(x, y, z)[0];
	}

	unsigned char* GetAddress(unsigned x, unsigned y, unsigned z)
	{
		return &blocktype[(y + 1) + ((z + 1) * zStride) + ((x + 1) * xStride)];
	}

	void SetSize(unsigned width, unsigned height, unsigned depth)
	{
		height_ = height;
		width_ = width;
		depth_ = depth;
		zStride = height + 2;
		xStride = (height + 2) * (depth + 2);
		unsigned blocktypeSize = (width + 2)*(height + 2)*(depth * 2);

		blocktype.Resize(blocktypeSize);
		for (unsigned i = 0; i < blocktype.Size(); ++i)
			blocktype[i] = 0;
	}

	void SetBlocktype(unsigned x, unsigned y, unsigned z, unsigned char val)
	{
		GetAddress(x, y, z)[0] = val;
	}

};

}
