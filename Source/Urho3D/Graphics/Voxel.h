#pragma once

#include "../Math/Vector3.h"
#include "../Core/Object.h"
#include <STB/stb_voxel_render.h>
#include "../Math/Color.h"
#include "Texture2DArray.h"
#include "../Container/ArrayPtr.h"
#include "../Scene/Serializable.h"
#include "../IO/Generator.h"

namespace Urho3D {

class VoxelChunk;
class VoxelMap;

struct VoxelRGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

class VoxelMapData : public RefCounted {
public:
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
};

struct VoxelProcessorWriters;
typedef void(*VoxelProcessorFunc)(VoxelChunk* chunk, VoxelMap* source, VoxelProcessorWriters writers);

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

unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled);

unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast);

unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot, VoxelHeight height);

unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot);

unsigned char VoxelEncodeGeometry(VoxelGeometry geometry);

unsigned char VoxelEncodeFaceMask(bool east, bool north, bool west, bool south, bool up, bool down);

unsigned char VoxelEncodeSideTextureSwap(unsigned char block, unsigned char overlay, unsigned char ecolor);

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

    inline void SetGeometry(int x, int y, int z, VoxelGeometry geometry)
    {
        buffer[GetIndex(x, y, z)] = VoxelEncodeGeometry(geometry);
    }
};

class URHO3D_API VoxelMap : public Serializable {
    OBJECT(VoxelMap);
    friend class VoxelBuilder;
    static const unsigned MAX_LOADED_MAPS = 200;
    static const unsigned NUM_BASIC_STREAMS = 17;
private:
	WeakPtr<VoxelMapData> voxelMapData_;

public:
    SharedPtr<VoxelBlocktypeMap> blocktypeMap;
    SharedPtr<VoxelTextureMap> textureMap;
    SharedPtr<VoxelOverlayMap> overlayMap;
    SharedPtr<VoxelColorPalette> colorPalette;

    unsigned dataMask_;
    unsigned processorDataMask_;
    unsigned height_;
    unsigned width_;
    unsigned depth_;
    unsigned size_;
    unsigned xStride;
    unsigned zStride;
	unsigned char dataSourceType;
    Vector<VoxelProcessorFunc> voxelProcessors_;
    ResourceRef resourceRef_;
    Generator generator_;

    /// Construct empty.
    VoxelMap(Context* context);

    /// Destruct.
    virtual ~VoxelMap();

    /// Register object factory.
    static void RegisterObject(Context* context);

