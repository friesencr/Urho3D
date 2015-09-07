#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../Resource/ResourceCache.h"

#include "VoxelWriter.h"
#include "VoxelBuilder.h"

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

struct VoxelBuildSlot
{
    Vector<VoxelWorkload> workloads;
    VoxelWriter writer;
    VoxelJob* job;
    stbvox_mesh_maker meshMakers[VOXEL_MAX_WORKERS];
    unsigned char workVertexBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_VERTEX_BUFFER_SIZE];
    unsigned char workFaceBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_FACE_BUFFER_SIZE];
    int numQuads;
    bool failed;
    bool free;
    int workCounter;
    int numWorkloads;
    BoundingBox box;
    Mutex workMutex;
    Mutex dataMutex;
};

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context),
    sharedIndexBuffer_(0)
{
    transform_.Resize(3);
    normals_.Resize(32);
    ambientTable_.Resize(3);
    texscaleTable_.Resize(64);
    texgenTable_.Resize(64);
    defaultColorTable_.Resize(64);

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
        defaultColorTable_[i] = Vector4(URHO3D_default_palette[i]);
}

VoxelBuilder::~VoxelBuilder()
{
    for (unsigned i = 0; i < buildJobs_.Size(); ++i)
        delete buildJobs_[i];
}

void VoxelBuilder::AllocateWorkerBuffers()
{
    unsigned numSlots = 2;
    if (numSlots == slots_.Size())
        return;

    slots_.Resize(numSlots);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        for (unsigned a = 0; a < VOXEL_MAX_WORKERS; ++a)
            stbvox_init_mesh_maker(&slots_[i].meshMakers[a]);

        slots_[i].job = 0;
        slots_[i].workloads.Resize(VOXEL_MAX_WORKERS);
        FreeBuildSlot(&slots_[i]);
    }
}

void VoxelBuilder::RegisterProcessor(String name, VoxelProcessorFunc func)
{
    processors_[name] = func;
}

bool VoxelBuilder::UnregisterProcessor(String name)
{
    return processors_.Erase(name);
}

void VoxelBuilder::FreeWorkerBuffers()
{
    slots_.Clear();
}


void VoxelBuilder::CompleteWork(unsigned priority)
{
    //PROFILE(VoxelWork);
    while (RunJobs() || UploadCompletedWork() || IsBuilding())
        Time::Sleep(0);
}

// Decode to vertex buffer
void VoxelBuilder::DecodeWorkBuffer(VoxelWorkload* workload)
{
    int numQuads = workload->numQuads;

    VoxelBuildSlot* slot = &slots_[workload->slot];
    VoxelChunk* chunk = slot->job->chunk;
    VoxelMesh& mesh = chunk->GetVoxelMesh(0);

    unsigned workloadIndex = workload->workloadIndex;
    unsigned numVertices = numQuads * 4; // 4 verts in a quad
    BoundingBox box;
    {
        // vertex data
        unsigned* sourceVertexData = (unsigned int*)slot->workVertexBuffers[workloadIndex];
        mesh.rawVertexData_[workloadIndex].Resize(numVertices * sizeof(unsigned));
        unsigned* chunkVertexData = (unsigned*)&mesh.rawVertexData_[workloadIndex].Front();
        for (int i = 0; i < numVertices; ++i)
        {
            unsigned v1 = sourceVertexData[i];
            chunkVertexData[i] = v1;
            Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0f, (float)((v1 >> 7u) & 127u));
            box.Merge(position);
        }
    }

    {
        // face data
        unsigned* sourceFaceData = (unsigned*)slot->workFaceBuffers[workloadIndex];
        mesh.rawFaceData_[workloadIndex].Resize(numQuads * sizeof(unsigned));
        unsigned* chunkFaceData = (unsigned*)&mesh.rawFaceData_[workloadIndex].Front();
        for (int i = 0; i < numQuads; ++i)
            chunkFaceData[i] = sourceFaceData[i];
    }

    {
        MutexLock lock(slot->dataMutex);
        slot->numQuads += numQuads;
        slot->box.Merge(box);
    }
}

void BuildWorkloadHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
    workload->builder->BuildWorkload(workload);
}

VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, bool async)
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

    if (!slots_.Size())
        AllocateWorkerBuffers();

    VoxelJob* job = CreateJob(chunk);
    job->voxelMap = voxelMap;
	RunJobs();
	if (!async)
		CompleteWork();

    return job;
}

bool VoxelBuilder::RunJobs()
{
	for (unsigned i = 0; i < slots_.Size(); ++i)
	{
		if (buildJobs_.Size() > 0)
		{
			int slotId = GetFreeBuildSlot();
			if (slotId != -1)
			{
				unsigned count = buildJobs_.Size();
				VoxelJob* job = buildJobs_[0];
				VoxelBuildSlot* slot = &slots_[slotId];
				job->slot = slotId;
				slot->job = job;
				buildJobs_.Erase(0);
				ProcessJob(job);
				break;
			}
		}
	}
    return buildJobs_.Size() > 0;
}

bool VoxelBuilder::UploadCompletedWork()
{
    MutexLock lock(uploadJobMutex_);
    for (;;)
    {
        if (!uploadJobs_.Size())
            return false;

        VoxelJob* job = uploadJobs_.Back();
        uploadJobs_.Pop();
        if (!UploadGpuData(job))
            LOGERROR("Failed to upload data to gpu");
        else
            job->chunk->OnVoxelChunkCreated();

        delete job;
    }
    return false;
}

VoxelJob* VoxelBuilder::CreateJob(VoxelChunk* chunk)
{
    VoxelJob* job = new VoxelJob();
    job->chunk = chunk;
    chunk->voxelJob_ = job;
    job->slot = 0;
    buildJobs_.Push(job);
    return job;
}

void VoxelBuilder::ProcessJob(VoxelJob* job)
{
    PROFILE(BuildChunk);
    WorkQueue* workQueue = GetSubsystem<WorkQueue>();
    VoxelBuildSlot* slot = &slots_[job->slot];
    VoxelMap* voxelMap = job->voxelMap;
    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

#if 1
    if (voxelMap->GetVoxelProcessors().Size() > 0 && voxelMap->processorDataMask_)
    {
		PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
		voxelMap->GetBasicDataArrays(datas);

		PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
		slot->writer.GetBasicDataArrays(writerDatas);
        slot->writer.SetDataMask(voxelMap->processorDataMask_);
        slot->writer.SetPadding(voxelMap->GetPadding());
        slot->writer.SetSize(voxelMap->width_, voxelMap->height_, voxelMap->depth_);

        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (!((1 << i) & voxelMap->processorDataMask_))
                continue;

            //unsigned char* source = &datas[i]->Front();
            unsigned char* dest = &writerDatas[i]->Front();
            memset(dest, 0, voxelMap->size_);
        }
    }
