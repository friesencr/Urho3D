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
#include <Urho3D/Voxel/Voxel.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Voxel/VoxelWriter.h>
#include <Urho3D/IO/Log.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

struct VoxelBuildWorldWorkload
{
    unsigned chunkX;
    unsigned chunkY;
    unsigned chunkZ;
    SharedPtr<VoxelMap> voxelMap;
    VoxelStore* voxelStore;

};

#if 1
static void DancingWorld(VoxelData* src, VoxelData* dest, const VoxelRangeFragment& range)
{
    unsigned j = 0;
    for (int x = range.startX; x < range.endX; x++)
    {
        unsigned k = Rand();
        for (int z = range.startZ; z < range.endZ; z++)
        {
            for (int y = range.startY; y < range.endY; y++)
            {
                unsigned char bt = src->GetBlocktype(x, y, z);
                if (bt)
                {
                    dest->SetBlocktype(x, y, z, Max(((bt + j + k) % 64), 1));
                }
            }
        }
    }
}

static void AOVoxelLighting(VoxelData* src, VoxelData* dest, const VoxelRangeFragment& range)
{
    unsigned char* bt = 0;
    const int xStride = src->GetStrideX();
    const int zStride = src->GetStrideZ();

    // have to generate data 1 block outside the normal chunk data
    unsigned char* lightingBuffer = &dest->GetLightingData().Front();

    for (int x = range.startX; x < range.endX; x++)
    {
        for (int z = range.startZ; z < range.endZ; z++)
        {
            for (int y = range.startY; y < range.endY; y++)
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

                lightingBuffer[index - 1] += light * 255 / 27;
                lightingBuffer[index] += light * 255 / 27;
                lightingBuffer[index + 1] += light * 255 / 27;
            }
        }
    }
}
#endif


void FillTerrainPerlin(VoxelStore* voxelStore, VoxelMap* voxelMap, unsigned yOff, unsigned chunkX, unsigned chunkY, unsigned chunkZ)
{
    int heightMap[36][36];
    float detailMap[36][36];

    const float noiseFactors[10] = {
        1.0, 1.0, 1.0, 0.3,
        0.3, 0.4, 0.7, 1.0,
        1.0, 1.0
    };

    // build the heightmap
    for (int x = -2; x < 34; ++x)
    {
        for (int z = -2; z < 34; ++z)
        {
            // detail noise
            float dt = 0.0;
            for (int o = 3; o < 5; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + chunkX * 32) / scale, (z + chunkZ * 32) / scale, (float)-o, 256, 256, 256);
                dt += Abs(ns);
            }

            // low frequency
            float ht = 0.0;
            for (int o = 3; o < 9; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + chunkX * 32) / scale, (z + chunkZ * 32) / scale, (float)o, 256, 256, 256);
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

    int blocks[8] = { FLOOR_BLOCK, SLATE_BLOCK, DIRT_BLOCK, DESERT_BLOCK, GRASS_BLOCK, SNOW_BLOCK, LIGHT_SNOW_BLOCK, WHITE_SNOW_BLOCK };
    int heights[8] = { FLOOR_HEIGHT, SLATE_HEIGHT, DIRT_HEIGHT, DESERT_HEIGHT, GRASS_HEIGHT, SNOW_HEIGHT, LIGHT_SNOW_HEIGHT, WHITE_SNOW_HEIGHT };
    int numBlocks = 8;

    // fill block type data based on heightmap
    for (int x = -2; x < 34; ++x)
    {
        for (int z = -2; z < 34; ++z)
        {
            int height = heightMap[x + 2][z + 2];
            float dt = detailMap[x + 2][z + 2];

            int clampedHeight = Min(height, yOff + VOXEL_CHUNK_SIZE_Y);
            for (unsigned i = yOff; i < clampedHeight; ++i)
            {
                int h = ((float)i * (dt / 2.0 + 1.0));
                int b = 0;
                for (int bh = 0; bh < numBlocks - 1; ++bh)
                {
                    if (h < heights[bh])
                    {
                        b = bh;
                        break;
                    }
                }
                voxelMap->SetBlocktype(x, i - yOff, z, blocks[b] + (dt > 0.5 ? 1 : 0));
            }

            if (clampedHeight < WATER_HEIGHT)
            {
                for (unsigned i = yOff; i < WATER_HEIGHT; ++i)
                {
                    if (height > WATER_HEIGHT - 5 && i > WATER_HEIGHT - 5)
                        voxelMap->SetBlocktype(x, i, z, WATER_BLOCK);
                    else
                        voxelMap->SetBlocktype(x, i, z, DEEP_WATER_BLOCK + (dt > 0.7 ? 1 : 0));
                }
            }
        }
    }
    voxelStore->UpdateVoxelMap(chunkX, chunkY, chunkZ, voxelMap, false);
}

