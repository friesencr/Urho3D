#pragma once

#include "../Core/Context.h"
#include "../IO/MemoryBuffer.h"
#include "../Scene/Component.h"
#include "../Resource/Resource.h"
#include "../Resource/ResourceCache.h"
#include "../Container/LinkedList.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "Voxel.h"

namespace Urho3D
{ 

static const unsigned VOXEL_STORE_CHUNK_SIZE_X = 64;
static const unsigned VOXEL_STORE_CHUNK_SIZE_Y = 128;
static const unsigned VOXEL_STORE_CHUNK_SIZE_Z = 64;
static const unsigned VOXEL_STORE_PAGE_SIZE_1D = 8;
static const unsigned VOXEL_STORE_PAGE_MASK_1D = 0x7;
static const unsigned VOXEL_STORE_PAGE_SIZE_2D = 8 * 8;
static const unsigned VOXEL_STORE_PAGE_SIZE_3D = 8 * 8 * 8;
static const unsigned VOXEL_STORE_PAGE_STRIDE_X = 6; // bits
static const unsigned VOXEL_STORE_PAGE_STRIDE_Z = 3; // bits

#define VOXEL_NEIGHBOR_DIRTY(x,y,z) \
    if (x < 2) \
        west = true; \

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
    PODVector<unsigned char> buffers_[VOXEL_STORE_PAGE_SIZE_3D];

public:

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelMapPage>("Voxel");
    }

    void SetDataMask(unsigned dataMask)
    {
        dataMask_ = dataMask;
    }

    bool BeginLoad(Deserializer& source)
    {
        if (source.ReadFileID() != "VOXP")
            return false;

        dataMask_ = source.ReadUInt();
        // resource ref
        for (unsigned i = 0; i < VOXEL_STORE_PAGE_SIZE_3D; ++i)
            buffers_[i] = source.ReadBuffer();

        return true;
    }

    bool Save(Serializer& dest)
    {
        if (!dest.WriteFileID("VOXP"))
            return false;

        dest.WriteUInt(dataMask_);
        for (unsigned i = 0; i < VOXEL_STORE_PAGE_SIZE_3D; ++i)
            dest.WriteBuffer(buffers_[i]);

        return true;
    }

    void SetVoxelMap(unsigned index, VoxelMap* voxelMap)
    {
        if (index >= VOXEL_STORE_PAGE_SIZE_3D)
            return;

        // TODO validate sizes
        VectorBuffer dest;
        VoxelMap::EncodeData(voxelMap, dest);
        buffers_[index] = dest.GetBuffer();
    }

    SharedPtr<VoxelMap> GetVoxelMap(unsigned index)
    {
        SharedPtr<VoxelMap> voxelMap;
        if (index >= VOXEL_STORE_PAGE_SIZE_3D)
            return voxelMap;

        MemoryBuffer source(buffers_[index]);
        if (source.GetSize())
        {
            voxelMap = new VoxelMap(context_);
            voxelMap->SetDataMask(dataMask_);
            voxelMap->SetSize(VOXEL_STORE_CHUNK_SIZE_X, VOXEL_STORE_CHUNK_SIZE_Y, VOXEL_STORE_CHUNK_SIZE_Z);
            VoxelMap::DecodeData(voxelMap, source);
            return voxelMap;
        }
        else
        {
            voxelMap = new VoxelMap(context_);
            voxelMap->SetDataMask(dataMask_);
            voxelMap->SetSize(VOXEL_STORE_CHUNK_SIZE_X, VOXEL_STORE_CHUNK_SIZE_Y, VOXEL_STORE_CHUNK_SIZE_Z);
            return voxelMap;
        }
    }
};

