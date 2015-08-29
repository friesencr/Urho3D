#pragma once

#include "../Math/Vector3.h"
#include "../Core/Object.h"
#include <STB/stb_voxel_render.h>
#include "../Math/Color.h"
#include "Texture2DArray.h"
#include "../Container/ArrayPtr.h"
#include "../Scene/Serializable.h"
#include "../IO/Generator.h"
#include "../IO/VectorBuffer.h"

namespace Urho3D {

static const unsigned VOXEL_BLOCK_BLOCKTYPE      = 0x00000001;
static const unsigned VOXEL_BLOCK_COLOR2         = 0x00000002;
static const unsigned VOXEL_BLOCK_COLOR2FACEMASK = 0x00000004;
static const unsigned VOXEL_BLOCK_COLOR3         = 0x00000008;
static const unsigned VOXEL_BLOCK_COLOR3FACEMASK = 0x00000010;
static const unsigned VOXEL_BLOCK_COLOR          = 0x00000020;
static const unsigned VOXEL_BLOCK_ECOLOR         = 0x00000040;
static const unsigned VOXEL_BLOCK_ECOLORFACEMASK = 0x00000080;
static const unsigned VOXEL_BLOCK_EXTENDEDCOLOR  = 0x00000100;
static const unsigned VOXEL_BLOCK_GEOMETRY       = 0x00000200;
static const unsigned VOXEL_BLOCK_LIGHTING       = 0x00000400;
static const unsigned VOXEL_BLOCK_OVERLAY        = 0x00000800;
static const unsigned VOXEL_BLOCK_ROTATE         = 0x00001000;
static const unsigned VOXEL_BLOCK_TEX2           = 0x00002000;
static const unsigned VOXEL_BLOCK_TEX2FACEMASK   = 0x00004000;
static const unsigned VOXEL_BLOCK_TEX2REPLACE    = 0x00008000;
static const unsigned VOXEL_BLOCK_VHEIGHT        = 0x00010000;

class VoxelChunk;
class VoxelMap;

struct VoxelRGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// Shapes of blocks that aren't always cubes
enum VoxelGeometry
{
    VOXEL_GEOMETRY_EMPTY,
    VOXEL_GEOMETRY_KNOCKOUT,  // CREATES A HOLE IN THE MESH
    VOXEL_GEOMETRY_SOLID,
    VOXEL_GEOMETRY_TRANSP,    // SOLID GEOMETRY, BUT TRANSPARENT CONTENTS SO NEIGHBORS GENERATE NORMALLY, UNLESS SAME BLOCKGEOMETRY

    // FO_GEOMETRYLLOWING 4 CAN BE REPRESENTED BY VHEIGHT AS WELL
    VOXEL_GEOMETRY_SLAB_UPPER,
    VOXEL_GEOMETRY_SLAB_LOWER,
    VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP,
    VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM,

    VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED,   // SAME AS FLOOR_SLOPE ABOVE, BUT USES WALL'S TEXTURE & TEXTURE PROJECTION
    VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED,
    VOXEL_GEOMETRY_CROSSED_PAIR,    // CORNER-TO-CORNER PAIRS, WITH NORMAL VECTOR BUMPED UPWARDS
    VOXEL_GEOMETRY_FORCE,           // LIKE GEOM_TRANSP, BUT FACES VISIBLE EVEN IF NEIGHBOR IS SAME GEOMETRY, E.G. MINECRAFT FANCY LEAVES

    // TH_GEOMETRYESE ACCESS VHEIGHT INPUT
    VOXEL_GEOMETRY_FLOOR_VHEIGHT_03 = 12,  // DIAGONAL IS SW-NE
    VOXEL_GEOMETRY_FLOOR_VHEIGHT_12,       // DIAGONAL IS SE-NW
    VOXEL_GEOMETRY_CEIL_VHEIGHT_03,
    VOXEL_GEOMETRY_CEIL_VHEIGHT_12,

    VOXEL_GEOMETRY_COUNT, // NUMBER OF GEOM CASES
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

enum VoxelFaceRotation
{
    VOXEL_FACEROT_0,
    VOXEL_FACEROT_90,
    VOXEL_FACEROT_180,
    VOXEL_FACEROT_270
};

unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled);
unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast);
unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot, VoxelHeight height);

class URHO3D_API VoxelWriter
{
public:
    unsigned size;
    unsigned strideX;
    unsigned strideZ;
    unsigned char* buffer;

    VoxelWriter() :
        size(0),
        strideX(0),
        strideZ(0)
    {

    }
    
