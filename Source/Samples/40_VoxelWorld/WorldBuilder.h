#pragma once

#include <Urho3D/Graphics/Voxel.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/VoxelSet.h>

using namespace Urho3D;

class WorldBuilder : public Object
{
    OBJECT(WorldBuilder);

    int height_;
    int width_;
    int depth_;
    SharedPtr<VoxelSet> voxelSet_;
    SharedPtr<VoxelBlocktypeMap> voxelBlocktypeMap_;

public:

    WorldBuilder(Context*);
    ~WorldBuilder();
    void SetVoxelSet(VoxelSet* voxelSet) { voxelSet_ = voxelSet; }
    void ConfigureParameters();
    void BuildWorld();
    void LoadWorld();
    void SetSize(unsigned width, unsigned depth) { height_ = 1; width_ = width; depth_ = depth; }
};

#if 0
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

static void FillTerrainPerlin(unsigned char* dataPtr, VariantMap& parameters, unsigned podIndex)
{
    unsigned tileX = parameters["TileX"].GetUInt();
    unsigned tileZ = parameters["TileZ"].GetUInt();
    unsigned chunkX = tileX * w;
    unsigned chunkZ = tileZ * d;

    VoxelWriter writer;
    writer.SetSize(w, h, d);
    writer.InitializeBuffer(dataPtr);
    writer.Clear(0);

	WorkQueue* queue = global_context_->GetSubsystem<WorkQueue>();
	for (unsigned x = 0; x < WORKLOAD_NUM_X; ++x)
	{
		for (unsigned z = 0; z < WORKLOAD_NUM_Z; ++z)
		{
			SharedPtr<WorkItem> workItem(new WorkItem());
			TerrainWorkload* workload = new TerrainWorkload();
			workload->chunkX = chunkX;
			workload->chunkZ = chunkZ;
			workload->x = x * WORKLOAD_SIZE_X;
			workload->z = z * WORKLOAD_SIZE_Z;
			workload->data = dataPtr;
			workItem->aux_ = workload;
			workItem->priority_ = M_MAX_UNSIGNED;
			workItem->workFunction_ = FillTerrainPerlinWorker;
			queue->AddWorkItem(workItem);
		}
	}
	queue->Complete(M_MAX_UNSIGNED);
}

static unsigned RandomTerrain(void* dest, unsigned size, unsigned position, VariantMap& parameters)
{
    if (position < headerSize)
    {
        unsigned* dataPtr = (unsigned*)dest;
        *dataPtr++ = chunkHeader[position/4];
        return 4;
    }
    else
    {
        unsigned podIndex = (position - headerSize) / dataSize;
        unsigned char* dataPtr = (unsigned char*)dest;
        FillTerrainPerlin(dataPtr, parameters, podIndex);
        return size;
    }
    return 0;
}

#endif