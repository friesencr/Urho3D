#pragma once

#include "../Container/LinkedList.h"
#include "../IO/Serializer.h"
#include "../IO/Deserializer.h"
#include "../Resource/Resource.h"

#include "VoxelBlocktypeMap.h"
#include "VoxelMap.h"

namespace Urho3D
{ 
class VoxelMap;

struct VoxelMapCacheNode : public LinkedListNode
{
    SharedPtr<VoxelMap> voxelMap_;
    unsigned index_;
};

class VoxelMapPage : public Resource
{
    OBJECT(VoxelMapPage);

public:
    VoxelMapPage(Context* context);
    virtual ~VoxelMapPage();
    static void RegisterObject(Context* context);
    void SetDataMask(unsigned dataMask);
    bool BeginLoad(Deserializer& source);
    bool Save(Serializer& dest);
    void SetVoxelMap(unsigned index, VoxelMap* voxelMap);
    SharedPtr<VoxelMap> GetVoxelMap(unsigned index);

private:
    unsigned dataMask_;
    unsigned processorDataMask_;
    PODVector<unsigned char> buffers_[VOXEL_STORE_PAGE_SIZE_3D];

};

class URHO3D_API VoxelStore : public Resource
{
    OBJECT(VoxelStore);

public:
    static const unsigned MAX_PAGES = 4096;
    VoxelStore(Context* context);
    virtual ~VoxelStore();

    static void RegisterObject(Context* context);

    bool BeginLoad(Deserializer& source);

    bool Save(Serializer& dest);

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
    const PODVector<StringHash>& GetVoxelProcessors();
    void SetVoxelProcessors(PODVector<StringHash>& voxelProcessors);
    void AddVoxelProcessor(StringHash voxelProcessorName);
    void RemoveVoxelProcessor(const StringHash& voxelProcessorName);


private:
    void SetSizeInternal();
    SharedPtr<VoxelBlocktypeMap> voxelBlocktypeMap_;
    VoxelMapPage* GetVoxelMapPageByChunkIndex(unsigned x, unsigned y, unsigned z);
    inline unsigned GetVoxelMapIndexInPage(unsigned x, unsigned y, unsigned z);
    PODVector<StringHash> voxelProcessors_;

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
