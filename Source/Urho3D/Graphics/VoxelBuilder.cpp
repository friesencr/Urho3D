#include "VoxelBuilder.h"
#include "../Resource/ResourceCache.h"
#include "Texture2DArray.h"
#include "../Resource/Image.h"
#include "Technique.h"
#include "Texture2D.h"
#include "TextureBuffer.h"
#include "Material.h"
#include "../IO/Log.h"
#include "Core/CoreEvents.h"
#include "../Container/ArrayPtr.h"
#include "../IO/Compression.h"
#include "../Math/Plane.h"
#include "../Core/Profiler.h"
#include "../Container/Sort.h"

#include "../DebugNew.h"

#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE  1
#include <STB/stb_voxel_render.h>

static const float URHO3D_RSQRT2 = 0.7071067811865f;
static const float URHO3D_RSQRT3 = 0.5773502691896f;

static float URHO3D_default_normals[32][3] =
{
	{ 1, 0, 0 },  // east
	{ 0, 1, 0 },  // north
	{ -1, 0, 0 }, // west
	{ 0, -1, 0 }, // south
	{ 0, 0, 1 },  // up
	{ 0, 0, -1 }, // down
	{ URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // east & up
	{ URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // east & down

	{ URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // east & up
	{ 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
	{ -URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // west & up
	{ 0, -URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
	{ URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // ne & up
	{ URHO3D_RSQRT3, URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // ne & down
	{ 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
	{ 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down

	{ URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // east & down
	{ 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down
	{ -URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // west & down
	{ 0, -URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NW & up
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // NW & down
	{ -URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // west & up
	{ -URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // west & down

	{ URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NE & up crossed
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NW & up crossed
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SW & up crossed
	{ URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SE & up crossed
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SW & up
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // SW & up
	{ 0, -URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
	{ 0, -URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
};

static float URHO3D_default_ambient[3][4] =
{
	{ 0.0f, 1.0f, 0.0f, 0.0f }, // reversed lighting direction
	{ 0.1f, 0.1f, 0.1f, 0.0f }, // directional color
	{ 0.0f, 0.0f, 0.0f, 0.0f }, // constant color
};

static float URHO3D_default_texscale[128][4] =
{
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
};

static float URHO3D_default_texgen[64][3] =
{
	{ 0, 1, 0 }, { 0, 0, 1 }, { 0, -1, 0 }, { 0, 0, -1 },
	{ -1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 0, -1 },
	{ 0, -1, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 0, -1 },
	{ 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 },

	{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 },
	{ -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 },
	{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 },
	{ -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 },

	{ 0, 0, -1 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, -1, 0 },
	{ 0, 0, -1 }, { -1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 },
	{ 0, 0, -1 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 1, 0 },
	{ 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 },

	{ 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
	{ 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 },
	{ 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
	{ 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 },
};

static float URHO3D_default_palette[64][4];

static unsigned char URHO3D_default_palette_compact[64][3] =
{
	{ 255, 255, 255 }, { 238, 238, 238 }, { 221, 221, 221 }, { 204, 204, 204 },
	{ 187, 187, 187 }, { 170, 170, 170 }, { 153, 153, 153 }, { 136, 136, 136 },
	{ 119, 119, 119 }, { 102, 102, 102 }, { 85, 85, 85 }, { 68, 68, 68 },
	{ 51, 51, 51 }, { 34, 34, 34 }, { 17, 17, 17 }, { 0, 0, 0 },
	{ 255, 240, 240 }, { 255, 220, 220 }, { 255, 160, 160 }, { 255, 32, 32 },
	{ 200, 120, 160 }, { 200, 60, 150 }, { 220, 100, 130 }, { 255, 0, 128 },
	{ 240, 240, 255 }, { 220, 220, 255 }, { 160, 160, 255 }, { 32, 32, 255 },
	{ 120, 160, 200 }, { 60, 150, 200 }, { 100, 130, 220 }, { 0, 128, 255 },
	{ 240, 255, 240 }, { 220, 255, 220 }, { 160, 255, 160 }, { 32, 255, 32 },
	{ 160, 200, 120 }, { 150, 200, 60 }, { 130, 220, 100 }, { 128, 255, 0 },
	{ 255, 255, 240 }, { 255, 255, 220 }, { 220, 220, 180 }, { 255, 255, 32 },
	{ 200, 160, 120 }, { 200, 150, 60 }, { 220, 130, 100 }, { 255, 128, 0 },
	{ 255, 240, 255 }, { 255, 220, 255 }, { 220, 180, 220 }, { 255, 32, 255 },
	{ 160, 120, 200 }, { 150, 60, 200 }, { 130, 100, 220 }, { 128, 0, 255 },
	{ 240, 255, 255 }, { 220, 255, 255 }, { 180, 220, 220 }, { 32, 255, 255 },
	{ 120, 200, 160 }, { 60, 200, 150 }, { 100, 220, 130 }, { 0, 255, 128 },
};

namespace Urho3D
{

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context),
    compatibilityMode(false),
    sharedIndexBuffer_(0)
{
    transform_.Resize(3);
    normals_.Resize(32);
    ambientTable_.Resize(3);
    texscaleTable_.Resize(64);
    texgenTable_.Resize(64);
    colorTable_.Resize(64);


    // copy transforms
    transform_[0] = Vector3(1.0, 0.5f, 1.0);
    transform_[1] = Vector3(0.0, 0.0, 0.0);
    transform_[2] = Vector3((float)(0 & 255), (float)(0 & 255), (float)(0 & 255));

    // copy normal table
    for (unsigned i = 0; i < 32; ++i)
        normals_[i] = Vector3(URHO3D_default_normals[i]);

    // ambient color table
    for (unsigned i = 0; i < 3; ++i)
        ambientTable_[i] = Vector3(URHO3D_default_ambient[i]);

    // texscale table
    for (unsigned i = 0; i < 64; ++i)
        texscaleTable_[i] = Vector4(URHO3D_default_texscale[i]);

    // texgen table
    for (unsigned i = 0; i < 64; ++i)
        texgenTable_[i] = Vector3(URHO3D_default_texgen[i]);

    // color table
    for (int i = 0; i < 64; ++i) {
        URHO3D_default_palette[i][0] = URHO3D_default_palette_compact[i][0] / 255.0f;
        URHO3D_default_palette[i][1] = URHO3D_default_palette_compact[i][1] / 255.0f;
        URHO3D_default_palette[i][2] = URHO3D_default_palette_compact[i][2] / 255.0f;
        URHO3D_default_palette[i][3] = 1.0f;
    }

    for (unsigned i = 0; i < 64; ++i)
        colorTable_[i] = Vector4(URHO3D_default_palette[i]);

    AllocateWorkerBuffers();
}

VoxelBuilder::~VoxelBuilder()
{
    for (unsigned i = 0; i < jobs_.Size(); ++i)
        delete jobs_[i];
}

void VoxelBuilder::AllocateWorkerBuffers()
{
    //WorkQueue* queue = GetSubsystem<WorkQueue>();

    // the workers will build a chunk and then convert it to urho vertex buffers, while the vertex buffer is being built it will start more chunks
    // but stall if the mesh building gets too far ahead of the vertex converter this is based on chunk size and number of cpu threads
    unsigned numSlots = 1; // (unsigned)Min((float)numChunks, Max(1.0f, ceil((float)queue->GetNumThreads() / (float)VOXEL_MAX_NUM_WORKERS_PER_CHUNK)) * 2.0f);

    if (numSlots == slots_.Size())
        return;

    slots_.Resize(numSlots);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        for (unsigned a = 0; a < VOXEL_MAX_WORKERS; ++a)
            stbvox_init_mesh_maker(&slots_[i].meshMakers[a]);

        slots_[i].job = 0;
        slots_[i].workloads.Resize(VOXEL_MAX_WORKERS);
        FreeWorkSlot(&slots_[i]);
    }
}

void VoxelBuilder::FreeWorkerBuffers()
{
    slots_.Clear();
}

bool VoxelBuilder::ResizeIndexBuffer(unsigned numQuads)
{
    if (!(sharedIndexBuffer_.Null() || sharedIndexBuffer_->GetIndexCount() / 6 < numQuads))
        return true;

    unsigned numIndices = numQuads * 6;
    unsigned numVertices = numQuads * 4;

    unsigned indexSize = sizeof(int); // numVertices > M_MAX_UNSIGNED_SHORT ? sizeof(int) : sizeof(short);
    SharedArrayPtr<unsigned char> data(new unsigned char[indexSize * numIndices]);
    unsigned char* dataPtr = data.Get();

    // triangulates the quads
    for (unsigned i = 0; i < numQuads; i++)
    {
        *((unsigned*)dataPtr) = i * 4;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 1;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 2;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 2;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 3;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4;
        dataPtr += indexSize;
    }

    if (sharedIndexBuffer_.Null())
    {
        sharedIndexBuffer_ = new IndexBuffer(context_);
        sharedIndexBuffer_->SetShadowed(true);
    }

    if (!sharedIndexBuffer_->SetSize(numIndices, indexSize == sizeof(unsigned), false))
    {
        LOGERROR("Could not set size of shared voxel index buffer.");
        return false;
    }

    if (!sharedIndexBuffer_->SetData(data.Get()))
    {
        LOGERROR("Could not set data of shared voxel index buffer.");
        return false;
    }
    return true;
}

void VoxelBuilder::CompleteWork(unsigned priority)
{
    PROFILE(VoxelWork);
    while (RunJobs());
}

// Decode to vertex buffer
void VoxelBuilder::DecodeWorkBuffer(VoxelWorkload* workload)
{
    int numQuads = workload->numQuads;
    if (numQuads <= 0)
        return;

    VoxelWorkSlot* slot = &slots_[workload->slot];
    int workloadIndex = workload->workloadIndex;
    //int gpuVertexSize = sizeof(float) * 8; // position(3) / normal(3) / uv1(2)
    //int cpuVertexSize = sizeof(float) * 3; // position(3)
    int numVertices = numQuads * 4; // 4 verts in a quad

    //workload->gpuData.Resize(numVertices * gpuVertexSize);
    //workload->cpuData.Resize(numVertices * cpuVertexSize);
    //float* gpu = (float*)&workload->gpuData.Front();
    //float* cpu = (float*)&workload->cpuData.Front();
    unsigned int* workData = (unsigned int*)slot->workVertexBuffers[workloadIndex];

    for (int i = 0; i < numVertices; ++i)
    {
        unsigned int v1 = *workData++;
        // unsigned int v2 = *workData++;

        Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0f, (float)((v1 >> 7u) & 127u));
        //float amb_occ = (float)((v1 >> 23u) & 63u) / 63.0;
        //unsigned char tex1 = v2 & 0xFF;
        //unsigned char tex2 = (v2 >> 8) & 0xFF;
        //unsigned char color = (v2 >> 16) & 0xFF;
        // unsigned char face_info = (v2 >> 24) & 0xFF;
        // unsigned char normal = face_info >> 2u;
        //unsigned char face_rot = face_info & 2u;
        // Vector3 normalf(URHO3D_default_normals[normal]);
        // normalf = Vector3(normalf.x_, normalf.z_, normalf.y_);

        workload->box.Merge(position);

        //*gpu++ = position.x_;
        //*gpu++ = position.y_;
        //*gpu++ = position.z_;
        //*cpu++ = position.x_;
        //*cpu++ = position.y_;
        //*cpu++ = position.z_;

        //*gpu++ = normalf.x_;
        //*gpu++ = normalf.y_;
        //*gpu++ = normalf.z_;

        //if (i % 4 == 0)
        //{
        //    *gpu++ = 0.0;
        //    *gpu++ = 0.0;
        //}
        //else if (i % 4 == 1)
        //{
        //    *gpu++ = 1.0;
        //    *gpu++ = 0.0;
        //}
        //else if (i % 4 == 2)
        //{
        //    *gpu++ = 1.0;
        //    *gpu++ = 1.0;
        //}
        //else if (i % 4 == 3)
        //{
        //    *gpu++ = 0.0;
        //    *gpu++ = 1.0;
        //}
    }

    {
        MutexLock lock(slot->dataMutex);
        slot->numQuads += numQuads;
        slot->box.Merge(workload->box);
    }
}

void BuildWorkloadHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
    workload->builder->BuildWorkload(workload);
}

VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, bool async)
{
    return BuildVoxelChunk(chunk, voxelMap, 0, 0, 0, 0, async);
}

VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, VoxelMap* northMap, 
    VoxelMap* southMap, VoxelMap* eastMap, VoxelMap* westMap, bool async)
{
    if (!chunk)
        return 0;

    if (!voxelMap)
        return 0;

    unsigned char width = voxelMap->width_;
    unsigned char depth = voxelMap->depth_;
    unsigned char height = voxelMap->height_;

    if (!(width > 0 && depth > 0 && height > 0))
        return 0;

    VoxelJob* job = CreateJob(chunk);
    job->voxelMap = voxelMap;
    job->northMap = northMap;
    job->southMap = southMap;
    job->eastMap = eastMap;
    job->westMap = westMap;
	RunJobs(async);
	if (!async)
		CompleteWork();

    return job;
}

bool VoxelBuilder::RunJobs(bool async)
{
	for (unsigned i = 0; i < slots_.Size(); ++i)
	{
		if (jobs_.Size() > 0)
		{
			int slotId = GetFreeWorkSlot();
			if (slotId != -1 && ReserveWorkSlot(slotId))
			{
				unsigned count = jobs_.Size();
				VoxelJob* job = jobs_[0];
				VoxelWorkSlot* slot = &slots_[slotId];
				job->slot = slotId;
				slot->job = job;
				jobs_.Erase(0);
				ProcessJob(job);
				break;
			}
		}
	}

	if (!async)
	{
		GetSubsystem<WorkQueue>()->Complete(0);

		for (unsigned i = 0; i < slots_.Size(); ++i)
		{
			VoxelWorkSlot* slot = &slots_[i];
			bool process = false;
			{
				MutexLock lock(slot->workMutex);
				process = !slot->free && slot->upload;
				if (process)
					slot->upload = false;
			}
			if (process)
				ProcessSlot(slot);
		}
	}

    return jobs_.Size() > 0;
}


VoxelJob* VoxelBuilder::CreateJob(VoxelChunk* chunk)
{
    VoxelJob* job = new VoxelJob();
    job->chunk = chunk;
    chunk->voxelJob_ = job;
    job->slot = 0;
    jobs_.Push(job);
    return job;
}

void VoxelBuilder::ProcessJob(VoxelJob* job)
{
    WorkQueue* workQueue = GetSubsystem<WorkQueue>();
    VoxelWorkSlot* slot = &slots_[job->slot];
    VoxelMap* voxelMap = job->voxelMap;
    VoxelChunk* chunk = job->chunk;

    // Copy adjacent data
    {
        const int MAP_NORTH = 0;
        const int MAP_SOUTH = 1;
        const int MAP_EAST = 2;
        const int MAP_WEST = 3;
        VoxelMap* maps[4] = { job->northMap, job->southMap, job->eastMap, job->westMap };
		for (unsigned m = 0; m < 4; ++m)
		{
			VoxelMap* srcMap = maps[m];

			if (!srcMap)
				continue;

			PODVector<unsigned char>* srcDatas[VoxelMap::NUM_BASIC_STREAMS];
			PODVector<unsigned char>* destDatas[VoxelMap::NUM_BASIC_STREAMS];
			srcMap->GetBasicDataArrays(srcDatas);
			voxelMap->GetBasicDataArrays(destDatas);

			for (unsigned i = 0; i < VoxelMap::NUM_BASIC_STREAMS; ++i)
			{
				if (!voxelMap->dataMask_ & (1 << i))
					continue;

				if (!srcMap->dataMask_ & (1 << i))
					continue;

				unsigned char* src = 0;
				unsigned char* dst = 0;

				if (srcDatas[i]->Size() == voxelMap->size_)
				{
					if (destDatas[i]->Size() == voxelMap->size_)
					{
						src = &srcDatas[i]->Front();
						dst = &destDatas[i]->Front();
					}
				}

				if (!(src && dst))
					continue;

				unsigned char* srcPtr = src;
				unsigned char* dstPtr = dst;

				if (m == MAP_NORTH)
				{
					for (unsigned x = 0; x < voxelMap->width_; ++x)
						for (unsigned y = 0; y < voxelMap->height_; ++y)
						{
							dstPtr[voxelMap->GetIndex(x, y, voxelMap->depth_)] = srcPtr[srcMap->GetIndex(x, y, 0)];
							dstPtr[voxelMap->GetIndex(x, y, voxelMap->depth_ + 1)] = srcPtr[srcMap->GetIndex(x, y, 1)];
						}
				}
				else if (m == MAP_SOUTH)
				{
					for (unsigned x = 0; x < voxelMap->width_; ++x)
						for (unsigned y = 0; y < voxelMap->height_; ++y)
						{
							dstPtr[voxelMap->GetIndex(x, y, -1)] = srcPtr[srcMap->GetIndex(x, y, srcMap->depth_ - 1)];
							dstPtr[voxelMap->GetIndex(x, y, -2)] = srcPtr[srcMap->GetIndex(x, y, srcMap->depth_ - 2)];
						}
				}
				else if (m == MAP_EAST)
				{
					for (unsigned z = 0; z < voxelMap->depth_; ++z)
						for (unsigned y = 0; y < voxelMap->height_; ++y)
						{
							dstPtr[voxelMap->GetIndex(voxelMap->width_, y, z)] = srcPtr[srcMap->GetIndex(0, y, z)];
							dstPtr[voxelMap->GetIndex(voxelMap->width_ + 1, y, z)] = srcPtr[srcMap->GetIndex(1, y, z)];
						}
				}
				else if (m == MAP_WEST)
				{
					for (unsigned z = 0; z < voxelMap->depth_; ++z)
						for (unsigned y = 0; y < voxelMap->height_; ++y)
						{
							dstPtr[voxelMap->GetIndex(-1, y, z)] = srcPtr[srcMap->GetIndex(srcMap->width_ - 1, y, z)];
							dstPtr[voxelMap->GetIndex(-2, y, z)] = srcPtr[srcMap->GetIndex(srcMap->width_ - 2, y, z)];
						}
				}
			}
		}
    }

#if 0
    if (voxelMap->GetVoxelProcessors().Size() > 0)
    {

        unsigned char* data[7] = {
            &voxelMap->blocktype.Front(), &voxelMap->color.Front(), &voxelMap->geometry.Front(),
            &voxelMap->vHeight.Front(), &voxelMap->lighting.Front(), &voxelMap->rotate.Front(),
            &voxelMap->tex2.Front()
        };

        VoxelWriter* writers[7] = {
            &slot->processorWriters.blocktype,
            &slot->processorWriters.color,
            &slot->processorWriters.geometry,
            &slot->processorWriters.vHeight,
            &slot->processorWriters.lighting,
            &slot->processorWriters.rotate,
            &slot->processorWriters.tex2
        };

        for (unsigned i = 0; i < 7; ++i)
        {
            unsigned test = (1 << i);
            if (!((1 << i) & voxelMap->processorDataMask_))
                continue;

            unsigned char* source = data[i];
            unsigned char* dest = slot->workProcessorBuffers[i];
            if (voxelMap->dataMask_ & (1 << i))
                memcpy(dest, source, voxelMap->size_);
            else
                memset(dest, 0, voxelMap->size_);
            
            writers[i]->InitializeBuffer(slot->workProcessorBuffers[i]);
            writers[i]->SetSize(voxelMap->width_, voxelMap->height_, voxelMap->depth_);
        }

        Vector<VoxelProcessorFunc> processors = voxelMap->GetVoxelProcessors();
        for (unsigned p = 0; p < processors.Size(); ++p)
        {
            processors[p](chunk, voxelMap, slot->processorWriters);
        }
    }
#endif

    unsigned char workloadsX = (unsigned char)ceil((float)voxelMap->width_ / (float)VOXEL_WORKER_SIZE_X);
    unsigned char workloadsY = (unsigned char)ceil((float)voxelMap->height_ / (float)VOXEL_WORKER_SIZE_Y);
    unsigned char workloadsZ = (unsigned char)ceil((float)voxelMap->depth_ / (float)VOXEL_WORKER_SIZE_Z);
    unsigned index = 0;
    slot->numWorkloads = workloadsX * workloadsY * workloadsZ;
    slot->workCounter = slot->numWorkloads;
    for (unsigned char x = 0; x < workloadsX; ++x)
    {
        for (unsigned char z = 0; z < workloadsZ; ++z)
        {
            for (unsigned char y = 0; y < workloadsY; ++y)
            {
                VoxelWorkload* workload = &slot->workloads[index];
                workload->builder = this;
                workload->slot = job->slot;
                workload->index[0] = x;
                workload->index[1] = y;
                workload->index[2] = z;
                workload->start[0] = x * VOXEL_WORKER_SIZE_X;
                workload->start[1] = 0;
                workload->start[2] = z * VOXEL_WORKER_SIZE_Z;
                workload->end[0] = Min((int)voxelMap->width_, (x + 1) * VOXEL_WORKER_SIZE_X);
                workload->end[1] = Min((int)voxelMap->height_, (y + 1) * VOXEL_WORKER_SIZE_Y);
                workload->end[2] = Min((int)voxelMap->depth_, (z + 1) * VOXEL_WORKER_SIZE_Z);
                workload->workloadIndex = index;
                workload->numQuads = 0;
                index++;

                SharedPtr<WorkItem> workItem(new WorkItem());
                workItem->priority_ = 0;
                workItem->aux_ = workload;
                workItem->start_ = slot;
                workItem->workFunction_ = BuildWorkloadHandler;
                workload->workItem = workItem;
                workQueue->AddWorkItem(workItem);
            }
        }
    }
}

void VoxelBuilder::BuildWorkload(VoxelWorkload* workload)
{
    VoxelWorkSlot* slot = &slots_[workload->slot];
    bool success = BuildMesh(workload);

    if (success)
        DecodeWorkBuffer(workload);
    else
    {
        MutexLock lock(slot->workMutex);
        slot->failed = true;
    }
    DecrementWorkSlot(slot);
}

void VoxelBuilder::AbortSlot(VoxelWorkSlot* slot)
{

}

void VoxelBuilder::CancelJob(VoxelJob* job)
{
    if (jobs_.Remove(job))
        delete job;
}

bool VoxelBuilder::BuildMesh(VoxelWorkload* workload)
{
    VoxelWorkSlot* slot = &slots_[workload->slot];
    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    VoxelMap* voxelMap = job->voxelMap;
    VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;

    stbvox_mesh_maker* mm = &slot->meshMakers[workload->workloadIndex];
    stbvox_set_input_stride(mm, voxelMap->GetStrideX(), voxelMap->GetStrideZ());

    stbvox_input_description *stbvox_map;
    stbvox_map = stbvox_get_input_description(mm);

    if (voxelBlocktypeMap)
    {
        stbvox_map->block_tex1 = voxelBlocktypeMap->blockTex1.Empty() ? 0 : &voxelBlocktypeMap->blockTex1.Front();
        stbvox_map->block_tex1_face = voxelBlocktypeMap->blockTex1Face.Empty() ? 0 : &voxelBlocktypeMap->blockTex1Face.Front();
        stbvox_map->block_tex2 = voxelBlocktypeMap->blockTex2.Empty() ? 0 : &voxelBlocktypeMap->blockTex2.Front();
        stbvox_map->block_tex2_face = voxelBlocktypeMap->blockTex2Face.Empty() ? 0 : &voxelBlocktypeMap->blockTex2Face.Front();
        stbvox_map->block_geometry = voxelBlocktypeMap->blockGeometry.Empty() ? 0 : &voxelBlocktypeMap->blockGeometry.Front();
        stbvox_map->block_vheight = voxelBlocktypeMap->blockVHeight.Empty() ? 0 : &voxelBlocktypeMap->blockVHeight.Front();
        stbvox_map->block_color = voxelBlocktypeMap->blockColor.Empty() ? 0 : &voxelBlocktypeMap->blockColor.Front();
    }

    // Set voxel maps for stb voxel
    int zero = voxelMap->GetIndex(0, 0, 0);
	stbvox_map->blocktype = &voxelMap->blocktype[zero];
	//stbvox_map->vheight = voxelMap->GetVHeight()->Empty() ? 0 : &voxelMap->GetVHeight()->Front();
	//stbvox_map->color = voxelMap->GetColor()->Empty() ? 0 : &voxelMap->GetColor()->Front();
	//stbvox_map->geometry = voxelMap->GetGeometry()->Empty() ? 0 : &voxelMap->GetGeometry()->Front();
	//stbvox_map->rotate = voxelMap->GetRotate()->Empty() ? 0 : &voxelMap->GetRotate()->Front();
	//stbvox_map->lighting = voxelMap->GetLighting()->Empty() ? 0 : &voxelMap->GetLighting()->Front();
 //   stbvox_map->tex2 = voxelMap->GetTex2()->Empty() ? 0 : &voxelMap->GetTex2()->Front();

#if 0
    if (voxelMap->GetVoxelProcessors().Size() > 0)
    {
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_BLOCKTYPE)
            stbvox_map->blocktype = &slot->workProcessorBuffers[0][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_COLOR)
            stbvox_map->color = &slot->workProcessorBuffers[1][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_GEOMETRY)
            stbvox_map->geometry = &slot->workProcessorBuffers[2][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_VHEIGHT)
            stbvox_map->vheight = &slot->workProcessorBuffers[3][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_LIGHTING)
            stbvox_map->lighting = &slot->workProcessorBuffers[4][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_ROTATE)
            stbvox_map->rotate = &slot->workProcessorBuffers[5][zero];
        if (voxelMap->processorDataMask_ & VOXEL_BLOCK_TEX2)
            stbvox_map->tex2 = &slot->workProcessorBuffers[6][zero];
    }
#endif

    stbvox_reset_buffers(mm);
    stbvox_set_buffer(mm, 0, 0, slot->workVertexBuffers[workload->workloadIndex], VOXEL_WORKER_VERTEX_BUFFER_SIZE);
    stbvox_set_buffer(mm, 0, 1, slot->workFaceBuffers[workload->workloadIndex], VOXEL_WORKER_FACE_BUFFER_SIZE);

    stbvox_set_default_mesh(mm, 0);

    bool success = true;
    for (unsigned y = 0; y < workload->end[1]; y += 16)
    {
        stbvox_set_input_range(
            mm,
            workload->start[0],
            workload->start[2],
            workload->start[1] + y,
            workload->end[0],
            workload->end[2],
            Min(y + 16, workload->end[1])
        );

        if (stbvox_make_mesh(mm) == 0)
        {
            // TODO handle partial uploads
            LOGERROR("Voxel mesh builder ran out of memory.");
            success = false;
            break;
        }
    }
    workload->numQuads = stbvox_get_quad_count(mm, 0);
    return success;
}

bool VoxelBuilder::UpdateMaterialParameters(Material* material, bool setColors)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Technique* technique = cache->GetResource<Technique>("Techniques/VoxelDiff.xml");
    material->SetTechnique(0, technique);
    material->SetShaderParameter("Transform", transform_);
    material->SetShaderParameter("NormalTable", normals_);
    material->SetShaderParameter("AmbientTable", ambientTable_);
    material->SetShaderParameter("TexScale", texscaleTable_);
    material->SetShaderParameter("TexGen", texgenTable_);
    if (setColors)
        material->SetShaderParameter("ColorTable", colorTable_);
    return true;
}

bool VoxelBuilder::UploadGpuDataCompatibilityMode(VoxelWorkSlot* slot, bool append)
{
    //if (!vb->SetSize(totalVertices, MASK_POSITION | MASK_TEXCOORD1 | MASK_NORMAL, true))
    //    return false;
    return false;
}

bool VoxelBuilder::UploadGpuData(VoxelWorkSlot* slot, bool append)
{
    PROFILE(UploadGpuData);

    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);
    VertexBuffer* vb = chunk->GetVertexBuffer(0);
    IndexBuffer* faceData = chunk->GetFaceData(0);
    chunk->numQuads_[0] = slot->numQuads;
    int totalVertices = slot->numQuads * 4;
    if (totalVertices == 0)
        return true;

    if (!vb->SetSize(totalVertices, MASK_DATA, false))
        return false;

    if (!faceData->SetSize(slot->numQuads, true, false))
        return false;

    // raw vertices
    //SharedArrayPtr<unsigned char> vertexData(new unsigned char[slot->numQuads * 4 * sizeof(unsigned)]);

    //// normal memory
    //SharedArrayPtr<unsigned char> normalData(new unsigned char[slot->numQuads * sizeof(unsigned)]);

    unsigned start = 0;
    for (int i = 0; i < slot->numWorkloads; ++i)
    {
        VoxelWorkload* workload = &slot->workloads[i];
        //memcpy(&vertexData[start*4], slot->workVertexBuffers[workload->workloadIndex], workload->numQuads * sizeof(unsigned) * 4);
        //memcpy(&normalData[copyIndex], slot->workFaceBuffers[workload->workloadIndex], workload->numQuads * sizeof(unsigned));
        //copyIndex += workload->numQuads * sizeof(unsigned);


        if (!vb->SetDataRange(
            slot->workVertexBuffers[workload->workloadIndex], 
            start*4,
            workload->numQuads*4))
        {
            LOGERROR("Error uploading voxel vertex data.");
            return false;
        }

        if (!faceData->SetDataRange( 
                slot->workFaceBuffers[workload->workloadIndex], 
                start,
                workload->numQuads))
        {
            LOGERROR("Error uploading voxel face data.");
            return false;
        }
        start += workload->numQuads;
    }
    Geometry* geo = chunk->GetGeometry(0);
#if 0
    unsigned char* newMeshPtr = 0;
    unsigned reducedSize = SimplifyMesh(slot, vertexData.Get(), normalData.Get(), &newMeshPtr);
    chunk->reducedQuadCount_[0] = reducedSize;
    geo->SetRawVertexData(SharedArrayPtr<unsigned char>(newMeshPtr), sizeof(Vector3), MASK_POSITION);
#else
    //chunk->reducedQuadCount_[0] = slot->numQuads;
    //geo->SetRawVertexData(vertexData, sizeof(unsigned), MASK_DATA);
#endif


    //PODVector<unsigned char> compressed(EstimateCompressBound(chunk->GetVoxelMap()->blocktype.Size()));
    //unsigned size = CompressData(&compressed.Front(), &chunk->GetVoxelMap()->blocktype.Front(), chunk->GetVoxelMap()->blocktype.Size());

    chunk->SetBoundingBox(slot->box);
    Material* material = chunk->GetMaterial(0);

    // face data
    {
        TextureBuffer* faceDataTexture = chunk->GetFaceBuffer(0);
        if (!faceDataTexture->SetSize(0))
        {
            LOGERROR("Error initializing voxel texture buffer");
            return false;
        }
        if (!faceDataTexture->SetData(faceData))
        {
            LOGERROR("Error setting voxel texture buffer data");
            return false;
        }
        material->SetTexture(TU_CUSTOM1, faceDataTexture);
    }

    if (!ResizeIndexBuffer(slot->numQuads))
    {
        LOGERROR("Error resizing shared voxel index buffer");
        return false;
    }
    geo->SetIndexBuffer(sharedIndexBuffer_);
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, slot->numQuads * 6, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }
    return true;
}

void VoxelBuilder::ProcessSlot(VoxelWorkSlot* slot)
{
    if (!slot->failed)
    {
        if (!UploadGpuData(slot))
            LOGERROR("Could not upload voxel data to graphics card.");
        else
            slot->job->chunk->OnVoxelChunkCreated();
    }
    FreeWorkSlot(slot);
}

unsigned VoxelBuilder::DecrementWorkSlot(VoxelWorkSlot* slot)
{
    MutexLock lock(slot->workMutex);
    slot->upload = --slot->workCounter == 0;
    return slot->workCounter;
}

void VoxelBuilder::FreeWorkSlot(VoxelWorkSlot* slot)
{
    MutexLock lock(slotMutex_);
    slot->failed = false;
    slot->workCounter = 0;
    slot->numQuads = 0;
    slot->numWorkloads = 0;
    slot->box = BoundingBox();
    slot->upload = false;
    slot->free = true;

    if (slot->job)
    {
        delete slot->job;
        slot->job = 0;
    }
}

int VoxelBuilder::GetFreeWorkSlot()
{
    MutexLock lock(slotMutex_);
    VoxelWorkSlot* slot = 0;
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (slots_[i].free)
        {
            slot = &slots_[i];
            return i;
        }
    }
    return -1;
}

bool VoxelBuilder::ReserveWorkSlot(unsigned index)
{
    MutexLock lock(slotMutex_);
	if (slots_[index].free)
	{
		slots_[index].free = false;
		return true;
	}
	return false;
}

const unsigned QUAD_VERTEX_MASK = 0x007FFFFF;
// QUAD
// [d,c]
// [a,b]
struct Quad {
    unsigned id;
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
    unsigned char normal;
    bool keep;

    Vector3 AVector() {
        return Vector3 ((float)(a & 127u), (float)((a >> 14u) & 511u) / 2.0f, (float)((a >> 7u) & 127u));
    }
    Vector3 BVector() {
        return Vector3 ((float)(b & 127u), (float)((b >> 14u) & 511u) / 2.0f, (float)((b >> 7u) & 127u));
    }
    Vector3 CVector() {
        return Vector3 ((float)(c & 127u), (float)((c >> 14u) & 511u) / 2.0f, (float)((c >> 7u) & 127u));
    }
    Vector3 DVector() {
        return Vector3 ((float)(d & 127u), (float)((d >> 14u) & 511u) / 2.0f, (float)((d >> 7u) & 127u));
    }

    Vector3 GetNormal() {
        return Vector3(
            URHO3D_default_normals[normal][0],
            URHO3D_default_normals[normal][2],
            URHO3D_default_normals[normal][1]
        );
    }
};

struct MeshSimplifyContext
{
    Quad* quads;
    int* byNormal;
    int byNormalLen;
    int merges;
    bool complete;
    Mutex statusMutex;
};

void SimplifyPlaneHandler(const WorkItem* workItem, unsigned threadIndex)
{
    MeshSimplifyContext* meshCtx = (MeshSimplifyContext*)workItem->aux_;
    Quad* quads = meshCtx->quads;
    int* byNormal = meshCtx->byNormal;
    PODVector<Plane> planes;
    Vector<PODVector<int> > byPlane;

    // bucket all of the quads by plane
    for (unsigned i = 0; i < meshCtx->byNormalLen; ++i)
    {
        Quad* quad = &quads[byNormal[i]];
        Vector3 point = quad->AVector();
        bool makePlane = true;
        for (unsigned p = 0; p < planes.Size(); ++p)
        {
            Plane plane = planes[p];
            float d = Abs(plane.Distance(point));
            if (d < M_EPSILON)
            {
                byPlane[p].Push(quad->id);
                makePlane = false;
                break;
            }
        }

        if (makePlane)
        {
            planes.Push(Plane(quad->GetNormal(), quad->AVector()));
            byPlane.Resize(planes.Size());
            byPlane[planes.Size() - 1].Push(quad->id);
        }
    }

    for (unsigned p = 0; p < planes.Size(); ++p)
    {
        int* planeIds = &byPlane[p].Front();
        for (;;)
        {
            bool aMerge = false;
            for (unsigned q1 = 0; q1 < byPlane[p].Size(); ++q1)
            {
                Quad* quad1 = &quads[planeIds[q1]];
                if (!quad1->keep)
                    continue;

                unsigned a1 = quad1->a & QUAD_VERTEX_MASK;
                unsigned b1 = quad1->b & QUAD_VERTEX_MASK;
                unsigned c1 = quad1->c & QUAD_VERTEX_MASK;
                unsigned d1 = quad1->d & QUAD_VERTEX_MASK;
                //Vector3 a1Vec = quad1->AVector();
                //Vector3 b1Vec = quad1->BVector();
                //Vector3 c1Vec = quad1->CVector();
                //Vector3 d1Vec = quad1->DVector();

                for (unsigned q2 = 0; q2 < byPlane[p].Size(); ++q2)
                {
                    Quad* quad2 = &quads[planeIds[q2]];
                    unsigned a2 = quad2->a & QUAD_VERTEX_MASK;
                    unsigned b2 = quad2->b & QUAD_VERTEX_MASK;
                    unsigned c2 = quad2->c & QUAD_VERTEX_MASK;
                    unsigned d2 = quad2->d & QUAD_VERTEX_MASK;
                    //Vector3 a2Vec = quad2->AVector();
                    //Vector3 b2Vec = quad2->BVector();
                    //Vector3 c2Vec = quad2->CVector();
                    //Vector3 d2Vec = quad2->DVector();

                    if (!quad2->keep || quad1->id == quad2->id)
                        continue;

                    //Vector3 a2Vec = quad2->AVector();
                    //float d = (a2Vec - a1Vec).Length();
                    if ((a2 == d1) && (b2 == c1)) // up
                    {
                        quad1->d = quad2->d;
                        quad1->c = quad2->c;
                        quad2->keep = false;
                        aMerge = true;
                        meshCtx->merges--;
                    }
                    else if ((d2 == c1) && (a2 == b1)) // right
                    {
                        quad1->c = quad2->c;
                        quad1->b = quad2->b;
                        quad2->keep = false;
                        aMerge = true;
                        meshCtx->merges--;
                    }
                    else if ((c2 == b1) && (d2 == a1)) // down
                    {
                        quad1->b = quad2->b;
                        quad1->a = quad2->a;
                        quad2->keep = false;
                        aMerge = true;
                        meshCtx->merges--;
                    }
                    else if ((b2 == a1) && (c2 == d1)) // left
                    {
                        quad1->a = quad2->a;
                        quad1->d = quad2->d;
                        quad2->keep = false;
                        aMerge = true;
                        meshCtx->merges--;
                    }
                }
            }
            if (!aMerge)
                break;
        }
    }
    {
        MutexLock lock(meshCtx->statusMutex);
        meshCtx->complete = true;
    }
}

unsigned VoxelBuilder::SimplifyMesh(VoxelWorkSlot* slot, unsigned char* vertices, unsigned char* normals, unsigned char** newMesh)
{
    PROFILE(SimplifyMesh);
    int minY = (int)slot->box.min_.y_;
    int maxY = (int)slot->box.max_.y_;

    SharedArrayPtr<Quad> quads(new Quad[slot->numQuads]);
    Vector<PODVector<int> > normalLookup(32);

    unsigned* vertexPtr = (unsigned*)vertices;
    unsigned* dataPtr = (unsigned*)normals;

    for (unsigned i = 0; i < slot->numQuads; ++i)
    {
        quads[i].id = i;
        quads[i].a = *vertexPtr++;
        quads[i].b = *vertexPtr++;
        quads[i].c = *vertexPtr++;
        quads[i].d = *vertexPtr++;
        quads[i].normal = (unsigned char)((*dataPtr++ >> 24) & 0xFF) >> 2u;
        quads[i].keep = true;

        // cache by normal to get away from n^3 and boundry multithreading
        normalLookup[normals[quads[i].normal]].Push(i);
    }

    MeshSimplifyContext meshJobs[32];
    WorkQueue* workQueue = GetSubsystem<WorkQueue>();
    for (unsigned n = 0; n < normalLookup.Size(); ++n)
    {
        if (normalLookup[n].Size() == 0)
        {
            meshJobs[n].merges = 0;
            meshJobs[n].complete = true;
        }
        else
        {
            meshJobs[n].complete = false;
            meshJobs[n].quads = quads.Get();
            meshJobs[n].byNormal = &normalLookup[n].Front();
            meshJobs[n].byNormalLen = normalLookup[n].Size();
            meshJobs[n].merges = normalLookup[n].Size();
            SharedPtr<WorkItem> workItem(new WorkItem());
            workItem->priority_ = 0;
            workItem->aux_ = &meshJobs[n];
            workItem->workFunction_ = SimplifyPlaneHandler;
            workQueue->AddWorkItem(workItem);
        }
    }

    while (true)
    {
        bool complete = true;
        for (unsigned i = 0; i < 32; ++i)
        {
            MutexLock lock(meshJobs[i].statusMutex);
            if (meshJobs[i].complete == false)
            {
                complete = false;
                break;
            }
        }
        if (complete)
            break;
        Time::Sleep(0);
    }

    int totalMerges = 0;
    for (unsigned i = 0; i < 32; ++i)
        totalMerges += meshJobs[i].merges;

    unsigned char* newMeshData = new unsigned char[totalMerges * 4 * sizeof(Vector3)];
    Vector3* facePtr = (Vector3*)newMeshData;
    for (unsigned i = 0; i < slot->numQuads; ++i)
    {
        Quad* quad = &quads[i];
        if (quad->keep)
        {
            *facePtr++ = quad->AVector();
            *facePtr++ = quad->BVector();
            *facePtr++ = quad->CVector();
            *facePtr++ = quad->DVector();
        }
    }
    *newMesh = newMeshData;
    return totalMerges;
}

}
