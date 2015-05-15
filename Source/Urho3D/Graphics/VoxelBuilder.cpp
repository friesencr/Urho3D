#include "VoxelBuilder.h"
#include "../Resource/ResourceCache.h"

#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE  0
#include <STB/stb_voxel_render.h>

#define URHO3D_RSQRT2   0.7071067811865f
#define URHO3D_RSQRT3   0.5773502691896f

static float URHO3D_default_normals[32][3] =
{
    { 1,0,0 },  // east
    { 0,1,0 },  // north
    { -1,0,0 }, // west
    { 0,-1,0 }, // south
    { 0,0,1 },  // up
    { 0,0,-1 }, // down
    {  URHO3D_RSQRT2,0, URHO3D_RSQRT2 }, // east & up
    {  URHO3D_RSQRT2,0, -URHO3D_RSQRT2 }, // east & down

    {  URHO3D_RSQRT2,0, URHO3D_RSQRT2 }, // east & up
    { 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
    { -URHO3D_RSQRT2,0, URHO3D_RSQRT2 }, // west & up
    { 0,-URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
    {  URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // ne & up
    {  URHO3D_RSQRT3, URHO3D_RSQRT3,-URHO3D_RSQRT3 }, // ne & down
    { 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
    { 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down

    {  URHO3D_RSQRT2,0, -URHO3D_RSQRT2 }, // east & down
    { 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down
    { -URHO3D_RSQRT2,0, -URHO3D_RSQRT2 }, // west & down
    { 0,-URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
    { -URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NW & up
    { -URHO3D_RSQRT3, URHO3D_RSQRT3,-URHO3D_RSQRT3 }, // NW & down
    { -URHO3D_RSQRT2,0, URHO3D_RSQRT2 }, // west & up
    { -URHO3D_RSQRT2,0, -URHO3D_RSQRT2 }, // west & down

    {  URHO3D_RSQRT3, URHO3D_RSQRT3,URHO3D_RSQRT3 }, // NE & up crossed
    { -URHO3D_RSQRT3, URHO3D_RSQRT3,URHO3D_RSQRT3 }, // NW & up crossed
    { -URHO3D_RSQRT3,-URHO3D_RSQRT3,URHO3D_RSQRT3 }, // SW & up crossed
    {  URHO3D_RSQRT3,-URHO3D_RSQRT3,URHO3D_RSQRT3 }, // SE & up crossed
    { -URHO3D_RSQRT3,-URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SW & up
    { -URHO3D_RSQRT3,-URHO3D_RSQRT3,-URHO3D_RSQRT3 }, // SW & up
    { 0,-URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
    { 0,-URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
};

namespace Urho3D
{

	VoxelBuilder::VoxelBuilder(Context* context) 
		: Object(context),
		sharedIndexBuffer_(0)
	{

		//SubscribeToEvent(E_WORKITEMCOMPLETED, HANDLER(VoxelBuilder, HandleWorkItemCompleted));
	}

	VoxelBuilder::~VoxelBuilder()
	{

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
			for (unsigned a = 0; a < 8; ++a)
				stbvox_init_mesh_maker(&slots_[i].meshMakers[a]);

			slots_[i].job = 0;
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
		for (int i = 0; i < numQuads; i++)
		{
			data[i * 6] = i * 4;
			data[i * 6 + 1] = i * 4 + 1;
			data[i * 6 + 2] = i * 4 + 2;
			data[i * 6 + 3] = i * 4 + 2;
			data[i * 6 + 4] = i * 4 + 3;
			data[i * 6 + 5] = i * 4;
		}
		sharedIndexBuffer_ = new IndexBuffer(context_);
		if (!sharedIndexBuffer_->SetSize(numIndices, true, false))
			return false;

		if (!sharedIndexBuffer_->SetData(&data.Front()))
			return false;
	}

	void VoxelBuilder::CompleteWork(unsigned priority)
	{
		WorkQueue* workQueue = GetSubsystem<WorkQueue>();
		workQueue->Complete(M_MAX_UNSIGNED);
	}

	// Decode to vertex buffer
	void VoxelBuilder::DecodeWorkBuffer(VoxelWorkload* workload)
	{
		int numQuads = workload->numQuads;
		if (numQuads <= 0)
			return;

		VoxelWorkSlot* slot = workload->slot;
		int threadIndex = workload->threadIndex;
		int gpuVertexSize = sizeof(float) * 8; // position(3) / normal(3) / uv1(2)
		int cpuVertexSize = sizeof(float) * 3; // position(3)
		int numVertices = numQuads * 4; // 4 verts in a quad

		workload->gpuData.Resize(numVertices * gpuVertexSize);
		workload->cpuData.Resize(numVertices * cpuVertexSize);
		float* gpu = (float*)&workload->gpuData.Front();
		float* cpu = (float*)&workload->cpuData.Front();
		unsigned int* workData = (unsigned int*)slot->workBuffers[threadIndex];

		for (int i = 0; i < numVertices; ++i)
		{
			unsigned int v1 = *workData++;
			unsigned int v2 = *workData++;

			Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0, (float)((v1 >> 7u) & 127u));
			//float amb_occ = (float)((v1 >> 23u) & 63u) / 63.0;
			//unsigned char tex1 = v2 & 0xFF;
			//unsigned char tex2 = (v2 >> 8) & 0xFF;
			//unsigned char color = (v2 >> 16) & 0xFF;
			unsigned char face_info = (v2 >> 24) & 0xFF;
			unsigned char normal = face_info >> 2u;
			//unsigned char face_rot = face_info & 2u;
			Vector3 normalf(URHO3D_default_normals[normal]);
			normalf = Vector3(normalf.x_, normalf.z_, normalf.y_);

			workload->box.Merge(position);

			//LOGINFO(position.ToString());

			*gpu++ = position.x_;
			*gpu++ = position.y_;
			*gpu++ = position.z_;
			*cpu++ = position.x_;
			*cpu++ = position.y_;
			*cpu++ = position.z_;

			*gpu++ = normalf.x_;
			*gpu++ = normalf.y_;
			*gpu++ = normalf.z_;

			if (i % 4 == 0)
			{
				*gpu++ = 0.0;
				*gpu++ = 0.0;
			}
			else if (i % 4 == 1)
			{
				*gpu++ = 1.0;
				*gpu++ = 0.0;
			}
			else if (i % 4 == 2)
			{
				*gpu++ = 1.0;
				*gpu++ = 1.0;
			}
			else if (i % 4 == 3)
			{
				*gpu++ = 0.0;
				*gpu++ = 1.0;
			}
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
		workload->threadIndex = threadIndex;
		workload->builder->BuildWorkload(workload);
	}

	VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, SharedPtr<VoxelDefinition> voxelDefinition)
	{
		if (voxelDefinition.Null())
			return 0;

		unsigned char width = voxelDefinition->width_;
		unsigned char depth = voxelDefinition->depth_;
		unsigned char height = voxelDefinition->height_;

		if (!(width > 0 && depth > 0 && height > 0))
			return 0;

		VoxelJob* job = CreateJob(chunk, voxelDefinition);
		RunJobs();
		return job;
	}

	int VoxelBuilder::RunJobs()
	{
		if (jobs_.Empty())
			return 0;

		AllocateWorkerBuffers();
		unsigned total = 0;
		for (;;)
		{
			VoxelWorkSlot* slot = GetFreeWorkSlot();
			if (slot == 0)
				return total;

			if (!jobs_.Empty())
			{
				total++;
				VoxelJob* job(jobs_[0]);
				job->slot = slot;
				slot->job = job;
				slot->free = false;
				jobs_.Erase(0);
				ProcessJob(job);
			}
			else
				return total;
		}

		return total;
	}

	VoxelJob* VoxelBuilder::CreateJob(VoxelChunk* chunk, VoxelDefinition* voxelDefinition)
	{
		VoxelJob* job = new VoxelJob();
		job->chunk = chunk;
		job->voxelDefinition = voxelDefinition;
		job->slot = 0;
		jobs_.Push(job);
		return job;
	}

	void VoxelBuilder::ProcessJob(VoxelJob* job)
	{
		WorkQueue* workQueue = GetSubsystem<WorkQueue>();
		VoxelChunk* chunk = job->chunk;
		VoxelWorkSlot* slot = job->slot;
		VoxelDefinition* definition = job->voxelDefinition;

		unsigned char workloadsX = (unsigned char)ceil((float)definition->width_ / (float)VOXEL_WORKER_SIZE_X);
		unsigned char workloadsY = (unsigned char)ceil((float)definition->height_ / (float)VOXEL_WORKER_SIZE_Y);
		unsigned char workloadsZ = (unsigned char)ceil((float)definition->depth_ / (float)VOXEL_WORKER_SIZE_Z);
		for (unsigned char x = 0; x < workloadsX; ++x)
		{
			for (unsigned char z = 0; z < workloadsZ; ++z)
			{
				for (unsigned char y = 0; y < workloadsY; ++y)
				{
					VoxelWorkload* workload = new VoxelWorkload();
					workload->builder = this;
					workload->slot = slot;
					workload->index[0] = x; 
					workload->index[1] = y; 
					workload->index[2] = z; 
					workload->start[0] = x * VOXEL_WORKER_SIZE_X;
					workload->start[1] = 0;
					workload->start[2] = z * VOXEL_WORKER_SIZE_Z;
					workload->end[0] = Min((int)definition->width_, (x + 1) * VOXEL_WORKER_SIZE_X);
					workload->end[1] = Min((int)definition->height_, (y + 1) * VOXEL_WORKER_SIZE_Y);
					workload->end[2] = Min((int)definition->depth_, (z + 1) * VOXEL_WORKER_SIZE_Z);
					workload->threadIndex = 0;
					workload->numQuads = 0;

					SharedPtr<WorkItem> workItem(new WorkItem());
					workItem->priority_ = M_MAX_UNSIGNED;
					workItem->aux_ = workload;
					workItem->start_ = slot;
					workItem->workFunction_ = BuildWorkloadHandler;
					workload->workItem = workItem;

					slot->workCounter++;
					slot->workloads.Push(workload);
					workQueue->AddWorkItem(workItem);
				}
			}
		}
		workQueue->Complete(M_MAX_UNSIGNED);
		ProcessSlot(slot);
	}

	void VoxelBuilder::TransferDataToSlot(VoxelWorkload* workload)
	{
		VoxelWorkSlot* slot = workload->slot;
		MutexLock lock(slot->dataMutex);
	}

	void VoxelBuilder::BuildWorkload(VoxelWorkload* workload)
	{
		bool success = BuildMesh(workload);

		if (success)
			DecodeWorkBuffer(workload);
		else
		{
			MutexLock lock(workload->slot->workMutex);
			workload->slot->failed = true;
		}

		if (DecrementWorkSlot(workload->slot, workload) == 0)
			workload->workItem->sendEvent_ = false;
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
		unsigned threadIndex = workload->threadIndex;
		VoxelWorkSlot* slot = workload->slot;
		VoxelJob* job = slot->job;
		VoxelDefinition* definition = job->voxelDefinition;

		stbvox_mesh_maker* mm = &slot->meshMakers[threadIndex];
		stbvox_set_input_stride(mm, definition->xStride, definition->zStride);
		
		stbvox_input_description *map;
		map = stbvox_get_input_description(mm);
		map->blocktype = definition->GetAddress(0,0,0);
		map->block_tex1_face = &definition->blockTex1Face.Front();
		map->block_tex2_face = &definition->blockTex2Face.Front();
		map->block_geometry = &definition->blockGeometry.Front();
		map->block_vheight = &definition->blockVHeight.Front();
		map->geometry = &definition->geometry.Front();

		stbvox_reset_buffers(mm);
		stbvox_set_buffer(mm, 0, 0, slot->workBuffers[threadIndex], VOXEL_WORKER_BUFFER_SIZE);

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
				Min(y+16, workload->end[1])
			);
			if (stbvox_make_mesh(mm) == 0)
			{
				success = false;
				break;
			}
		}
		workload->numQuads = stbvox_get_quad_count(mm, 0);
		return success;
	}

	bool VoxelBuilder::UploadGpuData(VoxelWorkSlot* slot, bool append)
	{
		VoxelJob* job = slot->job;
		VoxelChunk* chunk = job->chunk;
		VertexBuffer* vb = chunk->GetVertexBuffer();
		int totalVertices = slot->numQuads * 4;
		if (totalVertices == 0)
			return true;

		if (!vb->SetSize(totalVertices, MASK_POSITION | MASK_TEXCOORD1 | MASK_NORMAL, true))
			return false;

		int end = 0;
		int start = 0;
		for (unsigned i = 0; i < slot->workloads.Size(); ++i)
		{
			VoxelWorkload* workload = slot->workloads[i];
			start = end;
			end = start + workload->numQuads * 4;
			if (!vb->SetDataRange(&workload->gpuData.Front(), start, workload->numQuads * 4))
				return false;
		}

		ResourceCache* cache = GetSubsystem<ResourceCache>();
		chunk->SetBoundingBox(slot->box);
		chunk->SetMaterial(cache->GetResource<Material>("Materials/GreenSquare.xml"));
		chunk->SetCastShadows(true);

		{
			Geometry* geo = chunk->GetGeometry();
			const unsigned char* vertexData = 0;
			const unsigned char* indexData = 0;
			unsigned vertexCount = 0;
			unsigned indexCount = 0;
			unsigned mask = 0;
			int indexEnd = end / 4 * 6; // 6 indexes per quad
			//geo->GetRawData(vertexData, vertexCount, indexData, indexCount, mask);
			//PODVector<unsigned char> cpuData(vertexData, vertexCount * 4);

			ResizeIndexBuffer(slot->numQuads);

			geo->SetIndexBuffer(sharedIndexBuffer_);
			if (!geo->SetDrawRange(TRIANGLE_LIST, 0, indexEnd, false))
				return false;
		}
	}

	void VoxelBuilder::ProcessSlot(VoxelWorkSlot* slot)
	{
		if (!slot->failed)
			UploadGpuData(slot);
		FreeWorkSlot(slot);
	}

	void VoxelBuilder::HandleWorkItemCompleted(StringHash eventType, VariantMap& eventData)
	{
		using namespace WorkItemCompleted;
		SharedPtr<WorkItem> workItem((WorkItem*)eventData[P_ITEM].GetPtr());
		if (workItem->workFunction_ != BuildWorkloadHandler)
			return;

		VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
		VoxelWorkSlot* slot = workload->slot;
		ProcessSlot(slot);
		RunJobs();
	}

	unsigned VoxelBuilder::DecrementWorkSlot(VoxelWorkSlot* slot, VoxelWorkload* workload)
	{
		MutexLock lock(slot->workMutex);
		return --slot->workCounter;
	}

	void VoxelBuilder::FreeWorkSlot(VoxelWorkSlot* slot)
	{
		slot->free = true;
		slot->failed = false;
		slot->workCounter = 0;
		slot->numQuads = 0;
		slot->box = BoundingBox();
		for (unsigned i = 0; i < slot->workloads.Size(); ++i)
			delete slot->workloads[i];

		slot->workloads.Clear();
		if (slot->job)
		{
			delete slot->job;
			slot->job = 0;
		}
	}

	VoxelWorkSlot* VoxelBuilder::GetFreeWorkSlot()
	{
		VoxelWorkSlot* slot = 0;
		for (unsigned i = 0; i < slots_.Size(); ++i)
		{
			if (slots_[i].free)
			{
				slot = &slots_[i];
				break;
			}
		}
		return slot;
	}

}