#endif

    int workloadsX = (int)ceil((float)voxelMap->width_ / (float)VOXEL_WORKER_SIZE_X);
    int workloadsY = (int)ceil((float)voxelMap->height_ / (float)VOXEL_WORKER_SIZE_Y);
    int workloadsZ = (int)ceil((float)voxelMap->depth_ / (float)VOXEL_WORKER_SIZE_Z);
    unsigned index = 0;
    slot->numWorkloads = workloadsX * workloadsY * workloadsZ;
    slot->workCounter = slot->numWorkloads;
    for (int x = 0; x < workloadsX; ++x)
    {
        for (int z = 0; z < workloadsZ; ++z)
        {
            for (int y = 0; y < workloadsY; ++y)
            {
                VoxelWorkload* workload = &slot->workloads[index];
                workload->builder = this;
                workload->slot = job->slot;
                workload->range.positionX = 0;
                workload->range.positionY = 0;
                workload->range.positionZ = 0;
                workload->range.lengthX = voxelMap->GetWidth();
                workload->range.lengthY = voxelMap->GetHeight();
                workload->range.lengthZ = voxelMap->GetDepth();
                workload->range.startX = x * VOXEL_WORKER_SIZE_X;
                workload->range.startY = y * VOXEL_WORKER_SIZE_Y;
                workload->range.startZ = z * VOXEL_WORKER_SIZE_Z;
                workload->range.endX = Min((int)voxelMap->width_, (x + 1) * VOXEL_WORKER_SIZE_X);
                workload->range.endY = Min((int)voxelMap->height_, (y + 1) * VOXEL_WORKER_SIZE_Y);
                workload->range.endZ = Min((int)voxelMap->depth_, (z + 1) * VOXEL_WORKER_SIZE_Z);
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
    VoxelBuildSlot* slot = &slots_[workload->slot];
    bool success = BuildMesh(workload);

    if (success)
        DecodeWorkBuffer(workload);
    else
    {
        MutexLock lock(slot->workMutex);
        slot->failed = true;
    }

    if (DecrementBuildSlot(slot) == 0)
        PrepareCompletedWork(slot);
}

void VoxelBuilder::PrepareCompletedWork(VoxelBuildSlot* slot)
{
    if (slot->failed)
    {
        LOGERROR("Voxel build failed.");
    }
    else
    {
        VoxelChunk* chunk = slot->job->chunk;
        VoxelMesh& mesh = chunk->GetVoxelMesh(0);
        mesh.numQuads_ = slot->numQuads;
        chunk->SetBoundingBox(slot->box);

        MutexLock lock(uploadJobMutex_);
        uploadJobs_.Push(slot->job);
    }
    FreeBuildSlot(slot);
}

void VoxelBuilder::CancelJob(VoxelJob* job)
{
    if (buildJobs_.Remove(job))
        delete job;
}

bool VoxelBuilder::BuildMesh(VoxelWorkload* workload)
{
    VoxelBuildSlot* slot = &slots_[workload->slot];
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

    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    voxelMap->GetBasicDataArrays(datas);

    unsigned char** stb_data[VOXEL_DATA_NUM_BASIC_STREAMS] = {
        &stbvox_map->blocktype,
        &stbvox_map->color2,
        &stbvox_map->color2_facemask,
        &stbvox_map->color3,
        &stbvox_map->color3_facemask,
        &stbvox_map->color,
        &stbvox_map->ecolor_color,
        &stbvox_map->ecolor_facemask,
        &stbvox_map->extended_color,
        &stbvox_map->geometry,
        &stbvox_map->lighting,
        &stbvox_map->overlay,
        &stbvox_map->rotate,
        &stbvox_map->tex2,
        &stbvox_map->tex2_facemask,
        &stbvox_map->tex2_replace,
        &stbvox_map->vheight
    };

    // Set voxel maps for stb voxel
    int zero = voxelMap->GetIndex(0, 0, 0);
    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        *stb_data[i] = ((1 << i) & voxelMap->dataMask_) ? &datas[i]->At(zero) : 0;

#if 1
    VoxelRangeFragment processRange = workload->range;
    if (voxelMap->GetVoxelProcessors().Size() > 0 && voxelMap->processorDataMask_)
    {
		PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
		slot->writer.GetBasicDataArrays(writerDatas);

        // corrects the voxel processor to tell it to bleed outside the range
        processRange.startX--;
        processRange.startY--;
        processRange.startZ--;
        processRange.endX++;
        processRange.endY++;
        processRange.endZ++;

        PODVector<StringHash> mapProcessors = voxelMap->GetVoxelProcessors();
        for (unsigned p = 0; p < mapProcessors.Size(); ++p)
        {
            if (processors_.Contains(mapProcessors[p]))
            {
                VoxelProcessorFunc func = processors_[mapProcessors[p]];
                func(voxelMap, &slot->writer, processRange);
            }
        }

        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (((1 << i) & voxelMap->processorDataMask_))
                *stb_data[i] = &writerDatas[i]->At(zero);
        }
    }
#endif

    stbvox_reset_buffers(mm);
    stbvox_set_buffer(mm, 0, 0, slot->workVertexBuffers[workload->workloadIndex], VOXEL_WORKER_VERTEX_BUFFER_SIZE);
    stbvox_set_buffer(mm, 0, 1, slot->workFaceBuffers[workload->workloadIndex], VOXEL_WORKER_FACE_BUFFER_SIZE);

    stbvox_set_default_mesh(mm, 0);

    bool success = true;
    for (unsigned y = 0; y < workload->range.endY; y += 16)
    {
        stbvox_set_input_range(
            mm,
            workload->range.startX,
            workload->range.startZ,
            workload->range.startY + y,
            workload->range.endX,
            workload->range.endZ,
            Min(y + 16, workload->range.endY)
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
        material->SetShaderParameter("ColorTable", defaultColorTable_);
    return true;
}

bool VoxelBuilder::UploadGpuDataCompatibilityMode(VoxelBuildSlot* slot, bool append)
{
    //if (!vb->SetSize(totalVertices, MASK_POSITION | MASK_TEXCOORD1 | MASK_NORMAL, true))
    //    return false;
    return false;
}

bool VoxelBuilder::UploadGpuData(VoxelJob* job)
{
    PROFILE(UploadGpuData);

    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    unsigned numQuads = mesh.numQuads_;
    if (numQuads == 0)
        return true;

    VertexBuffer* chunkVertexData = mesh.vertexData_;
    IndexBuffer* chunkFaceData = mesh.faceData_;

    if (!chunkVertexData->SetSize(numQuads * 4, MASK_DATA, false))
    {
        LOGERROR("Error allocating voxel vertex data.");
        return false;
    }

    if (!chunkFaceData->SetSize(numQuads, true, false))
    {
        LOGERROR("Error allocating voxel face data.");
        return false;
    }

    unsigned quadPosition = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        unsigned quadCount = mesh.rawFaceData_[i].Size() / sizeof(unsigned);
        if (!chunkVertexData->SetDataRange(&mesh.rawVertexData_[i].Front(), quadPosition * 4, quadCount * 4))
        {
            LOGERROR("Error uploading voxel vertex data.");
            return false;
        }

        if (!chunkFaceData->SetDataRange(&mesh.rawFaceData_[i].Front(), quadPosition, quadCount))
        {
            LOGERROR("Error uploading voxel face data.");
            return false;
        }
        quadPosition += quadCount;
    }

    // face data
    Material* material = mesh.material_;
    {
        TextureBuffer* faceDataTexture = mesh.faceBuffer_;
        if (!faceDataTexture->SetSize(0))
        {
            LOGERROR("Error initializing voxel texture buffer");
            return false;
        }
        if (!faceDataTexture->SetData(chunkFaceData))
        {
            LOGERROR("Error setting voxel texture buffer data");
            return false;
        }
        material->SetTexture(TU_CUSTOM1, faceDataTexture);
    }

    // set shared quad index buffer
    if (!ResizeIndexBuffer(numQuads))
    {
        LOGERROR("Error resizing shared voxel index buffer");
        return false;
    }

    Geometry* geo = mesh.geometry_;
    geo->SetIndexBuffer(sharedIndexBuffer_);
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, numQuads * 6, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }
    return true;
}

unsigned VoxelBuilder::DecrementBuildSlot(VoxelBuildSlot* slot)
{
    MutexLock lock(slot->workMutex);
    slot->workCounter--;
    return slot->workCounter;
}

bool VoxelBuilder::IsBuilding()
{
    MutexLock lock(slotMutex_);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (!slots_[i].free)
            return true;
    }
    return false;
}

