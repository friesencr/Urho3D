#pragma once

#include "../Container/RefCounted.h"
#include "../Container/Vector.h"
#include "../IO/Serializer.h"
#include "../IO/Deserializer.h"

#include "VoxelUtils.h"

namespace Urho3D
{

class VoxelBuilder;
class VoxelMeshBuilder;

class VoxelData
{
friend class VoxelBuilder;
friend class VoxelMeshBuilder;

public:
    VoxelData();

    virtual ~VoxelData();

    static void RawEncode(VoxelData* voxelData, Serializer& dest);

    static unsigned RawDecode(VoxelData* voxelData, Deserializer &source);

    static void RunLengthEncodeData(VoxelData* voxelData, Serializer& dest);

    static unsigned RunLengthDecodeData(VoxelData* voxelData, Deserializer &source);

    unsigned GetHeight() const { return height_; }

    unsigned GetWidth() const { return width_; }

    unsigned GetDepth() const { return depth_; }

    unsigned GetSize() const { return size_; }

    unsigned GetDataMask() const { return dataMask_; }

    unsigned GetStrideX() const { return strideX_; }

    unsigned GetStrideZ() const { return strideZ_; }

    virtual inline unsigned GetPadding() const = 0;

    virtual inline unsigned GetIndex(int x, int y, int z) const { return (y + GetPadding()) + ((z + GetPadding()) * strideZ_) + ((x + GetPadding()) * strideX_); }

    void SetSize(unsigned width, unsigned height, unsigned depth);

    /// Sets the block type data mask.
    void SetDataMask(unsigned dataMask) { dataMask_ = dataMask; }

    void SetColor(int x, int y, int z, unsigned char val);

    void SetBlocktype(int x, int y, int z, unsigned char val);

    void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne);

    void SetLighting(int x, int y, int z, unsigned char val);

    void SetTex2(int x, int y, int z, unsigned char val);

    void SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry, VoxelRotation rotation, VoxelHeight height);

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

    PODVector<unsigned char>& GetBlocktypeData() { return blocktype; }
    PODVector<unsigned char>& GetOverlayData() { return overlay; }
    PODVector<unsigned char>& GetColorData() { return color; }
    PODVector<unsigned char>& GetColor2Data() { return color2; }
    PODVector<unsigned char>& GetColor2FacemaskData() { return color2Facemask; }
    PODVector<unsigned char>& GetColor3Data() { return color3; }
    PODVector<unsigned char>& GetColor3FacemaskData() { return color3Facemask; }
    PODVector<unsigned char>& GetExtendedColorData() { return extendedColor; }
    PODVector<unsigned char>& GetEColorData() { return eColor; }
    PODVector<unsigned char>& GetEColorFaceMaskData() { return eColorFaceMask; }
    PODVector<unsigned char>& GetGeometryData() { return geometry; }
    PODVector<unsigned char>& GetVHeightData() { return vHeight; }
    PODVector<unsigned char>& GetLightingData() { return lighting; }
    PODVector<unsigned char>& GetRotateData() { return rotate; }
    PODVector<unsigned char>& GetTex2Data() { return tex2; }
    PODVector<unsigned char>& GetTex2ReplaceData() { return tex2Facemask; }
    PODVector<unsigned char>& GetTex2FacemaskData() { return tex2Replace; }

protected:
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
    unsigned dataMask_;
    unsigned height_;
    unsigned width_;
    unsigned depth_;
    unsigned size_;
    unsigned strideX_;
    unsigned strideZ_;

    void GetBasicDataArrays(PODVector<unsigned char>** datas);

};

}
