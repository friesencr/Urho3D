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

static float URHO3D_default_ambient[4][4] =
{
	{ 0, 0, 1, 0 }, // reversed lighting direction
	{ 0.5, 0.5, 0.5, 0 }, // directional color
	{ 0.5, 0.5, 0.5, 0 }, // constant color
	{ 0.5, 0.5, 0.5, 1.0f / 1000.0f / 1000.0f }, // fog data for simple_fog
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


namespace Urho3D
{

	VoxelBuilder::VoxelBuilder(Context* context)
		: Object(context),
		compatibilityMode(false),
		sharedIndexBuffer_(0),
		maxFrameTime_(1),
		completeAllWork_(false)
	{
		transform_.Resize(3);
		normals_.Resize(32);
		ambientTable_.Resize(4);
		texscaleTable_.Resize(64);
		texgenTable_.Resize(64);


		// copy transforms
		transform_[0] = Vector3(1.0, 0.5f, 1.0);
		transform_[1] = Vector3(0.0, 0.0, 0.0);
		transform_[2] = Vector3((float)(0 & 255), (float)(0 & 255), (float)(0 & 255));

		// copy normal table
		for (unsigned i = 0; i < 32; ++i)
			normals_[i] = Vector3(URHO3D_default_normals[i]);

		// ambient color table
		for (unsigned i = 0; i < 4; ++i)
			ambientTable_[i] = Vector3(URHO3D_default_ambient[i]);

		// texscale table
		for (unsigned i = 0; i < 64; ++i)
			texscaleTable_[i] = Vector4(URHO3D_default_texscale[i]);

		// texgen table
		for (unsigned i = 0; i < 64; ++i)
			texgenTable_[i] = Vector3(URHO3D_default_texgen[i]);

		AllocateWorkerBuffers();
		SubscribeToEvent(E_BEGINFRAME, HANDLER(VoxelBuilder, HandleBeginFrame));
	}

	VoxelBuilder::~VoxelBuilder()
	{

	}

	void VoxelBuilder::AllocateWorkerBuffers()
	{
		//WorkQueue* queue = GetSubsystem<WorkQueue>();

		// the workers will build a chunk and then convert it to urho vertex buffers, while the vertex buffer is being built it will start more chunks
		// but stall if the mesh building gets too far ahead of the vertex converter this is based on chunk size and number of cpu threads
		unsigned numSlots = 2; // (unsigned)Min((float)numChunks, Max(1.0f, ceil((float)queue->GetNumThreads() / (float)VOXEL_MAX_NUM_WORKERS_PER_CHUNK)) * 2.0f);

		if (numSlots == slots_.Size())
			return;

		slots_.Resize(numSlots);
		for (unsigned i = 0; i < slots_.Size(); ++i)
		{
			for (unsigned a = 0; a < 4; ++a)
				stbvox_init_mesh_maker(&slots_[i].meshMakers[a]);

			slots_[i].job = 0;
			slots_[i].workloads.Resize(4);
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
		PODVector<int> data(numIndices);
		// triangulates the quads
		for (unsigned i = 0; i < numQuads; i++)
		{
			data[i * 6] = i * 4;
			data[i * 6 + 1] = i * 4 + 1;
			data[i * 6 + 2] = i * 4 + 2;
			data[i * 6 + 3] = i * 4 + 2;
			data[i * 6 + 4] = i * 4 + 3;
			data[i * 6 + 5] = i * 4;
		}

		if (sharedIndexBuffer_.Null())
		{
			sharedIndexBuffer_ = new IndexBuffer(context_);
			sharedIndexBuffer_->SetShadowed(true);
		}

		if (!sharedIndexBuffer_->SetSize(numIndices, true, false))
		{
			LOGERROR("Could not set size of shared voxel index buffer.");
			return false;
		}

		if (!sharedIndexBuffer_->SetData(&data.Front()))
		{
			LOGERROR("Could not set data of shared voxel index buffer.");
			return false;
		}
		return true;
	}

	void VoxelBuilder::CompleteWork(unsigned priority)
	{
		completeAllWork_ = true;
		while (RunJobs() > 0)
		{
			Time::Sleep(0);
		}
		completeAllWork_ = false;
		//WorkQueue* workQueue = GetSubsystem<WorkQueue>();
		//workQueue->Complete(M_MAX_UNSIGNED);
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

			Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0, (float)((v1 >> 7u) & 127u));
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

	VoxelJob* VoxelBuilder::BuildVoxelChunkAsync(SharedPtr<VoxelChunk> chunk)
	{
		if (chunk.Null())
			return 0;

		SharedPtr<VoxelMap> voxelMap(chunk->GetVoxelMap());
		if (voxelMap.Null())
			return 0;

		unsigned char width = voxelMap->width_;
		unsigned char depth = voxelMap->depth_;
		unsigned char height = voxelMap->height_;

		if (!(width > 0 && depth > 0 && height > 0))
			return 0;

		VoxelJob* job = CreateJob(chunk);
		job->async = true;
		RunJobs();

		return job;
	}

	VoxelJob* VoxelBuilder::BuildVoxelChunk(SharedPtr<VoxelChunk> chunk)
	{
		if (chunk.Null())
			return 0;

		SharedPtr<VoxelMap> voxelMap(chunk->GetVoxelMap());
		if (voxelMap.Null())
			return 0;

		unsigned char width = voxelMap->width_;
		unsigned char depth = voxelMap->depth_;
		unsigned char height = voxelMap->height_;

		if (!(width > 0 && depth > 0 && height > 0))
			return 0;

		VoxelJob* job = CreateJob(chunk);
		job->async = false;

		RunJobs();
		CompleteWork();
		return 0;
	}

	int VoxelBuilder::RunJobs()
	{
		if (!completeAllWork_ && frameTimer_.GetMSec(false) > maxFrameTime_)
			return 0;

		for (unsigned i = 0; i < slots_.Size(); ++i)
		{
			VoxelWorkSlot* slot = &slots_[i];
			bool process = false;
			{
				MutexLock lock(slot->workMutex);
				process = !slot->free && slot->upload;
				slot->upload = false;
			}
			if (process)
			{
				ProcessSlot(slot);
				return jobs_.Size();
			}
		}

		for (;;)
		{
			if (!jobs_.Empty())
			{
				int slotId = GetFreeWorkSlot();
				if (slotId == -1)
					break;

				unsigned count = jobs_.Size();
				VoxelJob* job = jobs_[0];
				VoxelWorkSlot* slot = &slots_[slotId];
				job->slot = slotId;
				slot->job = job;
				jobs_.Erase(0);
				ProcessJob(job);
			}
			else
				break;
		}

		return jobs_.Size();
	}

	VoxelJob* VoxelBuilder::CreateJob(VoxelChunk* chunk)
	{
		VoxelJob* job = new VoxelJob();
		job->chunk = chunk;
		job->slot = 0;
		unsigned size = jobs_.Size();
		jobs_.Push(job);
		size = jobs_.Size();
		return job;
	}

	void VoxelBuilder::ProcessJob(VoxelJob* job)
	{
		WorkQueue* workQueue = GetSubsystem<WorkQueue>();
		VoxelWorkSlot* slot = &slots_[job->slot];
		VoxelMap* voxelMap = job->chunk->GetVoxelMap();

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
					workItem->priority_ = M_MAX_UNSIGNED;
					workItem->aux_ = workload;
					workItem->start_ = slot;
					workItem->workFunction_ = BuildWorkloadHandler;
					workload->workItem = workItem;
					workQueue->AddWorkItem(workItem);
				}
			}
		}

		//if (!async)
		//{
		//	workQueue->Complete(M_MAX_UNSIGNED);
		//	ProcessSlot(slot);
		//}
	}

	void VoxelBuilder::BuildWorkload(VoxelWorkload* workload)
	{
		VoxelWorkSlot* slot = &slots_[workload->slot];
		bool success = BuildMesh(workload);

		if (success)
		{
			DecodeWorkBuffer(workload);
		}
		else
		{
			MutexLock lock(slot->workMutex);
			slot->failed = true;
		}

		if (DecrementWorkSlot(slot) == 0)
			slot->upload = true;
		//{
		//	if (workload->slot->job->async)
		//		workload->workItem->sendEvent_ = true;
		//}
	}

	void VoxelBuilder::AbortSlot(VoxelWorkSlot* slot)
	{

	}

	void VoxelBuilder::CancelJob(VoxelJob* job)
	{
		jobs_.Remove(job);
	}

	bool VoxelBuilder::BuildMesh(VoxelWorkload* workload)
	{
		VoxelWorkSlot* slot = &slots_[workload->slot];
		VoxelJob* job = slot->job;
		VoxelChunk* chunk = job->chunk;
		VoxelMap* voxelMap = chunk->GetVoxelMap();
		VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;

		stbvox_mesh_maker* mm = &slot->meshMakers[workload->workloadIndex];
		stbvox_set_input_stride(mm, voxelMap->xStride, voxelMap->zStride);

		stbvox_input_description *map;
		map = stbvox_get_input_description(mm);

		VoxelChunk* northChunk = chunk->GetNeighborNorth();
		VoxelChunk* southChunk = chunk->GetNeighborSouth();
		VoxelChunk* eastChunk = chunk->GetNeighborEast();
		VoxelChunk* westChunk = chunk->GetNeighborWest();
		VoxelMap* northMap = northChunk ? northChunk->GetVoxelMap() : 0;
		VoxelMap* southMap = southChunk ? southChunk->GetVoxelMap() : 0;
		VoxelMap* eastMap = eastChunk ? eastChunk->GetVoxelMap() : 0;
		VoxelMap* westMap = westChunk ? westChunk->GetVoxelMap() : 0;

		if (voxelBlocktypeMap)
		{
			map->block_tex1 = &voxelBlocktypeMap->blockTex1.Front();
			map->block_tex1_face = &voxelBlocktypeMap->blockTex1Face.Front();
			map->block_tex2 = &voxelBlocktypeMap->blockTex2.Front();
			map->block_tex2_face = &voxelBlocktypeMap->blockTex2Face.Front();
			map->block_geometry = &voxelBlocktypeMap->blockGeometry.Front();
			map->block_vheight = &voxelBlocktypeMap->blockVHeight.Front();
		}

		if (voxelMap->blocktype.Size())
		{
			map->blocktype = &voxelMap->blocktype[voxelMap->GetIndex(0, 0, 0)];

			// transfer data from nearby blocks
			// we are assuming same size 
			if (northMap)
				for (unsigned x = 0; x < voxelMap->width_; ++x)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->SetBlocktype(x, y, voxelMap->depth_, northMap->GetBlocktype(x, y, 0));

			if (southMap)
				for (unsigned x = 0; x < voxelMap->width_; ++x)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->SetBlocktype(x, y, -1, southMap->GetBlocktype(x, y, southMap->depth_ - 1));

			if (eastMap)
				for (unsigned z = 0; z < voxelMap->depth_; ++z)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->SetBlocktype(voxelMap->width_, y, z, eastMap->GetBlocktype(0, y, z));
			if (westMap)
				for (unsigned z = 0; z < voxelMap->depth_; ++z)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->SetBlocktype(-1, y, z, westMap->GetBlocktype(westMap->width_ - 1, y, z));
		}

		if (voxelMap->geometry.Size())
		{
			map->geometry = &voxelMap->geometry[voxelMap->GetIndex(0, 0, 0)];

		}

		if (voxelMap->vHeight.Size())
		{
			map->vheight = &voxelMap->vHeight[voxelMap->GetIndex(0, 0, 0)];

			if (northMap)
				for (unsigned x = 0; x < voxelMap->width_; ++x)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->vHeight[voxelMap->GetIndex(x, y, voxelMap->depth_)] = northMap->GetVheight(x, y, 0);

			if (southMap)
				for (unsigned x = 0; x < voxelMap->width_; ++x)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->vHeight[voxelMap->GetIndex(x, y, -1)] = southMap->GetVheight(x, y, southMap->depth_ - 1);

			if (eastMap)
				for (unsigned z = 0; z < voxelMap->depth_; ++z)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->vHeight[voxelMap->GetIndex(voxelMap->width_, y, z)] = eastMap->GetVheight(0, y, z);
			if (westMap)
				for (unsigned z = 0; z < voxelMap->depth_; ++z)
					for (unsigned y = 0; y < voxelMap->height_; ++y)
						voxelMap->vHeight[voxelMap->GetIndex(-1, y, z)] = westMap->GetVheight(westMap->width_ - 1, y, z);
		}



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
				success = false;
				break;
			}
		}
		workload->numQuads = stbvox_get_quad_count(mm, 0);
		return success;
	}

	bool VoxelBuilder::SetMaterialParameters(Material* material)
	{
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		Technique* technique = cache->GetResource<Technique>("Techniques/VoxelDiff.xml");
		material->SetTechnique(0, technique);
		material->SetShaderParameter("Transform", transform_);
		material->SetShaderParameter("NormalTable", normals_);
		material->SetShaderParameter("AmbientTable", ambientTable_);
		material->SetShaderParameter("TexScale", texscaleTable_);
		material->SetShaderParameter("TexGen", texgenTable_);
		return true;
	}

	bool VoxelBuilder::UploadGpuData(VoxelWorkSlot* slot, bool append)
	{
		VoxelJob* job = slot->job;
		VoxelChunk* chunk = job->chunk;
		VertexBuffer* vb = chunk->GetVertexBuffer(0);
		IndexBuffer* faceData = chunk->GetFaceData(0);
		chunk->numQuads_[0] = slot->numQuads;
		int totalVertices = slot->numQuads * 4;
		if (totalVertices == 0)
			return true;

		//if (!vb->SetSize(totalVertices, MASK_POSITION | MASK_TEXCOORD1 | MASK_NORMAL, true))
		//    return false;

		if (!vb->SetSize(totalVertices, MASK_DATA, false))
			return false;

		if (!faceData->SetSize(slot->numQuads, true, false))
			return false;

		int end = 0;
		int start = 0;
		if (slot->numWorkloads < 4)
			int missingworkloads = 0;

		for (int i = 0; i < slot->numWorkloads; ++i)
		{
			VoxelWorkload* workload = &slot->workloads[i];
			start = end;
			end = start + workload->numQuads;
			//if (!vb->SetDataRange(&workload->gpuData.Front(), start, workload->numQuads * 4))
			unsigned char* vertexBuffer = (unsigned char*)slot->workVertexBuffers[workload->workloadIndex];
			if (!vb->SetDataRange(vertexBuffer, start * 4, workload->numQuads * 4))
			{
				LOGERROR("Error uploading voxel vertex data.");
				return false;
			}

			unsigned char* indexBuffer = (unsigned char*)slot->workFaceBuffers[workload->workloadIndex];
			if (!faceData->SetDataRange(indexBuffer, start, workload->numQuads))
			{
				LOGERROR("Error uploading voxel face data.");
				return false;
			}
		}

		if (!chunk->GetHasShaderParameters(0))
		{
			SetMaterialParameters(chunk->GetMaterial(0));
			chunk->SetHasShaderParameters(0, true);
		}

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


	{
		//const unsigned char* vertexData = 0;
		//const unsigned char* indexData = 0;
		//unsigned vertexCount = 0;
		//unsigned indexCount = 0;
		//unsigned mask = 0;
		//geo->GetRawData(vertexData, vertexCount, indexData, indexCount, mask);
		//PODVector<unsigned char> cpuData(vertexData, vertexCount * 4);

		Geometry* geo = chunk->GetGeometry(0);
		int indexEnd = end * 6; // 6 indexes per quad
		if (!ResizeIndexBuffer(VOXEL_CHUNK_SIZE))
		{
			LOGERROR("Error resizing shared voxel index buffer");
			return false;
		}

		geo->SetIndexBuffer(sharedIndexBuffer_);
		if (!geo->SetDrawRange(TRIANGLE_LIST, 0, indexEnd, false))
		{
			LOGERROR("Error setting voxel draw range");
			return false;
		}
	}
	}

	void VoxelBuilder::ProcessSlot(VoxelWorkSlot* slot)
	{
		if (!slot->failed)
			if (!UploadGpuData(slot))
				LOGERROR("Could not upload voxel data to graphics card.");

		FreeWorkSlot(slot);
	}

	void VoxelBuilder::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
	{
		frameTimer_.Reset();
		while (frameTimer_.GetMSec(false) < maxFrameTime_ &&  RunJobs() > 0)
		{
			Time::Sleep(0);
		}
	}

	unsigned VoxelBuilder::DecrementWorkSlot(VoxelWorkSlot* slot)
	{
		MutexLock lock(slot->workMutex);
		slot->workCounter--;
		return slot->workCounter;
	}

	void VoxelBuilder::FreeWorkSlot(VoxelWorkSlot* slot)
	{
		MutexLock lock(slotMutex_);
		slot->failed = false;
		slot->workCounter = 0;
		slot->numQuads = 0;
		slot->box = BoundingBox();
		slot->numWorkloads = 0;
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
				slot->free = false;
				return i;
			}
		}
		return -1;
	}

