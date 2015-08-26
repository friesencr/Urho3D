//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho3D.h>

#include "WorldBuilder.h"
#include <Urho3D/Graphics/Voxel.h>
#include <Urho3D/Graphics/VoxelChunk.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/ResourceCache.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"


#if 1
    static void AOVoxelLighting(VoxelChunk* chunk, VoxelMap* src, VoxelProcessorWriters writers)
{
    const unsigned char* bt = 0;
    const int xStride = src->GetStrideX();
    const int zStride = src->GetStrideZ();

    for (int x = -1; x < (int)src->GetWidth()+1; x++)
    {
        for (int z = -1; z < (int)src->GetDepth()+1; z++)
        {
            for (int y = -1; y < (int)src->GetHeight()+1; y++)
            {
                int index = src->GetIndex(x, y, z);
                bt = &src->GetBlocktypeData()[index];

#ifdef SMOOTH_TERRAIN
                if (bt[0] > 0)
                {
                    if (bt[1] == 0)
                    {
                        VoxelHeight scores[4] = { VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0 };
                        int corners[4] = { -xStride - zStride, xStride - zStride, -xStride + zStride, xStride + zStride };
                        unsigned checks[4][6] = {
                            { zStride, zStride+1, 0, 1, xStride, xStride+1 },
                            { zStride, zStride+1, 0, 1, -xStride, -xStride+1 },
                            { -zStride, -zStride+1, 0, 1, xStride, xStride+1 },
                            { -zStride, -zStride+1, 0, 1, -xStride, -xStride+1 }
                        };
                        //int checks[4][3] = {
                        //    { zStride+1, 1, xStride+1 },
                        //    { zStride+1, 1, -xStride+1 },
                        //    { -zStride+1, 1, xStride+1 },
                        //    { -zStride+1, 1, -xStride+1 }
                        //};

                        unsigned score = 0;
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            for (unsigned j = 0; j < 6; ++j)
                            {
                                score += bt[corners[i] + checks[i][j]] > 0;
                            }
                            scores[i] = (VoxelHeight)(unsigned)(score/6);
                        }
                        writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_FLOOR_VHEIGHT_03);
                        writers.vHeight.buffer[index] = VoxelEncodeVHeight(scores[0], scores[1], scores[2], scores[3]);
                    }
                    //else if (bt[-1] == 0)
                    //{
                    //    //writers.geometry.buffer[index] = VoxelEncodeGeometry(x*z % 2 == 0 ? VOXEL_TYPE_CEIL_VHEIGHT_03 : VOXEL_TYPE_CEIL_VHEIGHT_12);
                    //    writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_CEIL_VHEIGHT_03);
                    //    writers.vHeight.buffer[index] = VoxelEncodeVHeight(
                    //        (bt[-xStride - zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,   // sw
                    //        (bt[xStride - zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,    // se
                    //        (bt[-xStride + zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,   // nw
                    //        (bt[xStride + zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0    // ne
                    //    );
                    //}
                    else
                        writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_SOLID);
                }
#endif
                int light = 
                    (bt[-xStride - zStride] == 0) +  // nw
                    (bt[-zStride] == 0) +            // n
                    (bt[xStride - zStride] == 0) +   // ne
                    (bt[-xStride] == 0) +            // w
                    (bt[0] == 0) +                   // origin
                    (bt[xStride] == 0) +             // e
                    (bt[-xStride + zStride] == 0) +  // sw
                    (bt[zStride] == 0) +             // s
                    (bt[xStride + zStride] == 0);    // se

                writers.lighting.buffer[index - 1] += light * 255 / 27;
                writers.lighting.buffer[index] += light * 255 / 27;
                writers.lighting.buffer[index + 1] += light * 255 / 27;
            }
        }
    }
}
#endif

