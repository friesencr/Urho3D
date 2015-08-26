#pragma once

#include "../IO/MemoryBuffer.h"
#include "../Scene/Component.h"
#include "../Resource/Resource.h"
#include "../Container/LinkedList.h"
#include "Voxel.h"

namespace Urho3D
{ 

#define VOXEL_NEIGHBOR_DIRTY(x,y,z) \
    if (x < 2) \
        west = true; \


struct VoxelPageItem : public RefCounted
{
    unsigned char dirty;

};

struct VoxelMapCacheNode : public LinkedListNode
{
    SharedPtr<VoxelMap> voxelMap_;
    unsigned index_;
};

class VoxelMapPage : public Resource
{
public:
    OBJECT(VoxelMapPage);
    VoxelMapPage(Context* context) : Resource(context) {}
    virtual ~VoxelMapPage() {}

private:
    unsigned dataMask_;
    unsigned processorDataMask_;
    unsigned height_;
    unsigned width_;
    unsigned depth_;
    Vector<PODVector<unsigned char> > buffers_;

public:

    void SetDataMask(unsigned dataMask)
    {
        dataMask_ = dataMask;
    }

	void SetVoxelMapSize(unsigned width, unsigned height, unsigned depth)
	{
		width_ = width;
		height_ = height;
		depth_ = depth;
	}

    void SetNumberOfMaps(unsigned numberOfMaps)
    {
        buffers_.Resize(numberOfMaps);
    }

    bool BeginLoad(Deserializer& source)
    {
        if (source.ReadFileID() != "VOXP")
            return false;

        SetVoxelMapSize(source.ReadUInt(), source.ReadUInt(), source.ReadUInt());
        dataMask_ = source.ReadUInt();
        unsigned numberOfBuffers = source.ReadUInt();
        // resource ref
        for (unsigned i = 0; i < numberOfBuffers; ++i)
            buffers_[i] = source.ReadBuffer();

        return true;
    }

    bool Save(Serializer& dest)
    {
        dest.WriteUInt(width_);
        dest.WriteUInt(height_);
        dest.WriteUInt(depth_);
        dest.WriteUInt(dataMask_);
        dest.WriteUInt(buffers_.Size());
        for (unsigned i = 0; i < buffers_.Size(); ++i)
            dest.WriteBuffer(buffers_[i]);

        return true;
    }

    void SetVoxelMap(unsigned index, VoxelMap* voxelMap)
    {
        if (index >= buffers_.Size())
            return;

        // TODO validate sizes
        VectorBuffer dest;
        VoxelMap::EncodeData(voxelMap, dest);
        buffers_[index] = dest.GetBuffer();
    }

    SharedPtr<VoxelMap> GetVoxelMap(unsigned index)
    {
        SharedPtr<VoxelMap> voxelMap;
        if (index >= buffers_.Size())
            return voxelMap;

        MemoryBuffer source(buffers_[index]);
        if (source.GetSize())
        {
            voxelMap = new VoxelMap(context_);
            voxelMap->SetDataMask(dataMask_);
            voxelMap->SetSize(width_, height_, depth_);
            VoxelMap::DecodeData(voxelMap, source);
            return voxelMap;
        }
        else
        {
            voxelMap = new VoxelMap(context_);
            voxelMap->SetDataMask(dataMask_);
            voxelMap->SetSize(width_, height_, depth_);
            return voxelMap;
        }
    }
};

class URHO3D_API VoxelStore : public Resource
{
    const static unsigned MAX_PAGE_SIZE = 8;
    OBJECT(VoxelStore);

public:
    VoxelStore(Context* context) : Resource(context) {}
    virtual ~VoxelStore() {};
    void UpdateVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap, bool updateNeighbors = true);
    SharedPtr<VoxelMap> GetVoxelMap(unsigned x, unsigned y, unsigned z);
    void SetSize(unsigned numChunksX, unsigned numChunksY, unsigned numChunksZ, unsigned chunkSizeX, unsigned chunkSizeY, unsigned chunkSizeZ);
    void SetDataMask(unsigned dataMask) { dataMask_ = dataMask; }
    unsigned GetDataMask() const { return dataMask_; }
    unsigned GetMapIndex(int x, int y, int z) { return x * chunkXStride_ + y + z * chunkZStride_; }
    unsigned GetNumChunks() const { return numChunks_; }
    unsigned GetNumChunksX() const { return numChunksX_; }
    unsigned GetNumChunksY() const { return numChunksY_; }
    unsigned GetNumChunksZ() const { return numChunksZ_; }
    unsigned GetChunkSizeX() const { return chunkSizeX_; }
    unsigned GetChunkSizeY() const { return chunkSizeY_; }
    unsigned GetChunkSizeZ() const { return chunkSizeZ_; }
    void SetVoxelBlocktypeMap(VoxelBlocktypeMap* voxelBlocktypeMap) { voxelBlocktypeMap_ = voxelBlocktypeMap; }
    VoxelBlocktypeMap* GetVoxelBlocktypeMap() { return voxelBlocktypeMap_; }

private:
    void SetSizeInternal();
    SharedPtr<VoxelBlocktypeMap> voxelBlocktypeMap_;
    VoxelMapPage* GetVoxelMapPageByChunkIndex(unsigned x, unsigned y, unsigned z);
    unsigned GetVoxelMapIndexInPage(unsigned x, unsigned y, unsigned z);

    unsigned dataMask_;
    unsigned chunkSizeX_;
    unsigned chunkSizeY_;
    unsigned chunkSizeZ_;
    unsigned numChunks_;
    unsigned numChunksX_;
    unsigned numChunksY_;
    unsigned numChunksZ_;
    unsigned chunkXStride_;
    unsigned chunkZStride_;
    unsigned numPagesX_;
    unsigned numPagesY_;
    unsigned numPagesZ_;
    unsigned numPages_;
    unsigned pageSizeX_;
    unsigned pageSizeY_;
    unsigned pageSizeZ_;
    unsigned pageStrideX_;
    unsigned pageStrideY_;
    unsigned pageStrideZ_;
    unsigned voxelMapCacheCount_;
    LinkedList<VoxelMapCacheNode> voxelMapCache_;
    Vector<SharedPtr<VoxelMapPage> > voxelMapPages_;
};

}
