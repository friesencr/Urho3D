#include "VoxelStore.h"
#include "Voxel.h"
#include "../Core/Profiler.h"
#include "../IO/Log.h"

namespace Urho3D
{

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
    voxelMap->SetBlocktypeMap(GetVoxelBlocktypeMap());
    voxelMap->SetDataMask(GetDataMask());
    voxelMap->SetProcessorDataMask(GetProcessorDataMask());
    voxelMap->SetVoxelProcessors(voxelProcessors_);
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
    numPagesX_ = ceilf((float)numChunksX_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
    numPagesY_ = ceilf((float)numChunksY_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
    numPagesZ_ = ceilf((float)numChunksZ_ / (float)(VOXEL_STORE_PAGE_SIZE_1D));
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
                pageIndex++;
            }
        }
    }

}

}
