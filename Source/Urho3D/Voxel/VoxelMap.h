#pragma once

#include "../Core/Object.h"
#include "../Resource/Resource.h"
#include "../Container/Vector.h"

#include "VoxelBlocktypeMap.h"
#include "VoxelTextureMap.h"
#include "VoxelOverlayMap.h"
#include "VoxelColorPalette.h"
#include "VoxelBuilder.h"

namespace Urho3D 
{

class URHO3D_API VoxelMap : public Resource {
    OBJECT(VoxelMap);
    friend class VoxelBuilder;
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
    unsigned GetStrideX() const { return strideX_; }
    unsigned GetStrideZ() const { return strideZ_; }

    void TransferAdjacentData(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west);
    void TransferAdjacentNorthData(VoxelMap* source);
    void TransferAdjacentSouthData(VoxelMap* source);
    void TransferAdjacentEastData(VoxelMap* source);
    void TransferAdjacentWestData(VoxelMap* source);
    void TransferAdjacentDataDirection(VoxelMap* source, int direction);

    inline unsigned GetIndex(int x, int y, int z) const { return (y + 2) + ((z + 2) * strideZ_) + ((x + 2) * strideX_); }
    void SetSize(unsigned width, unsigned height, unsigned depth);

    const PODVector<StringHash>& GetVoxelProcessors();
    void SetVoxelProcessors(PODVector<StringHash>& voxelProcessors);
    void AddVoxelProcessor(StringHash voxelProcessorName);
    void RemoveVoxelProcessor(const StringHash& voxelProcessorName);

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

    void SetColor(int x, int y, int z, unsigned char val);

    void SetBlocktype(int x, int y, int z, unsigned char val);

    void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne);

    void SetLighting(int x, int y, int z, unsigned char val);

    void SetTex2(int x, int y, int z, unsigned char val);

    void SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry, VoxelRotation rotation, VoxelHeight height);

private:
    unsigned dataMask_;
    unsigned processorDataMask_;
    unsigned height_;
    unsigned width_;
    unsigned depth_;
    unsigned size_;
    unsigned strideX_;
    unsigned strideZ_;
    String loadVoxelBlocktypeMap_;
    PODVector<StringHash> voxelProcessors_;
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

    void GetBasicDataArrays(PODVector<unsigned char>** datas);
};

}