    inline unsigned GetIndex(int x, int y, int z) { return (y + 2) + ((z + 2) * strideZ) + ((x + 2) * strideX); }

    void SetSize(unsigned width, unsigned height, unsigned depth)
    {
        strideZ = height + 4;
        strideX = (height + 4) * (depth + 4);
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

    inline void SetGeometry(int x, int y, int z, VoxelGeometry geometry, VoxelRotation rotation, VoxelHeight height)
    {
        buffer[GetIndex(x, y, z)] = VoxelEncodeGeometry(geometry, rotation, height);
    }
};

struct VoxelProcessorWriters
{
	VoxelWriter blocktype;
	VoxelWriter color2;
	VoxelWriter color2Facemask;
	VoxelWriter color3;
	VoxelWriter color3Facemask;
	VoxelWriter color;
	VoxelWriter eColor;
	VoxelWriter eColorFaceMask;
	VoxelWriter extendedColor;
	VoxelWriter geometry;
	VoxelWriter lighting;
	VoxelWriter overlay;
	VoxelWriter rotate;
	VoxelWriter tex2;
	VoxelWriter tex2Facemask;
	VoxelWriter tex2Replace;
	VoxelWriter vHeight;
};

struct VoxelRange
{
    int startX;
    int startY;
    int startZ;
    int endX;
    int endY;
    int endZ;
};

struct VoxelRangeFragment : VoxelRange
{
    int indexX;
    int indexY;
    int indexZ;
    int lenX;
    int lenY;
    int lenZ;
};

typedef void(*VoxelProcessorFunc)(VoxelChunk* voxelChunk, VoxelMap* source, const VoxelRangeFragment& range, VoxelProcessorWriters writers);

//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot);
//
//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry);

//unsigned char VoxelEncodeFaceMask(bool east, bool north, bool west, bool south, bool up, bool down);
//
//unsigned char VoxelEncodeSideTextureSwap(unsigned char block, unsigned char overlay, unsigned char ecolor);

class URHO3D_API VoxelColorPalette : public Resource {
    OBJECT(VoxelColorPalette);

public:
    VoxelColorPalette(Context* context) : Resource(context) { }
    ~VoxelColorPalette() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

private:
    /// Palette color data.
    VariantVector colors_;

public:
    /// Gets the colors.
    const VariantVector& GetColors() { return colors_; }

    /// Sets the colors.
    void SetColors(const VariantVector colors) { colors_ = colors; }

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);
};

class URHO3D_API VoxelTextureMap : public Resource {
    OBJECT(VoxelTextureMap);

public:
    VoxelTextureMap(Context* context) : Resource(context) { }
    ~VoxelTextureMap() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

    Texture* GetDiffuse1Texture() const { return diffuse1Texture; }

    Texture* GetDiffuse2Texture() const { return diffuse2Texture; }

    void SetDiffuse1Texture(Texture* texture);

    void SetDiffuse2Texture(Texture* texture);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);

private:
    SharedPtr<Texture> diffuse1Texture;
    SharedPtr<Texture> diffuse2Texture;
    PODVector<unsigned char> defaultTexture2Map;
    void SetDiffuse1TextureArrayAttr(const ResourceRef& value);
    void SetDiffuse2TextureArrayAttr(const ResourceRef& value);

};

class URHO3D_API VoxelBlocktypeMap : public Resource {
    OBJECT(VoxelBlocktypeMap);
    
public:
    VoxelBlocktypeMap(Context* context) : Resource(context) { }
    ~VoxelBlocktypeMap() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

    PODVector<unsigned char> blockColor;
    PODVector<unsigned char> blockGeometry;
    PODVector<unsigned char> blockVHeight;
    PODVector<unsigned char> blockTex1;
    PODVector<unsigned char> blockTex2;
    PODVector<unsigned char> blockSelector;
    PODVector<unsigned char> blockSideTextureRotation;
    PODVector<unsigned char[6]> blockColorFace;
    PODVector<unsigned char[6]> blockTex1Face;
    PODVector<unsigned char[6]> blockTex2Face;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);
};

class URHO3D_API VoxelOverlayMap : public Resource {
    OBJECT(VoxelOverlayMap);

public:
    VoxelOverlayMap(Context* context) : Resource(context) { }
    ~VoxelOverlayMap() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

    PODVector<unsigned char[6]> overlayTex1;
    PODVector<unsigned char[6]> overlayTex2;
    PODVector<unsigned char[6]> overlayColor;
    PODVector<unsigned char> overlaySideTextureRotation;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);
};


