#include "../Core/Profiler.h"
#include "../IO/Log.h"
#include "../IO/VectorBuffer.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../IO/Compression.h"

#include "VoxelStore.h"

#include "../DebugNew.h"

namespace Urho3D
{

static void ApplyBrushFragment(VoxelData* src, VoxelData* dest, const VoxelRange& range)
{

}

VoxelMapPage::VoxelMapPage(Context* context) :
    Resource(context)
{

}

VoxelMapPage::~VoxelMapPage()
{

}

void VoxelMapPage::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelMapPage>("Voxel");
}

void VoxelMapPage::SetDataMask(unsigned dataMask)
{
    dataMask_ = dataMask;
}

bool VoxelMapPage::BeginLoad(Deserializer& source)
{
    if (source.ReadFileID() != "VOXP")
        return false;

    compressionMask_ = source.ReadUByte();
    dataMask_ = source.ReadUInt();
    // resource ref
    for (unsigned i = 0; i < VOXEL_STORE_PAGE_SIZE_3D; ++i)
        buffers_[i] = source.ReadBuffer();

    return true;
}

bool VoxelMapPage::Save(Serializer& dest)
{
    if (!dest.WriteFileID("VOXP"))
        return false;

    dest.WriteUByte(compressionMask_);
    dest.WriteUInt(dataMask_);
    for (unsigned i = 0; i < VOXEL_STORE_PAGE_SIZE_3D; ++i)
        dest.WriteBuffer(buffers_[i]);
    
    dirtySave_ = false;
    return true;
}

void VoxelMapPage::SetVoxelMap(unsigned index, VoxelMap* voxelMap)
{
    if (index >= VOXEL_STORE_PAGE_SIZE_3D)
        return;

    // TODO validate sizes
    VectorBuffer dest;

    if (compressionMask_ & VOXEL_COMPRESSION_RLE)
        VoxelData::RunLengthEncodeData(voxelMap, dest);
    else
        VoxelData::RawEncode(voxelMap, dest);

    if (compressionMask_ & VOXEL_COMPRESSION_LZ4)
        dest = CompressVectorBuffer(dest);

    dirtySave_ = true;

    buffers_[index] = dest.GetBuffer();
}

SharedPtr<VoxelMap> VoxelMapPage::GetVoxelMap(unsigned index)
{
    SharedPtr<VoxelMap> voxelMap;
    if (index >= VOXEL_STORE_PAGE_SIZE_3D)
        return voxelMap;

    VectorBuffer source(buffers_[index]);
    if (source.GetSize())
    {
        voxelMap = new VoxelMap(context_);
        voxelMap->SetDataMask(dataMask_);
        voxelMap->SetSize(VOXEL_CHUNK_SIZE_X, VOXEL_CHUNK_SIZE_Y, VOXEL_CHUNK_SIZE_Z);

        if (compressionMask_ & VOXEL_COMPRESSION_LZ4)
            source = DecompressVectorBuffer(source);

        if (compressionMask_ & VOXEL_COMPRESSION_RLE)
            VoxelData::RunLengthDecodeData(voxelMap, source);
        else
            VoxelData::RawDecode(voxelMap, source);

        return voxelMap;
    }
    else
    {
        voxelMap = new VoxelMap(context_);
        voxelMap->SetDataMask(dataMask_);
        voxelMap->SetSize(VOXEL_CHUNK_SIZE_X, VOXEL_CHUNK_SIZE_Y, VOXEL_CHUNK_SIZE_Z);
        return voxelMap;
    }
}


VoxelStore::VoxelStore(Context* context) : Resource(context)
    , dataMask_(0)
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
    , compressionMask_(0)
{

}

VoxelStore::~VoxelStore() {

};

void VoxelStore::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelStore>("Voxel");
}

