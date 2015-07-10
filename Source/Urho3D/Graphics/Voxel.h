#pragma once

#include "../Math/Vector3.h"
#include "../Core/Object.h"
#include <STB/stb_voxel_render.h>
#include "../Math/Color.h"
#include "Texture2DArray.h"
#include "../Container/ArrayPtr.h"

namespace Urho3D {

class VoxelChunk;
class VoxelMap;

struct VoxelProcessorWriters;
typedef void(*VoxelProcessorFunc)(VoxelChunk* chunk, VoxelMap* source, VoxelProcessorWriters writers);

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

static const unsigned VOXEL_BLOCK_BLOCKTYPE = 0x00000001;
static const unsigned VOXEL_BLOCK_COLOR     = 0x00000002;
static const unsigned VOXEL_BLOCK_GEOMETRY  = 0x00000004;
static const unsigned VOXEL_BLOCK_VHEIGHT   = 0x00000008;
static const unsigned VOXEL_BLOCK_LIGHTING  = 0x00000010;
static const unsigned VOXEL_BLOCK_ROTATE    = 0x00000020;
static const unsigned VOXEL_BLOCK_TEX2      = 0x00000040;

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
	PODVector<unsigned char> blockSelector;
	PODVector<unsigned char[6]> blockTex1Face;
	PODVector<unsigned char[6]> blockTex2Face;
	SharedPtr<Texture2DArray> diffuse1Textures;
	SharedPtr<Texture2DArray> diffuse2Textures;

	VoxelBlocktypeMap(Context* context) : Object(context) { }
	~VoxelBlocktypeMap() { }
};

class URHO3D_API VoxelWriter
{
public:
    unsigned size;
	unsigned xStride;
	unsigned zStride;
    unsigned char* buffer;

    VoxelWriter() :
        size(0),
        xStride(0),
        zStride(0)
    {

    }
    
	inline unsigned GetIndex(int x, int y, int z) { return (y + 2) + ((z + 2) * zStride) + ((x + 2) * xStride); }

	void SetSize(unsigned width, unsigned height, unsigned depth)
	{
		zStride = height + 4;
		xStride = (height + 4) * (depth + 4);
        size = (width + 4)*(height + 4)*(depth + 4);
	}

    void InitializeBuffer(unsigned char* data)
    {
        buffer = data;
    }

    void Clear(unsigned char value)
    {
		memset(buffer, value, sizeof(char) * size);
    }

    inline void Set(int x, int y, int z, unsigned char val)
    {
		buffer[GetIndex(x, y, z)] = val;
    }

	inline void SetColor(int x, int y, int z, unsigned char val)
	{
		buffer[GetIndex(x, y, z)] = val;
	}

	inline void SetBlocktype(int x, int y, int z, unsigned char val)
	{
		buffer[GetIndex(x, y, z)] = val;
	}

	inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
	{
		buffer[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
	}

	inline void SetLighting(int x, int y, int z, unsigned char val)
	{
		buffer[GetIndex(x, y, z)] = val;
	}

	inline void SetTex2(int x, int y, int z, unsigned char val)
	{
		buffer[GetIndex(x, y, z)] = val;
	}

    inline void SetGeometry(int x, int y, int z, VoxelGeometryType geometrytype)
    {
        buffer[GetIndex(x, y, z)] = VoxelEncodeGeometry(geometrytype);
    }
};

class URHO3D_API VoxelMap : public Resource {
	OBJECT(VoxelMap);

public:
	SharedPtr<VoxelBlocktypeMap> blocktypeMap;
	PODVector<unsigned char> blocktype;
	PODVector<unsigned char> color;
	PODVector<unsigned char> geometry;
	PODVector<unsigned char> vHeight;
	PODVector<unsigned char> lighting;
	PODVector<unsigned char> rotate;
	PODVector<unsigned char> tex2;
    unsigned dataMask_;
    unsigned processorDataMask_;
	unsigned height_;
	unsigned width_;
	unsigned depth_;
	unsigned size_;
	unsigned xStride;
	unsigned zStride;
    Vector<VoxelProcessorFunc> voxelProcessors_;
    SharedPtr<Object> source_;

    /// Construct empty.
    VoxelMap(Context* context);

    /// Destruct.
    virtual ~VoxelMap();

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);

    /// Unloads block information from memory.
    virtual void Unload();

    /// Reloads the voxel map if source is available.
    virtual bool Reload();

    /// See if voxel map is loaded into memory.
    virtual bool IsLoaded();

    /// Sets the deserializer to load the data when chunk is built.
    virtual void SetSource(Object* deserializer);

    /// Sets the block type data mask.
    virtual void SetDataMask(unsigned dataMask) { dataMask_ = dataMask; }

    /// Sets the block type data mask.
    virtual void SetProcessorDataMask(unsigned processorDataMask) { processorDataMask_ = processorDataMask; }

	inline unsigned GetIndex(int x, int y, int z) { return (y + 2) + ((z + 2) * zStride) + ((x + 2) * xStride); }
    void SetSize(unsigned width, unsigned height, unsigned depth);
    void InitializeBlocktype(unsigned char initialValue = 0);
	void InitializeVHeight(unsigned char initialValue = 0);
	void InitializeLighting(unsigned char initialValue = 0);
	void InitializeColor(unsigned char initialValue = 0);
	void InitializeTex2(unsigned char initialValue = 0);
	void InitializeGeometry(unsigned char initialValue = 0);

    Vector<VoxelProcessorFunc> GetVoxelProcessors() { return voxelProcessors_; }

    void AddVoxelProcessor(VoxelProcessorFunc voxelProcessor) { voxelProcessors_.Push(voxelProcessor); }

    void RemoveVoxelProcessor(unsigned index) { voxelProcessors_.Erase(index); }

	inline void SetColor(int x, int y, int z, unsigned char val)
	{
		color[GetIndex(x, y, z)] = val;
	}

	inline void SetBlocktype(int x, int y, int z, unsigned char val)
	{
		blocktype[GetIndex(x, y, z)] = val;
	}

	inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
	{
		vHeight[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
	}

	inline void SetLighting(int x, int y, int z, unsigned char val)
	{
		lighting[GetIndex(x, y, z)] = val;
	}

	inline void SetTex2(int x, int y, int z, unsigned char val)
	{
		tex2[GetIndex(x, y, z)] = val;
	}

    inline void SetGeometry(int x, int y, int z, VoxelGeometryType geometrytype)
    {
        geometry[GetIndex(x, y, z)] = VoxelEncodeGeometry(geometrytype);
    }

};

struct VoxelProcessorWriters
{
    VoxelWriter blocktype;
	VoxelWriter color;
	VoxelWriter geometry;
	VoxelWriter vHeight;
	VoxelWriter lighting;
	VoxelWriter rotate;
	VoxelWriter tex2;
};

class Voxel
{
private:
	unsigned index_;
	VoxelMap* map_;

public:
	
	Voxel(){

	}
	Voxel(VoxelMap* map, int x, int y, int z)
	{
		map_ = map;
		index_ = map->GetIndex(x, y, z);
	}
	inline void SetBlocktype(int x, int y, int z, unsigned char val)
	{
		map_->blocktype[index_] = val;
	}
	inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
	{
		map_->vHeight[index_] = VoxelEncodeVHeight(sw, se, nw, ne);
	}
};

}