#if 0
	static inline long longPair(unsigned a, unsigned b)
	{
		return (long)a & ((long)b >> 32);
	}

	void VoxelBuilder::SimplifyMesh(VoxelWorkSlot* slot)
	{
		//int Polyhedron::MergeAdjacentPlanarFaces(bool snapVerticesToMergedPlanes, bool conservativeEnclose, float angleEpsilon, float distanceEpsilon)
		Vector<Vector4> faceNormals(slot->numQuads);
		// get quad normals

		Vector<unsigned> faceGroups(slot->numQuads);
		for (unsigned i = 0; i < slot->numQuads; ++i)
			faceGroups[i] = i;

		PODVector<Vector3> vertices(slot->numQuads * 4);
		{
			Vector3* ptr = &vertices.Front();
			for (unsigned i = 0; slot->numWorkloads; ++i)
			{
				VoxelWorkload* workload = &slot->workloads[i];
				unsigned* vertexData = (unsigned*)slot->workVertexBuffers[workload->threadIndex];
				unsigned* faceData = (unsigned*)slot->workFaceBuffers[workload->threadIndex];
				for (unsigned j = 0; j < workload->numQuads; ++j)
				{
					for (unsigned k = 0; k < 4; ++k)
					{
						unsigned data = *vertexData++;
						Vector3 vertex((float)(data & 127u), (float)((data >> 14u) & 511u) / 2.0, (float)((data >> 7u) & 127u));
						*ptr++ = vertex;
					}
				}
			}
		}
		unsigned numMerges = 0;
		unsigned numVertices = slot->numQuads * 4;
		HashMap<long, int> verticesToFaces;
		for (unsigned i = 0; i < slot->numQuads; ++i)
		{
			unsigned v0 = i + 4;
			for (size_t j = 0; j < 4; ++j)
			{
				unsigned v1 = i + j;
				long key = (long)v0 & ((long)v1 >> 32)
					verticesToFaces[key] = i;
				verticesToFaces.Find( )
					std::map<std::pair<int, int>, int>::iterator neighbor = verticesToFaces.find(std::make_pair(v1, v0));

				if (neighbor != verticesToFaces.end())
				{
					int nf = neighbor->second;
					if (!f[nf].v.empty())
					{
						cv thisNormal = faceNormals[i];
						cv nghbNormal = faceNormals[nf];
						if (thisNormal.Dot(nghbNormal) >= 1.0 - angleEpsilon)
						{
							cs eNeg, ePos, nNeg, nPos;
							PolyExtremeVertexOnFace(*this, i, nghbNormal, eNeg, ePos);
							//						PolyExtremeVertexOnFace(*this, nf, nghbNormal, nNeg, nPos);
							//						if (Max(ePos, nPos) - Min(eNeg, nNeg) <= distanceEpsilon)
							PolyExtremeVertexOnFace(*this, nf, thisNormal, nNeg, nPos);
							if (ePos - eNeg <= distanceEpsilon && nPos - nNeg <= distanceEpsilon)
							{
								++numMerges;
								// Merge this face to neighboring face.
								int fg = i;
								while (faceGroups[fg] != fg)
									fg = faceGroups[fg];
								int nfgr = nf;
								while (faceGroups[nfgr] != nfgr)
									nfgr = faceGroups[nfgr];
								faceGroups[fg] = nfgr;
								break;
							}
						}
					}
				}
				v0 = v1;
			}
		}

		std::vector<std::set<std::pair<int, int> > > newEdgesPerFace(f.size());
		for (size_t i = 0; i < f.size(); ++i)
		{
			Face &face = f[i];

			int fg = i;
			while (faceGroups[fg] != fg)
				fg = faceGroups[fg];

			int v0 = face.v.back();
			for (size_t j = 0; j < face.v.size(); ++j)
			{
				int v1 = face.v[j];

				if (newEdgesPerFace[fg].find(std::make_pair(v1, v0)) != newEdgesPerFace[fg].end())
				{
					newEdgesPerFace[fg].erase(std::make_pair(v1, v0));
				}
				else
				{
					newEdgesPerFace[fg].insert(std::make_pair(v0, v1));
				}

				v0 = v1;
			}
		}

		for (size_t i = 0; i < f.size(); ++i)
		{
			Face &face = f[i];

			std::vector<std::pair<int, int> > boundaryEdges(newEdgesPerFace[i].begin(), newEdgesPerFace[i].end());
			for (size_t j = 0; j < boundaryEdges.size(); ++j)
				for (size_t k = j + 1; k < boundaryEdges.size(); ++k)
					if (boundaryEdges[j].second == boundaryEdges[k].first)
					{
						Swap(boundaryEdges[j + 1], boundaryEdges[k]);
						break;
					}
			face.v.clear();
			for (size_t j = 0; j < boundaryEdges.size(); ++j)
				face.v.push_back(boundaryEdges[j].first);

			// Snap all vertices to the plane of the face.
			if (snapVerticesToMergedPlanes)
			{
				// If conservativeEnclose == true, compute the maximum distance for the plane
				// so it encloses all the points. Otherwise, compute the average.
				cs d = conservativeEnclose ? -FLOAT_INF : 0;
				for (size_t j = 0; j < face.v.size(); ++j)
				{
					int vtx = face.v[j];
					if (conservativeEnclose)
						d = Max(d, faceNormals[i].Dot(POINT_TO_FLOAT4(vec(v[vtx]))));
					else
						d += faceNormals[i].Dot(POINT_TO_FLOAT4(vec(v[vtx])));
				}
				if (!conservativeEnclose)
					d /= (cs)face.v.size();
				for (size_t j = 0; j < face.v.size(); ++j)
				{
					cv vtx = POINT_TO_FLOAT4(vec(v[face.v[j]]));
					v[face.v[j]] = FLOAT4_TO_POINT((vtx + (d - faceNormals[i].Dot(vtx)) * faceNormals[i]).ToFloat4());
				}
			}
		}
		RemoveDegenerateFaces();
		RemoveRedundantVertices();
		assume(IsClosed());
		assume(IsConvex());

		return numMerges;
	}
#endif


}