bool VoxelStore::BeginLoad(Deserializer& source)
{
    if (source.ReadFileID() != "VOXS")
        return false;

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    SetCompressionMask(source.ReadUByte());
    SetDataMask(source.ReadUInt());
    SetSize(source.ReadUInt(), source.ReadUInt(), source.ReadUInt());

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

bool VoxelStore::Save(Serializer& dest)
{
    if (!dest.WriteFileID("VOXS"))
        return false;

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    dest.WriteUByte(compressionMask_);
    dest.WriteUInt(dataMask_);
    dest.WriteUInt(numChunksX_);
    dest.WriteUInt(numChunksY_);
    dest.WriteUInt(numChunksZ_);

    for (unsigned i = 0; i < numPages_; ++i)
    {
        String pageName = ReplaceExtension(GetName(), String(".").AppendWithFormat("%d", i));
        File file(context_, pageName, FILE_WRITE);
        if (!voxelMapPages_[i]->Save(file))
            return false;
    }
    return true;
}

void VoxelStore::UpdateVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap, bool updateNeighbors)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return;

    VoxelMapPage* page = GetVoxelMapPageByChunkIndex(x, y, z);
    if (!page)
    {
        LOGERROR("Could not retrieve voxel page for requested voxel map index");
        return;
    }

    if (updateNeighbors)
    {
        unsigned index = GetMapIndex(x, y, z);
        {
            unsigned neighborID = index + chunkZStride_;
            SharedPtr<VoxelMap> north(GetVoxelMap(x, y, z + 1));
            if (north)
            {
                north->TransferAdjacentSouthData(voxelMap);
                voxelMap->TransferAdjacentNorthData(north);
                UpdateVoxelMap(x, y, z + 1, north, false);
            }
        }
        {
            unsigned neighborID = index - chunkZStride_;
            SharedPtr<VoxelMap> south(GetVoxelMap(x, y, z - 1));
            if (south)
            {
                south->TransferAdjacentNorthData(voxelMap);
                voxelMap->TransferAdjacentSouthData(south);
                UpdateVoxelMap(x, y, z - 1, south, false);
            }
        }
        {
            unsigned neighborID = index + chunkXStride_;
            SharedPtr<VoxelMap> east(GetVoxelMap(x + 1, y, z));
            if (east)
            {
                east->TransferAdjacentWestData(voxelMap);
                voxelMap->TransferAdjacentEastData(east);
                UpdateVoxelMap(x + 1, y, z, east, false);
            }
        }
        {
            SharedPtr<VoxelMap> west(GetVoxelMap(x - 1, y, z));
            if (west)
            {
                west->TransferAdjacentEastData(voxelMap);
                voxelMap->TransferAdjacentWestData(west);
                UpdateVoxelMap(x - 1, y, z, west, false);
            }
        }
    }

    unsigned mapIndex = GetVoxelMapIndexInPage(x, y, z);
    page->SetVoxelMap(mapIndex, voxelMap);
    page->dirtySave_ = true;
}

VoxelMapPage* VoxelStore::GetVoxelMapPageByChunkIndex(unsigned x, unsigned y, unsigned z)
{
    unsigned index = 
        ((x / (VOXEL_STORE_PAGE_SIZE_1D)) * numPagesZ_) +
        (y / (VOXEL_STORE_PAGE_SIZE_1D)) +
        ((z / (VOXEL_STORE_PAGE_SIZE_1D)) * numPagesY_);

    if (index >= numPages_)
        return 0;
    return voxelMapPages_[index];
}

unsigned VoxelStore::GetVoxelMapIndexInPage(unsigned x, unsigned y, unsigned z)
{
    return (y & VOXEL_STORE_PAGE_MASK_1D) | 
        ((z & VOXEL_STORE_PAGE_MASK_1D) << VOXEL_STORE_PAGE_STRIDE_Z) | 
        ((x & VOXEL_STORE_PAGE_MASK_1D) << VOXEL_STORE_PAGE_STRIDE_X);
}