void VoxelBuilder::FreeBuildSlot(VoxelBuildSlot* slot)
{
    MutexLock lock(slotMutex_);
    slot->failed = false;
    slot->workCounter = 0;
    slot->numQuads = 0;
    slot->numWorkloads = 0;
    slot->box = BoundingBox();
    slot->free = true;
    slot->job = 0;
}

int VoxelBuilder::GetFreeBuildSlot()
{
    MutexLock lock(slotMutex_);
    VoxelBuildSlot* slot = 0;
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (slots_[i].free)
        {
            slots_[i].free = false;
            slot = &slots_[i];
            return i;
        }
    }
    return -1;
}
bool VoxelBuilder::ResizeIndexBuffer(unsigned numQuads)
{
    if (!(sharedIndexBuffer_.Null() || sharedIndexBuffer_->GetIndexCount() / 6 < numQuads))
        return true;

    unsigned numIndices = numQuads * 6;
    unsigned numVertices = numQuads * 4;

    unsigned indexSize = sizeof(int); // numVertices > M_MAX_UNSIGNED_SHORT ? sizeof(int) : sizeof(short);
    //SharedArrayPtr<unsigned char> data(new unsigned char[indexSize * (unsigned)(numIndices * 1.1)]); // add padding to reduce number of resizes
    SharedArrayPtr<unsigned char> data(new unsigned char[indexSize * 1000000]); // add padding to reduce number of resizes
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

}