struct WorldBuildWorkload
{
    VoxelMap* voxelMap;
    unsigned chunkX;
    unsigned chunkY;
    unsigned chunkZ;
    int startX;
    int endX;
    int startZ;
    int endZ;
};

void FillTerrainPerlinWorker(const WorkItem* workItem, unsigned threadIndex)
{
	WorldBuildWorkload* workload = (WorldBuildWorkload*)workItem->aux_;
    VoxelMap* voxelMap = workload->voxelMap;
    int heightMap[68][68];
    float detailMap[68][68];

    const float noiseFactors[10] = {
        1.0, 1.0, 1.0, 0.3,
        0.3, 0.4, 0.7, 1.0,
        1.0, 1.0
    };

    // build the heightmap
    for (int x = workload->startX; x < workload->endX; ++x)
    {
        for (int z = workload->startZ; z < workload->endZ; ++z)
        {
            // detail noise
            float dt = 0.0;
            for (int o = 3; o < 5; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + workload->chunkX * 64) / scale, (z + workload->chunkZ * 64) / scale, (float)-o, 256, 256, 256);
                dt += Abs(ns);
            }

            // low frequency
            float ht = 0.0;
            for (int o = 3; o < 9; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + workload->chunkX * 64) / scale, (z + workload->chunkZ * 64) / scale, (float)o, 256, 256, 256);
                ht += ns * noiseFactors[o];
            }

            // biome
            //float biome = stb_perlin_noise3((x + chunkX) / 2048, (z + chunkZ) / 2048, 32.0, 256, 256, 256);

            int height = (int)((ht + 0.2) * 45.0) + 32;
            heightMap[x + 2][z + 2] = Clamp(height, 1, 128); 
            detailMap[x + 2][z + 2] = dt;
        }
    }

    const int DEEP_WATER_BLOCK = 27;
    const int WATER_BLOCK = 28;
    const int DESERT_BLOCK = 44;
    const int DIRT_BLOCK = 44;
    const int GRASS_BLOCK = 36;
    const int SLATE_BLOCK = 29;
    const int FLOOR_BLOCK = 12;
    const int SNOW_BLOCK = 5;
    const int LIGHT_SNOW_BLOCK = 2;
    const int WHITE_SNOW_BLOCK = 0;

    const int FLOOR_HEIGHT = 10;
    const int SLATE_HEIGHT = 20;
    const int DIRT_HEIGHT = 30;
    const int DESERT_HEIGHT = 45;
    const int GRASS_HEIGHT = 70;
    const int SNOW_HEIGHT = 90;
    const int LIGHT_SNOW_HEIGHT = 105;
    const int WHITE_SNOW_HEIGHT = 128;
    const int WATER_HEIGHT = 30;

    int blocks[8] = {FLOOR_BLOCK, SLATE_BLOCK, DIRT_BLOCK, DESERT_BLOCK, GRASS_BLOCK, SNOW_BLOCK, LIGHT_SNOW_BLOCK, WHITE_SNOW_BLOCK };
    int heights[8] = {FLOOR_HEIGHT, SLATE_HEIGHT, DIRT_HEIGHT, DESERT_HEIGHT, GRASS_HEIGHT, SNOW_HEIGHT, LIGHT_SNOW_HEIGHT, WHITE_SNOW_HEIGHT };
    int numBlocks = 8;

    // fill block type data based on heightmap
    for (int x = workload->startX; x < workload->endX; ++x)
    {
        for (int z = workload->startZ; z < workload->endZ; ++z)
        {
            int height = heightMap[x + 2][z + 2];
            float dt = detailMap[x + 2][z + 2];

            for (unsigned i = 0; i < height; ++i)
            {
                int h = ((float)i * (dt/2.0 + 1.0));
                int b = 0;
                for (int bh = 0; bh < numBlocks - 1; ++bh)
                {
                    if (h < heights[bh])
                    {
                        b = bh;
                        break;
                    }
                }
                voxelMap->SetBlocktype(x, i, z, blocks[b] + (dt > 0.5 ? 1 : 0));
            }

            if (height < WATER_HEIGHT)
            {
                for (unsigned i = 0; i < WATER_HEIGHT; ++i)
                {
                    if (height > WATER_HEIGHT - 5 && i > WATER_HEIGHT - 5)
                        voxelMap->SetBlocktype(x, i, z, WATER_BLOCK);
                    else
                        voxelMap->SetBlocktype(x, i, z, DEEP_WATER_BLOCK +(dt > 0.7 ? 1 : 0));
                }
            }
        }
    }
    delete workload;
}