class URHO3D_API VoxelStore : public Resource
{
    OBJECT(VoxelStore);

public:
    static const unsigned MAX_PAGES = 4096;
    VoxelStore(Context* context) : Resource(context)
        , dataMask_(0)
        , processorDataMask_(0)
        , numChunks_(0)
        , numChunksX_(0)
        , numChunksY_(0)
        , numChunksZ_(0)
        , chunkXStride_(0)
        , chunkZStride_(0)
        , numPagesX_(0)
        , numPagesY_(0)
        , numPagesZ_(0)
        , numPages_(0)
        , voxelBlocktypeMap_(0)
    {
    }
    virtual ~VoxelStore() {};

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelStore>("Voxel");
    }

    bool BeginLoad(Deserializer& source)
    {
        if (source.ReadFileID() != "VOXS")
            return false;

        ResourceCache* cache = GetSubsystem<ResourceCache>();

        SetDataMask(source.ReadUInt());
        SetProcessorDataMask(source.ReadUInt());
        SetSize(source.ReadUInt(), source.ReadUInt(), source.ReadUInt());
        ResourceRef blocktypeMap = source.ReadResourceRef();
        SetVoxelBlocktypeMap(cache->GetResource<VoxelBlocktypeMap>(blocktypeMap.name_));

        for (unsigned i = 0; i < numPages_; ++i)
        {
            String pageName = ReplaceExtension(source.GetName(), String(".").AppendWithFormat("%d", i));
            VoxelMapPage* page = cache->GetResource<VoxelMapPage>(pageName);
            if (!page)
                return false;
            voxelMapPages_[i] = page;
        }

        return true;
    }

    bool Save(Serializer& dest)
    {
        if (!dest.WriteFileID("VOXS"))
            return false;

        ResourceCache* cache = GetSubsystem<ResourceCache>();

        dest.WriteUInt(dataMask_);
        dest.WriteUInt(processorDataMask_);
        dest.WriteUInt(numChunksX_);
        dest.WriteUInt(numChunksY_);
        dest.WriteUInt(numChunksZ_);
        if (voxelBlocktypeMap_)
            dest.WriteResourceRef(ResourceRef(VoxelBlocktypeMap::GetTypeNameStatic(), voxelBlocktypeMap_->GetName()));
        else
            dest.WriteResourceRef(Variant::emptyResourceRef);

        for (unsigned i = 0; i < numPages_; ++i)
        {
            String pageName = ReplaceExtension(GetName(), String(".").AppendWithFormat("%d", i));
            File file(context_, pageName, FILE_WRITE);
            if (!voxelMapPages_[i]->Save(file))
                return false;
        }
        return true;
    }



    void UpdateVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap, bool updateNeighbors = true);
    SharedPtr<VoxelMap> GetVoxelMap(unsigned x, unsigned y, unsigned z);
    void SetSize(unsigned numChunksX, unsigned numChunksY, unsigned numChunksZ);
    void SetDataMask(unsigned dataMask) { dataMask_ = dataMask; }
    unsigned GetDataMask() const { return dataMask_; }
    unsigned GetMapIndex(int x, int y, int z) { return x * chunkXStride_ + y + z * chunkZStride_; }
    unsigned GetNumChunks() const { return numChunks_; }
    unsigned GetNumChunksX() const { return numChunksX_; }
    unsigned GetNumChunksY() const { return numChunksY_; }
    unsigned GetNumChunksZ() const { return numChunksZ_; }
    void SetVoxelBlocktypeMap(VoxelBlocktypeMap* voxelBlocktypeMap) { voxelBlocktypeMap_ = voxelBlocktypeMap; }
    VoxelBlocktypeMap* GetVoxelBlocktypeMap() { return voxelBlocktypeMap_; }

    unsigned GetProcessorDataMask() const { return processorDataMask_; }
    void SetProcessorDataMask(unsigned processorDataMask) { processorDataMask_ = processorDataMask; }
    Vector<VoxelProcessorFunc>& GetVoxelProcessors() { return voxelProcessors_; }
    void SetVoxelProcessors(Vector<VoxelProcessorFunc>& voxelProcessors) { voxelProcessors_ = voxelProcessors; }
    void AddVoxelProcessor(VoxelProcessorFunc voxelProcessor) { voxelProcessors_.Push(voxelProcessor); }
    void RemoveVoxelProcessor(unsigned index) { voxelProcessors_.Erase(index); }

private:
    void SetSizeInternal();
    SharedPtr<VoxelBlocktypeMap> voxelBlocktypeMap_;
    VoxelMapPage* GetVoxelMapPageByChunkIndex(unsigned x, unsigned y, unsigned z);
    inline unsigned GetVoxelMapIndexInPage(unsigned x, unsigned y, unsigned z);
    Vector<VoxelProcessorFunc> voxelProcessors_;

    unsigned dataMask_;
    unsigned processorDataMask_;
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
    unsigned voxelMapCacheCount_;
    LinkedList<VoxelMapCacheNode> voxelMapCache_;
    Vector<SharedPtr<VoxelMapPage> > voxelMapPages_;
};

}