class URHO3D_API VoxelMap : public Resource {
    OBJECT(VoxelMap);
    friend class VoxelBuilder;
    static const unsigned NUM_BASIC_STREAMS = 17;
private:
    unsigned dataMask_;
    unsigned processorDataMask_;
    unsigned height_;
    unsigned width_;
    unsigned depth_;
    unsigned size_;
    unsigned strideX;
    unsigned strideZ;
    String loadVoxelBlocktypeMap_;
    Vector<VoxelProcessorFunc> voxelProcessors_;
    PODVector<unsigned char> blocktype;
    PODVector<unsigned char> overlay;
    PODVector<unsigned char> color;
    PODVector<unsigned char> color2;
    PODVector<unsigned char> color2Facemask;
    PODVector<unsigned char> color3;
    PODVector<unsigned char> color3Facemask;
    PODVector<unsigned char> extendedColor;
    PODVector<unsigned char> eColor;
    PODVector<unsigned char> eColorFaceMask;
    PODVector<unsigned char> geometry;
    PODVector<unsigned char> vHeight;
    PODVector<unsigned char> lighting;
    PODVector<unsigned char> rotate;
    PODVector<unsigned char> tex2;
    PODVector<unsigned char> tex2Replace;
    PODVector<unsigned char> tex2Facemask;

    void TransferAdjacentDataInternal(VoxelMap* source, int direction);
public:
    SharedPtr<VoxelBlocktypeMap> blocktypeMap;
    SharedPtr<VoxelTextureMap> textureMap;
    SharedPtr<VoxelOverlayMap> overlayMap;
    SharedPtr<VoxelColorPalette> colorPalette;

    unsigned char dataSourceType;

    /// Construct empty.
    VoxelMap(Context* context);

    /// Destruct.
    virtual ~VoxelMap();

    /// Register object factory.
    static void RegisterObject(Context* context);