WorldBuilder::WorldBuilder(Context* context) : Object(context)
{

}

WorldBuilder::~WorldBuilder()
{

}

void WorldBuilder::ConfigureParameters()
{
    voxelBlocktypeMap_ = new VoxelBlocktypeMap(context_);
    voxelBlocktypeMap_->blockColor.Push(0);
    for (unsigned i = 1; i < 64; ++i)
        voxelBlocktypeMap_->blockColor.Push(i);

    voxelBlocktypeMap_->SetName("ColorVoxelBlocktypeMap");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddManualResource(voxelBlocktypeMap_);

    voxelStore_ = new VoxelStore(context_);
    voxelStore_->SetDataMask(VOXEL_BLOCK_BLOCKTYPE);
    voxelStore_->SetSize(width_, 1, depth_, 64, 128, 64);
    voxelStore_->SetVoxelBlocktypeMap(voxelBlocktypeMap_);
    voxelSet_->SetVoxelStore(voxelStore_);

    //File file(context_);
    //if (file.Open("BlocktypeMap.bin", FILE_WRITE))
    //    voxelBlocktypeMap_->Save(file);
}

void WorldBuilder::BuildWorld()
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();
	for (unsigned x = 0; x < width_; ++x)
	{
		for (unsigned z = 0; z < depth_; ++z)
		{
            const unsigned chunkSize = 16;
            SharedPtr<VoxelMap> voxelMap = voxelStore_->GetVoxelMap(x, 0, z);
            if (!voxelMap)
                voxelMap = new VoxelMap(context_);

            // split up into pieces for performance
            for (unsigned blockX = 0; blockX < 4; ++blockX)
            {
                for (unsigned blockZ = 0; blockZ < 4; ++blockZ)
                {
                    SharedPtr<WorkItem> workItem(new WorkItem());
                    WorldBuildWorkload* workload = new WorldBuildWorkload();
                    workload->voxelMap = voxelMap;
                    workload->chunkX = x;
                    workload->chunkZ = z;
                    // starts and ends need to generate padding around the chunks
                    workload->startX = blockX ? blockX * chunkSize : -2;
                    workload->startZ = blockZ ? blockZ * chunkSize : -2;
                    workload->endX = blockX == 3 ? 66 : (blockX + 1) * chunkSize;
                    workload->endZ = blockZ == 3 ? 66 : (blockZ + 1) * chunkSize;
                    workItem->aux_ = workload;
                    workItem->priority_ = M_MAX_UNSIGNED;
                    workItem->workFunction_ = FillTerrainPerlinWorker;
                    queue->AddWorkItem(workItem);
                }
            }

            queue->Complete(M_MAX_UNSIGNED);
            voxelStore_->UpdateVoxelMap(x, 0, z, voxelMap, false);
		}
	}
}

void WorldBuilder::LoadWorld()
{
    //for (unsigned x = 0; x < width_ * 16; ++x)
    //{
    //    for (unsigned z = 0; z < depth_ * 16; ++z)
    //    {
    //        String filename = "VoxelWorldMap/VoxelWorldMap_" + String(x % 64) + "_" + String(z % 64) + ".bin";
    //        voxelSet_->SetVoxelMapResource(x, 0, z, filename);
    //    }
    //}
    //voxelSet_->Build();
}