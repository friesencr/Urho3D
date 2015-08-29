#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/ResourceCache.h"

#include "VoxelMap.h"

#include "../DebugNew.h"

namespace Urho3D 
{

VoxelMap::VoxelMap(Context* context) 
    : Resource(context),
    width_(0),
    height_(0),
    depth_(0),
    dataMask_(0),
    processorDataMask_(0),
    blocktypeMap(0)
{

}

VoxelMap::~VoxelMap()
{

}

/// Register object factory.
void VoxelMap::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelMap>("Voxel");
}

void VoxelMap::SetColor(int x, int y, int z, unsigned char val)
{
    color[GetIndex(x, y, z)] = val;
}

void VoxelMap::SetBlocktype(int x, int y, int z, unsigned char val)
{
    blocktype[GetIndex(x, y, z)] = val;
}

void VoxelMap::SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
{
    vHeight[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
}

void VoxelMap::SetLighting(int x, int y, int z, unsigned char val)
{
    lighting[GetIndex(x, y, z)] = val;
}

void VoxelMap::SetTex2(int x, int y, int z, unsigned char val)
{
    tex2[GetIndex(x, y, z)] = val;
}

void VoxelMap::SetGeometry(int x, int y, int z, VoxelGeometry voxelGeometry, VoxelRotation rotation, VoxelHeight height)
{
    geometry[GetIndex(x, y, z)] = VoxelEncodeGeometry(voxelGeometry, rotation, height);
}


void VoxelMap::EncodeData(VoxelMap* voxelMap, Serializer& dest)
{
    PODVector<unsigned char>* datas[VOXEL_MAP_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();
    for (unsigned i = 0; i < VOXEL_MAP_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask & (1 << i))
        {
            unsigned char* data = &datas[i]->Front();
            if (!datas[i]->Size())
                continue;

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

unsigned VoxelMap::DecodeData(VoxelMap* voxelMap, Deserializer& source)
{
    unsigned numDatas = 0;
    PODVector<unsigned char>* datas[VOXEL_MAP_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);
    unsigned dataMask = voxelMap->GetDataMask();
    unsigned size = voxelMap->GetSize();

    for (unsigned i = 0; i < VOXEL_MAP_NUM_BASIC_STREAMS; ++i)
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

bool VoxelMap::BeginLoad(Deserializer& source)
{
    PROFILE(VoxelMapLoad);
    if (source.ReadFileID() != "VOXM")
        return false;

    PODVector<unsigned char> buffer(source.GetSize());
    source.Read(&buffer.Front(), source.GetSize());
    MemoryBuffer memBuffer(buffer);

    SetSize(memBuffer.ReadUInt(), memBuffer.ReadUInt(), memBuffer.ReadUInt());
    dataMask_ = memBuffer.ReadUInt();
    
    ResourceRef ref = memBuffer.ReadResourceRef();
    loadVoxelBlocktypeMap_ = ref.name_;

    unsigned dataSize = DecodeData(this, memBuffer);
    SetMemoryUse(sizeof(VoxelMap) + dataSize);
    return true;
}

bool VoxelMap::EndLoad()
{
    if (!loadVoxelBlocktypeMap_.Empty())
    {
        SetBlocktypeMapAttr(ResourceRef(VoxelBlocktypeMap::GetTypeStatic(), loadVoxelBlocktypeMap_));
        loadVoxelBlocktypeMap_ = String::EMPTY;
    }
    return true;
}

bool VoxelMap::Save(Serializer& dest)
{
    if (!dest.WriteFileID("VOXM"))
    {
        LOGERROR("Failed to save voxel map");
        return false;
    }
    dest.WriteUInt(width_);
    dest.WriteUInt(height_);
    dest.WriteUInt(depth_);
    dest.WriteUInt(dataMask_);
    dest.WriteResourceRef(GetBlocktypeMapAttr());

    return true;
}

void VoxelMap::SetSize(unsigned width, unsigned height, unsigned depth)
{
    height_ = height;
    width_ = width;
    depth_ = depth;
    strideZ_ = height + 4;
    strideX_ = (height + 4) * (depth + 4);
    size_ = (width_ + 4)*(height_ + 4)*(depth_ + 4);

    PODVector<unsigned char>* datas[VOXEL_MAP_NUM_BASIC_STREAMS];
    GetBasicDataArrays(datas);
    for (unsigned i = 0; i < VOXEL_MAP_NUM_BASIC_STREAMS; ++i)
    {
        if (dataMask_ & (1 << i))
        {
            datas[i]->Resize(size_);
            memset(&datas[i]->Front(), 0, size_);
        }
    }
}

void VoxelMap::SetBlocktypeMap(VoxelBlocktypeMap* voxelBlocktypeMap)
{
    blocktypeMap = voxelBlocktypeMap;
}

void VoxelMap::SetBlocktypeMapAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    VoxelBlocktypeMap* blocktypeMap = cache->GetResource<VoxelBlocktypeMap>(value.name_);
    SetBlocktypeMap(blocktypeMap);
}

ResourceRef VoxelMap::GetBlocktypeMapAttr() const
{
    return GetResourceRef(blocktypeMap, VoxelBlocktypeMap::GetTypeStatic());
}

void VoxelMap::TransferAdjacentData(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west)
{
    TransferAdjacentDataDirection(north, VOXEL_NEIGHBOR_NORTH);
    TransferAdjacentDataDirection(south, VOXEL_NEIGHBOR_SOUTH);
    TransferAdjacentDataDirection(east, VOXEL_NEIGHBOR_EAST);
    TransferAdjacentDataDirection(west, VOXEL_NEIGHBOR_WEST);
}

void VoxelMap::TransferAdjacentNorthData(VoxelMap* source)
{
    TransferAdjacentDataDirection(source, VOXEL_NEIGHBOR_NORTH);
}

void VoxelMap::TransferAdjacentSouthData(VoxelMap* source)
{
    TransferAdjacentDataDirection(source, VOXEL_NEIGHBOR_SOUTH);
}

void VoxelMap::TransferAdjacentEastData(VoxelMap* source)
{
    TransferAdjacentDataDirection(source, VOXEL_NEIGHBOR_EAST);
}

void VoxelMap::TransferAdjacentWestData(VoxelMap* source)
{
    TransferAdjacentDataDirection(source, VOXEL_NEIGHBOR_WEST);
}

void VoxelMap::TransferAdjacentDataDirection(VoxelMap* source, int direction)
{
    if (!source)
        return;

    // Copy adjacent data
    PODVector<unsigned char>* srcDatas[VOXEL_MAP_NUM_BASIC_STREAMS];
    PODVector<unsigned char>* destDatas[VOXEL_MAP_NUM_BASIC_STREAMS];
    source->GetBasicDataArrays(srcDatas);
    this->GetBasicDataArrays(destDatas);

    for (unsigned i = 0; i < VOXEL_MAP_NUM_BASIC_STREAMS; ++i)
    {
        if (!this->dataMask_ & (1 << i))
            continue;

        if (!source->dataMask_ & (1 << i))
            continue;

        unsigned char* src = 0;
        unsigned char* dst = 0;

        if (srcDatas[i]->Size() == this->size_)
        {
            if (destDatas[i]->Size() == this->size_)
            {
                src = &srcDatas[i]->Front();
                dst = &destDatas[i]->Front();
            }
        }

        if (!(src && dst))
            continue;

        unsigned char* srcPtr = src;
        unsigned char* dstPtr = dst;

        if (direction == VOXEL_NEIGHBOR_NORTH)
        {
            for (unsigned x = 0; x < this->width_; ++x)
                for (unsigned y = 0; y < this->height_; ++y)
                {
                    dstPtr[this->GetIndex(x, y, this->depth_)] = srcPtr[source->GetIndex(x, y, 0)];
                    dstPtr[this->GetIndex(x, y, this->depth_ + 1)] = srcPtr[source->GetIndex(x, y, 1)];
                }
        }
        else if (direction == VOXEL_NEIGHBOR_SOUTH)
        {
            for (unsigned x = 0; x < this->width_; ++x)
                for (unsigned y = 0; y < this->height_; ++y)
                {
                    dstPtr[this->GetIndex(x, y, -1)] = srcPtr[source->GetIndex(x, y, source->depth_ - 1)];
                    dstPtr[this->GetIndex(x, y, -2)] = srcPtr[source->GetIndex(x, y, source->depth_ - 2)];
                }
        }
        else if (direction == VOXEL_NEIGHBOR_EAST)
        {
            for (unsigned z = 0; z < this->depth_; ++z)
                for (unsigned y = 0; y < this->height_; ++y)
                {
                    dstPtr[this->GetIndex(this->width_, y, z)] = srcPtr[source->GetIndex(0, y, z)];
                    dstPtr[this->GetIndex(this->width_ + 1, y, z)] = srcPtr[source->GetIndex(1, y, z)];
                }
        }
        else if (direction == VOXEL_NEIGHBOR_WEST)
        {
            for (unsigned z = 0; z < this->depth_; ++z)
                for (unsigned y = 0; y < this->height_; ++y)
                {
                    dstPtr[this->GetIndex(-1, y, z)] = srcPtr[source->GetIndex(source->width_ - 1, y, z)];
                    dstPtr[this->GetIndex(-2, y, z)] = srcPtr[source->GetIndex(source->width_ - 2, y, z)];
                }
        }
    }
}

const PODVector<StringHash>& VoxelMap::GetVoxelProcessors()
{ 
    return voxelProcessors_;
}

void VoxelMap::SetVoxelProcessors(PODVector<StringHash>& voxelProcessors) 
{ 
    voxelProcessors_ = voxelProcessors; 
}

void VoxelMap::AddVoxelProcessor(StringHash voxelProcessorHash)
{ 
    voxelProcessors_.Push(voxelProcessorHash); 
}

void VoxelMap::RemoveVoxelProcessor(const StringHash& voxelProcessorHash) 
{ 
    voxelProcessors_.Remove(voxelProcessorHash);
}

void VoxelMap::GetBasicDataArrays(PODVector<unsigned char>** datas)
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