void BuildWorldWorkloadHandler(const WorkItem* workItem, unsigned thread)
{
    VoxelBuildWorldWorkload* workload = (VoxelBuildWorldWorkload*)workItem->aux_;
    //FillTerrainPerlin( workload->voxelStore, workload->voxelMap, workload->chunkX, workload->chunkY, workload->chunkZ );
    workload->voxelMap = 0;
    delete workItem->aux_;
}

WorldBuilder::WorldBuilder(Context* context) : Object(context)
{

}

WorldBuilder::~WorldBuilder()
{

}

void WorldBuilder::ConfigureParameters()
{
    VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
    //builder->RegisterProcessor("DancingWorld", DancingWorld);
    builder->RegisterProcessor("AOVoxelLighting", AOVoxelLighting);

    voxelStore_ = new VoxelStore(context_);
    voxelStore_->SetCompressionMask(VOXEL_COMPRESSION_RLE);
    voxelStore_->SetDataMask(VOXEL_BLOCK_BLOCKTYPE);
    voxelStore_->SetSize(width_, 1, depth_);

    voxelBlocktypeMap_ = new VoxelBlocktypeMap(context_);
    voxelBlocktypeMap_->blockColor.Push(0);
    for (unsigned i = 1; i < 64; ++i)
        voxelBlocktypeMap_->blockColor.Push(i);
    voxelStore_->SetVoxelBlocktypeMap(voxelBlocktypeMap_);

    voxelBlocktypeMap_->SetName("ColorVoxelBlocktypeMap");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddManualResource(voxelBlocktypeMap_);

    voxelSet_->AddVoxelProcessor("AOVoxelLighting");
    //voxelSet->AddVoxelProcessor("DancingWorld");
    voxelSet_->SetProcessorDataMask(VOXEL_BLOCK_LIGHTING);
    voxelSet_->SetVoxelStore(voxelStore_);
    voxelSet_->SetVoxelColorPalette(new VoxelColorPalette(context_));
}

void WorldBuilder::CreateWorld()
{
    //SharedPtr<VoxelMap> voxelMap = voxelStore_->GetVoxelMap(0, 0, 0);
    //voxelMap->SetDataMask(VOXEL_BLOCK_BLOCKTYPE);
    //voxelMap->SetSize(64, 128, 64);
    //for (unsigned x = 0; x < 64; ++x)
    //{
    //    for (unsigned y = 0; y < 64; ++y)
    //    {
    //        for (unsigned z = 0; z < 64; ++z)
    //        {
    //            if (x >= 15 && x < 18 && y >= 15 && y < 18 && z >= 15 && z < 18)
    //            {
    //                voxelMap->SetBlocktype(x, y, z, 25);
    //            }
    //        }
    //    }
    //}
    //voxelStore_->UpdateVoxelMap(0, 0, 0, voxelMap, false);
    //return;

    WorkQueue* queue = GetSubsystem<WorkQueue>();
    for (int x = 0; x < width_; ++x)
    {
        for (int z = 0; z < depth_; ++z)
        {
            for (int y = 0; y < 1; ++y)
            {
                SharedPtr<VoxelMap> voxelMap = voxelStore_->GetVoxelMap(x, y, z);
                voxelMap->SetDataMask(VOXEL_BLOCK_BLOCKTYPE);
                voxelMap->SetSize(VOXEL_CHUNK_SIZE_X, VOXEL_CHUNK_SIZE_Y, VOXEL_CHUNK_SIZE_Z);
                FillTerrainPerlin(voxelStore_, voxelMap, y * VOXEL_CHUNK_SIZE_Y, x, y, z);
                voxelStore_->UpdateVoxelMap(x, y, z, voxelMap, false);

#if 0
                VoxelBuildWorldWorkload* workload = new VoxelBuildWorldWorkload();
                workload->chunkX = x;
                workload->chunkY = 0;
                workload->chunkZ = z;
                workload->voxelMap = voxelMap;
                workload->voxelStore = voxelStore_;

                SharedPtr<WorkItem> workItem(new WorkItem());
                workItem->aux_ = workload;

                workItem->workFunction_ = BuildWorldWorkloadHandler;
                queue->AddWorkItem(workItem);
#endif

            }
        }
    }
}

void WorldBuilder::SaveWorld()
{
    voxelStore_->SetName("Data/VoxelWorldMap/VoxelWorld");
    {
        File file(context_, "Data/VoxelWorldMap/VoxelWorld.vox", FILE_WRITE);
        if (!voxelStore_->Save(file))
        {
            LOGERROR("Error saving voxel world.");
            return;
        }
    }
}

void WorldBuilder::LoadWorld()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    voxelStore_ = cache->GetResource<VoxelStore>("VoxelWorldMap/VoxelWorld.vox");
    VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
    builder->RegisterProcessor("AOVoxelLighting", AOVoxelLighting);
    //voxelSet_->AddVoxelProcessor("AOVoxelLighting");
    voxelSet_->SetProcessorDataMask(VOXEL_BLOCK_LIGHTING);
    voxelSet_->SetVoxelStore(voxelStore_);
}

void WorldBuilder::BuildWorld()
{
    voxelSet_->Build();
}