    static void EncodeData(VoxelMap* voxelMap, Serializer& dest);
    static unsigned DecodeData(VoxelMap* voxelMap, Deserializer &source);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    virtual bool EndLoad();

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);

    /// Sets the block type data mask.
    void SetDataMask(unsigned dataMask) { dataMask_ = dataMask; }

    /// Sets the block type data mask.
    void SetProcessorDataMask(unsigned processorDataMask) { processorDataMask_ = processorDataMask; }

    /// Sets the blocktype map attribute
    void SetBlocktypeMap(VoxelBlocktypeMap*);
    
    /// Sets the blocktype map attribute
    void SetBlocktypeMapAttr(const ResourceRef&);

    /// Gets the blocktype map attribute
    ResourceRef GetBlocktypeMapAttr() const;

    unsigned GetHeight() const { return height_; }
    unsigned GetWidth() const { return width_; }
    unsigned GetDepth() const { return depth_; }
    unsigned GetSize() const { return size_; }
    unsigned GetDataMask() const { return dataMask_; }
    unsigned GetProcessorDataMask() const { return processorDataMask_; }
    unsigned GetStrideX() const { return strideX; }
    unsigned GetStrideZ() const { return strideZ; }

    void TransferAdjacentData(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west);
    void TransferAdjacentNorthData(VoxelMap* source);
    void TransferAdjacentSouthData(VoxelMap* source);
    void TransferAdjacentEastData(VoxelMap* source);
    void TransferAdjacentWestData(VoxelMap* source);

    inline unsigned GetIndex(int x, int y, int z) const { return (y + 2) + ((z + 2) * strideZ) + ((x + 2) * strideX); }
    void SetSize(unsigned width, unsigned height, unsigned depth);

    Vector<VoxelProcessorFunc> GetVoxelProcessors() { return voxelProcessors_; }
    void SetVoxelProcessors(Vector<VoxelProcessorFunc>& voxelProcessors) { voxelProcessors_ = voxelProcessors; }
    void AddVoxelProcessor(VoxelProcessorFunc voxelProcessor) { voxelProcessors_.Push(voxelProcessor); }
    void RemoveVoxelProcessor(unsigned index) { voxelProcessors_.Erase(index); }

    unsigned char GetBlocktype(int x, int y, int z) const { return blocktype[GetIndex(x,y,z)]; }
    unsigned char GetOverlay(int x, int y, int z) const { return overlay[GetIndex(x,y,z)]; }
    unsigned char GetColor(int x, int y, int z) const { return color[GetIndex(x,y,z)]; }
    unsigned char GetColor2(int x, int y, int z) const { return color2[GetIndex(x,y,z)]; }
    unsigned char GetColor2Facemask(int x, int y, int z) const { return color2Facemask[GetIndex(x,y,z)]; }
    unsigned char GetColor3(int x, int y, int z) const { return color3[GetIndex(x,y,z)]; }
    unsigned char GetColor3Facemask(int x, int y, int z)  const { return color3Facemask[GetIndex(x,y,z)]; }
    unsigned char GetExtendedColor(int x, int y, int z)  const { return extendedColor[GetIndex(x,y,z)]; }
    unsigned char GetEColor(int x, int y, int z)  const { return eColor[GetIndex(x,y,z)]; }
    unsigned char GetEColorFaceMask(int x, int y, int z)  const { return eColorFaceMask[GetIndex(x,y,z)]; }
    unsigned char GetGeometry(int x, int y, int z)  const { return geometry[GetIndex(x,y,z)]; }
    unsigned char GetVHeight(int x, int y, int z)  const { return vHeight[GetIndex(x,y,z)]; }
    unsigned char GetLighting(int x, int y, int z)  const { return lighting[GetIndex(x,y,z)]; }
    unsigned char GetRotate(int x, int y, int z)  const { return rotate[GetIndex(x,y,z)]; }
    unsigned char GetTex2(int x, int y, int z)  const { return tex2[GetIndex(x,y,z)]; }
    unsigned char GetTex2Replace(int x, int y, int z)  const { return tex2Facemask[GetIndex(x,y,z)]; }
    unsigned char GetTex2Facemask(int x, int y, int z)  const { return tex2Replace[GetIndex(x,y,z)]; }

    const PODVector<unsigned char>& GetBlocktypeData() const { return blocktype; }
    const PODVector<unsigned char>& GetOverlayData() const { return overlay; }
    const PODVector<unsigned char>& GetColorData() const { return color; }
    const PODVector<unsigned char>& GetColor2Data() const { return color2; }
    const PODVector<unsigned char>& GetColor2FacemaskData() const { return color2Facemask; }
    const PODVector<unsigned char>& GetColor3Data() const { return color3; }
    const PODVector<unsigned char>& GetColor3FacemaskData() const { return color3Facemask; }
    const PODVector<unsigned char>& GetExtendedColorData() const { return extendedColor; }
    const PODVector<unsigned char>& GetEColorData() const { return eColor; }
    const PODVector<unsigned char>& GetEColorFaceMaskData() const { return eColorFaceMask; }
    const PODVector<unsigned char>& GetGeometryData() const { return geometry; }
    const PODVector<unsigned char>& GetVHeightData() const { return vHeight; }
    const PODVector<unsigned char>& GetLightingData() const { return lighting; }
    const PODVector<unsigned char>& GetRotateData() const { return rotate; }
    const PODVector<unsigned char>& GetTex2Data() const { return tex2; }
    const PODVector<unsigned char>& GetTex2ReplaceData() const { return tex2Facemask; }
    const PODVector<unsigned char>& GetTex2FacemaskData() const { return tex2Replace; }

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

    inline void SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry, VoxelRotation rotation, VoxelHeight height)
    {
        geometry[GetIndex(x, y, z)] = VoxelEncodeGeometry(voxelGeometry, rotation, height);
    }

private:

    void GetBasicDataArrays(PODVector<unsigned char>** datas)
    {
	datas[0] = &blocktype;
	datas[1] = &color2;
	datas[2] = &color2Facemask;
	datas[3] = &color3;
	datas[4] = &color3Facemask;
	datas[5] = &color;
	datas[6] = &eColor;
	datas[7] = &eColorFaceMask;
	datas[8] = &extendedColor;
	datas[9] = &geometry;
	datas[10] = &lighting;
	datas[11] = &overlay;
	datas[12] = &rotate;
	datas[13] = &tex2;
	datas[14] = &tex2Facemask;
	datas[15] = &tex2Replace;
	datas[16] = &vHeight;
    }
};

//class Voxel
//{
//private:
//    unsigned index_;
//    VoxelMap* map_;
//
//public:
//
//    Voxel(){
//    }
//
//    Voxel(VoxelMap* map, int x, int y, int z)
//    {
//        map_ = map;
//        index_ = map->GetIndex(x, y, z);
//    }
//
//    inline void SetBlocktype(int x, int y, int z, unsigned char val)
//    {
//        map_->blocktype[index_] = val;
//    }
//
//    inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
//    {
//        map_->vHeight[index_] = VoxelEncodeVHeight(sw, se, nw, ne);
//    }
//};

}
