#include "VoxelDefs.h"
#include "VoxelUtils.h"
#include "VoxelData.h"

namespace Urho3D
{ 

VoxelData::VoxelData() :
    width_(0),
    height_(0),
    depth_(0),
    dataMask_(0),
    size_(0)
{

}

VoxelData::~VoxelData()
{

}

void VoxelData::SetSize(unsigned width, unsigned height, unsigned depth)
{
    height_ = height;
    width_ = width;
    depth_ = depth;
    strideZ_ = height + GetPadding() * 2;
    strideX_ = (height + GetPadding() * 2) * (depth + GetPadding() * 2);
    size_ = (width_ + GetPadding() * 2)*(height_ + GetPadding() * 2)*(depth_ + GetPadding() * 2);

    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    GetBasicDataArrays(datas);
    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask_ & (1 << i))
        {
            datas[i]->Resize(size_);
            memset(&datas[i]->Front(), 0, size_);
        }
    }
}

void VoxelData::SetColor(int x, int y, int z, unsigned char val)
{
    color[GetIndex(x, y, z)] = val;
}

void VoxelData::SetBlocktype(int x, int y, int z, unsigned char val)
{
    blocktype[GetIndex(x, y, z)] = val;
}

void VoxelData::SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
{
    vHeight[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
}

void VoxelData::SetLighting(int x, int y, int z, unsigned char val)
{
    lighting[GetIndex(x, y, z)] = val;
}

void VoxelData::SetTex2(int x, int y, int z, unsigned char val)
{
    tex2[GetIndex(x, y, z)] = val;
}

void VoxelData::SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry, VoxelRotation rotation, VoxelHeight height)
{
    geometry[GetIndex(x, y, z)] = VoxelEncodeGeometry(voxelGeometry, rotation, height);
}

void VoxelData::RawEncode(VoxelData* voxelMap, Serializer& dest)
{
    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();
    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask & (1 << i))
        {
            if (!datas[i]->Size())
                continue;
            dest.Write(&datas[i]->Front(), size);
        }
    }
}

unsigned VoxelData::RawDecode(VoxelData* voxelMap, Deserializer& source)
{
    unsigned numDatas = 0;
    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();

    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask & (1 << i))
        {
            numDatas++;
            datas[i]->Resize(size);
            source.Read(&datas[i]->Front(), size);
        }
    }
    return numDatas * size;
}

void VoxelData::RunLengthEncodeData(VoxelData* voxelMap, Serializer& dest)
{
    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();

    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask & (1 << i))
        {
            if (!datas[i]->Size())
                continue;

            unsigned char* data = &datas[i]->Front();
            unsigned char val = data[0];
            unsigned count = 1;
            unsigned position = 1;
            do
            {
                if (position == size || val != data[position])
                {
                    dest.WriteVLE(count);
                    dest.WriteUByte(val);
                    count = 0;
                    val = data[position];
                }
                count++;
            } while (position++ < size);
        }
    }
}

unsigned VoxelData::RunLengthDecodeData(VoxelData* voxelMap, Deserializer& source)
{
    unsigned numDatas = 0;
    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();

    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask & (1 << i))
        {
            numDatas++;
            datas[i]->Resize(size);
            unsigned char* dataPtr = &datas[i]->Front();
            unsigned position = 0;
            while (position < size)
            {
                unsigned count = source.ReadVLE();
                unsigned char val = source.ReadUByte();
                memset(dataPtr, val, count);
                dataPtr += count;
                position += count;
            }
        }
    }
    return numDatas * size;
}

void VoxelData::GetBasicDataArrays(PODVector<unsigned char>** datas)
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

}