SharedPtr<VoxelMap> VoxelStore::GetVoxelMap(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return SharedPtr<VoxelMap>();

#if VOXEL_MAP_CACHE
    unsigned chunkIndex = GetMapIndex(x, y, z);
    VoxelMapCacheNode* cacheItem = 0;
    {
        PROFILE(GetVoxelMapFromCache);
        cacheItem = voxelMapCache_.First();
        while (cacheItem)
        {
            if (cacheItem->index_ == chunkIndex)
            {
                voxelMap = cacheItem->voxelMap_;
                break;
            }
            cacheItem = voxelMapCache_.Next(cacheItem);
        }
    }

    if (voxelMap)
        return voxelMap;
#endif

    VoxelMapPage* page = GetVoxelMapPageByChunkIndex(x, y, z);
    if (!page)
    {
        LOGERROR("Could not retrieve voxel page for requested voxel map index");
        return SharedPtr<VoxelMap>();
    }

    SharedPtr<VoxelMap> voxelMap(page->GetVoxelMap(GetVoxelMapIndexInPage(x, y, z)));
    voxelMap->SetDataMask(GetDataMask());
    voxelMap->SetBlocktypeMap(blocktypeMap_);
    return voxelMap;

#if VOXEL_MAP_CACHE
    if (voxelMap)
    {
        if (voxelMapCacheCount_ == 60)
        {
            voxelMapCacheCount_--;
            voxelMapCache_.Erase(voxelMapCache_.Last());
        }
        cacheItem = new VoxelMapCacheNode();
        cacheItem->index_ = chunkIndex;
        cacheItem->voxelMap_ = voxelMap;
        voxelMapCache_.Insert(cacheItem);
        voxelMapCacheCount_++;
    }
#endif
}

void VoxelStore::SetSize(unsigned numChunksX, unsigned numChunksY, unsigned numChunksZ)
{
    numChunksX_ = numChunksX;
    numChunksY_ = numChunksY;
    numChunksZ_ = numChunksZ;
    numChunks_ = numChunksX * numChunksY * numChunksZ;
    SetSizeInternal();
}

void VoxelStore::SetSizeInternal()
{
    // chunk settings
    chunkZStride_ = numChunksY_;
    chunkXStride_ = numChunksY_ * numChunksZ_;

    // voxel map page settings
    numPagesX_ = (unsigned)ceilf((float)numChunksX_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
    numPagesY_ = (unsigned)ceilf((float)numChunksY_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
    numPagesZ_ = (unsigned)ceilf((float)numChunksZ_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
    numPages_ = numPagesX_ * numPagesY_ * numPagesZ_;
    voxelMapPages_.Resize(numPages_);
    for (unsigned i = 0; i < numPages_; ++i)
        voxelMapPages_[i] = new VoxelMapPage(context_);

    // initialize voxelmap pages
    unsigned pageIndex = 0;
    for (unsigned x = 0; x < numPagesX_; ++x)
    {
        for (unsigned z = 0; z < numPagesZ_; ++z)
        {
            for (unsigned y = 0; y < numPagesY_; ++y)
            {
                VoxelMapPage* voxelMapPage = voxelMapPages_[pageIndex];
                voxelMapPage->SetDataMask(dataMask_);
                voxelMapPage->compressionMask_ = compressionMask_;
                pageIndex++;
            }
        }
    }

}

void VoxelStore::SetCompressionMask(unsigned compressionMask)
{
    compressionMask_ = compressionMask;
}

unsigned VoxelStore::GetCompressionMask() const
{
    return compressionMask_;
}

void VoxelStore::ApplyBrushStroke(VoxelBrush* brush, unsigned positionX, unsigned positionY, unsigned positionZ)
{
    unsigned globalStartX = positionX;
    unsigned globalEndX = positionX + brush->GetWidth();
    unsigned globalStartY = positionY;
    unsigned globalEndY = positionY + brush->GetHeight();
    unsigned globalStartZ = positionZ;
    unsigned globalEndZ = positionZ + brush->GetDepth();

    unsigned mapStartX = positionX / VOXEL_CHUNK_SIZE_X;
    unsigned mapEndX = (positionX + brush->GetWidth()) / VOXEL_CHUNK_SIZE_X;
    unsigned mapStartY = positionY / VOXEL_CHUNK_SIZE_Y;
    unsigned mapEndY = (positionY + brush->GetHeight()) / VOXEL_CHUNK_SIZE_Y;
    unsigned mapStartZ = positionZ / VOXEL_CHUNK_SIZE_Z;
    unsigned mapEndZ = (positionZ + brush->GetDepth()) / VOXEL_CHUNK_SIZE_Z;
    
    for (unsigned x = mapStartX; x <= mapEndX; ++x)
    {
        for (unsigned z = mapStartZ; z <= mapEndZ; ++z)
        {
            for (unsigned y = mapStartY; y <= mapEndY; ++y)
            {
                VoxelRangeFragment range;
                range.startX = x * VOXEL_CHUNK_SIZE;
                range.endX = (x + 1) * VOXEL_CHUNK_SIZE;
            }
        }
    }
}



}
