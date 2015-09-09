#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/ResourceCache.h"

#include "VoxelMap.h"

#include "../DebugNew.h"

namespace Urho3D 
{

VoxelMap::VoxelMap(Context* context) : 
    Resource(context),
    blocktypeMap(0),
    processorDataMask_(0)
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

    unsigned dataSize = RunLengthDecodeData(this, memBuffer);
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
    PODVector<unsigned char>* srcDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
    PODVector<unsigned char>* destDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
    source->GetBasicDataArrays(srcDatas);
    this->GetBasicDataArrays(destDatas);

    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
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
                    for(unsigned p = 0; p < GetPadding(); ++p)
                        dstPtr[this->GetIndex(x, y, this->depth_ + p)] = srcPtr[source->GetIndex(x, y, p)];
        }
        else if (direction == VOXEL_NEIGHBOR_SOUTH)
        {
            for (unsigned x = 0; x < this->width_; ++x)
                for (unsigned y = 0; y < this->height_; ++y)
                    for(unsigned p = -GetPadding(); p < 0; ++p)
                        dstPtr[this->GetIndex(x, y, p)] = srcPtr[source->GetIndex(x, y, source->depth_ - p)];
        }
        else if (direction == VOXEL_NEIGHBOR_EAST)
        {
            for (unsigned z = 0; z < this->depth_; ++z)
                for (unsigned y = 0; y < this->height_; ++y)
                    for(unsigned p = 0; p < GetPadding(); ++p)
                        dstPtr[this->GetIndex(this->width_ + p, y, z)] = srcPtr[source->GetIndex(p, y, z)];
        }
        else if (direction == VOXEL_NEIGHBOR_WEST)
        {
            for (unsigned z = 0; z < this->depth_; ++z)
                for (unsigned y = 0; y < this->height_; ++y)
                    for(unsigned p = -GetPadding(); p < 0; ++p)
                        dstPtr[this->GetIndex(p, y, z)] = srcPtr[source->GetIndex(source->width_ - p, y, z)];
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

}
