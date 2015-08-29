#pragma once

#include "VoxelUtils.h"
#include "VoxelMap.h"

namespace Urho3D 
{

class URHO3D_API VoxelWriter
{
    friend class VoxelBuilder;
public:
    VoxelWriter();
    ~VoxelWriter();
    inline unsigned GetIndex(int x, int y, int z);
    unsigned GetSize() const { return size_; }
    unsigned GetStrideX() const { return strideX_; }
    unsigned GetStrideZ() const { return strideZ_; }
    void SetSize(unsigned width, unsigned height, unsigned depth);
    void SetBuffer(unsigned char* data);
    unsigned char* GetBuffer();
    void Clear(unsigned char value);
    inline void Set(int x, int y, int z, unsigned char val);
    inline void SetColor(int x, int y, int z, unsigned char val);
    inline void SetBlocktype(int x, int y, int z, unsigned char val);
    inline void SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne);
    inline void SetLighting(int x, int y, int z, unsigned char val);
    inline void SetTex2(int x, int y, int z, unsigned char val);
    inline void SetGeometry(int x, int y, int z, VoxelGeometry geometry, VoxelRotation rotation, VoxelHeight height);

private:
    unsigned size_;
    unsigned strideX_;
    unsigned strideZ_;
    unsigned char* buffer_;
};

struct URHO3D_API VoxelProcessorWriters
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

typedef void(*VoxelProcessorFunc)(VoxelMap* source, const VoxelRangeFragment& range, VoxelProcessorWriters writers);

}