    virtual bool Load(Deserializer& source);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);

    /// Unloads block information from memory.
    virtual void Unload();

    /// Reloads the voxel map if source is available.
    virtual bool Reload(bool force=false);

    /// See if voxel map is loaded into memory.
    virtual bool IsLoaded();

    /// Sets the data source to a generator
    virtual void SetSource(Generator generator);

    /// Sets the data source to a resource reference
    virtual void SetSource(ResourceRef resourceRef);

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

	PODVector<unsigned char>* GetBlocktype() const { return &voxelMapData_->blocktype; }
	PODVector<unsigned char>* GetOverlay() const { return &voxelMapData_->overlay; }
	PODVector<unsigned char>* GetColor() const { return &voxelMapData_->color; }
	PODVector<unsigned char>* GetColor2() const { return &voxelMapData_->color2; }
	PODVector<unsigned char>* GetColor2Facemask() const { return &voxelMapData_->color2Facemask; }
	PODVector<unsigned char>* GetColor3() const { return &voxelMapData_->color3; }
	PODVector<unsigned char>* GetColor3Facemask() const { return &voxelMapData_->color3Facemask; }
	PODVector<unsigned char>* GetExtendedColor() const { return &voxelMapData_->extendedColor; }
	PODVector<unsigned char>* GetEColor() const { return &voxelMapData_->eColor; }
	PODVector<unsigned char>* GetEColorFaceMask() const { return &voxelMapData_->eColorFaceMask; }
	PODVector<unsigned char>* GetGeometry() const { return &voxelMapData_->geometry; }
	PODVector<unsigned char>* GetVHeight() const { return &voxelMapData_->vHeight; }
	PODVector<unsigned char>* GetLighting() const { return &voxelMapData_->lighting; }
	PODVector<unsigned char>* GetRotate() const { return &voxelMapData_->rotate; }
	PODVector<unsigned char>* GetTex2() const { return &voxelMapData_->tex2; }
	PODVector<unsigned char>* GetTex2Replace() const { return &voxelMapData_->tex2Facemask; }
	PODVector<unsigned char>* GetTex2Facemask() const { return &voxelMapData_->tex2Replace; }

	unsigned char* GetBlocktypeRaw() const { return &voxelMapData_->blocktype.Front(); }
	unsigned char* GetOverlayRaw() const { return &voxelMapData_->overlay.Front(); }
	unsigned char* GetColorRaw() const { return &voxelMapData_->color.Front(); }
	unsigned char* GetColor2Raw() const { return &voxelMapData_->color2.Front(); }
	unsigned char* GetColor2FacemaskRaw() const { return &voxelMapData_->color2Facemask.Front(); }
	unsigned char* GetColor3Raw() const { return &voxelMapData_->color3.Front(); }
	unsigned char* GetColor3FacemaskRaw() const { return &voxelMapData_->color3Facemask.Front(); }
	unsigned char* GetExtendedColorRaw() const { return &voxelMapData_->extendedColor.Front(); }
	unsigned char* GetEColorRaw() const { return &voxelMapData_->eColor.Front(); }
	unsigned char* GetEColorFaceMaskRaw() const { return &voxelMapData_->eColorFaceMask.Front(); }
	unsigned char* GetGeometryRaw() const { return &voxelMapData_->geometry.Front(); }
	unsigned char* GetVHeightRaw() const { return &voxelMapData_->vHeight.Front(); }
	unsigned char* GetLightingRaw() const { return &voxelMapData_->lighting.Front(); }
	unsigned char* GetRotateRaw() const { return &voxelMapData_->rotate.Front(); }
	unsigned char* GetTex2Raw() const { return &voxelMapData_->tex2.Front(); }
	unsigned char* GetTex2ReplaceRaw() const { return &voxelMapData_->tex2Facemask.Front(); }
	unsigned char* GetTex2FacemaskRaw() const { return &voxelMapData_->tex2Replace.Front(); }


    inline void SetColor(int x, int y, int z, unsigned char val)
    {
        voxelMapData_->color[GetIndex(x, y, z)] = val;
    }

    inline void SetBlocktype(int x, int y, int z, unsigned char val)
    {
        voxelMapData_->blocktype[GetIndex(x, y, z)] = val;
    }

    inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
    {
        voxelMapData_->vHeight[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
    }

    inline void SetLighting(int x, int y, int z, unsigned char val)
    {
        voxelMapData_->lighting[GetIndex(x, y, z)] = val;
    }

    inline void SetTex2(int x, int y, int z, unsigned char val)
    {
        voxelMapData_->tex2[GetIndex(x, y, z)] = val;
    }

    inline void SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry)
    {
        voxelMapData_->geometry[GetIndex(x, y, z)] = VoxelEncodeGeometry(voxelGeometry);
    }

private:

    void GetBasicDataArrays(PODVector<unsigned char>** datas)
    {
            datas[0] = &voxelMapData_->blocktype;
            datas[1] = &voxelMapData_->color2;
            datas[2] = &voxelMapData_->color2Facemask;
            datas[3] = &voxelMapData_->color3;
            datas[4] = &voxelMapData_->color3Facemask;
            datas[5] = &voxelMapData_->color;
            datas[6] = &voxelMapData_->eColor;
            datas[7] = &voxelMapData_->eColorFaceMask;
            datas[8] = &voxelMapData_->extendedColor;
            datas[9] = &voxelMapData_->geometry;
            datas[10] = &voxelMapData_->lighting;
            datas[11] = &voxelMapData_->overlay;
            datas[12] = &voxelMapData_->rotate;
            datas[13] = &voxelMapData_->tex2;
            datas[14] = &voxelMapData_->tex2Facemask;
            datas[15] = &voxelMapData_->tex2Replace;
            datas[16] = &voxelMapData_->vHeight;
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
