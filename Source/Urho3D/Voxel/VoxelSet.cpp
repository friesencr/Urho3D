#include "VoxelSet.h"
#include "../../ThirdParty/STB/stb_voxel_render.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"

#define STBVOX_CONFIG_MODE  0
#define STBVOX_CONFIG_DISABLE_TEX2
#define STB_VOXEL_RENDER_IMPLEMENTATION


namespace Urho3D {

struct VoxelWorkload {
	SharedPtr<VoxelSet> set_;
	SharedPtr<VoxelChunk> chunk_;
	SharedPtr<WorkItem> workItem_;
	VoxelWorkSlot* workSlot_;
	int chunkX_;
	int chunkY_;
	int chunkZ_;
	int sizeX_;
	int sizeY_;
	int sizeZ_;
};


VoxelSet::VoxelSet(Context* context) :
	Component(context)
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();
	meshMakers_.Resize(queue->GetNumThreads());
	for (int i = 0; i < meshMakers_.Size(); ++i)
		stbvox_init_mesh_maker(&meshMakers_[i]);
}

void VoxelSet::AllocateWorkerBuffers()
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();

	// the workers will build a chunk and then convert it to urho vertex buffers, while the vertex buffer is being built it will start more chunks
	// but stall if the mesh building gets too far ahead of the vertex converter this is based on chunk size and number of cpu threads
	int numSlots = Min(1.0, ceil((float)queue->GetNumThreads() / (float)VOXEL_MAX_NUM_WORKERS_PER_CHUNK)) * 2.0;
	workSlots_.Resize(numSlots);
	for (int i = 0; i < numSlots; ++i)
		workSlots_[i].buffer = new char[VOXEL_CHUNK_WORK_BUFFER_SIZE];
}

void VoxelSet::FreeWorkerBuffers()
{
	for (int i = 0; i < workSlots_.Size(); ++i)
		delete workSlots_[i].buffer;
}

// Decode to vertex buffer
void VoxelSet::DecodeWorkBuffer(VoxelWorkSlot* workSlot)
{
	workSlots_->numQuads_ = stbvox_get_quad_count(&mm, chunk->GetID());

	for (int i = 0; i < workSlot->
	workSlot->buffer
	
	" ((stbvox_uint32)((x)+((y) << 7) + ((z) << 14) + ((ao) << 23) + ((texlerp) << 29)))

		"   vec3 offset;\n"
		"   offset.x = float( (attr_vertex       ) & 127u );\n"             // a[0..6]
		"   offset.y = float( (attr_vertex >>  7u) & 127u );\n"             // a[7..13]
		"   offset.z = float( (attr_vertex >> 14u) & 511u );\n"             // a[14..22]
		"   amb_occ  = float( (attr_vertex >> 23u) &  63u ) / 63.0;\n"      // a[23..28]
		"   texlerp  = float( (attr_vertex >> 29u)        ) /  7.0;\n"      // a[29..31]

		"   vnormal = normal_table[(facedata.w>>2u) & 31u];\n"
		"   voxelspace_pos = offset * transform[0];\n"  // mesh-to-object scale
		"   vec3 position  = voxelspace_pos + transform[1];\n"  // mesh-to-object translate
}

void BuildChunkWorkHandler(const WorkItem* workItem, unsigned threadIndex)
{
	VoxelWorkload* workLoad = (VoxelWorkload*)workItem->aux_;
	workLoad->set_->BuildChunkWork(workLoad, threadIndex);
}

void VoxelSet::OnSetEnabled()
{
	using namespace SceneUpdate;
	Scene* scene = GetScene();
	if (IsEnabled())
		SubscribeToEvent(scene, E_SCENEUPDATE, HANDLER(VoxelSet, HandleSceneUpdate));
	else
		UnsubscribeFromEvent(scene, E_SCENEUPDATE);
}

void VoxelSet::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{

}

void VoxelSet::BuildGeometry()
{
	int chunksX = ceil((float)width_ / (float)VOXEL_CHUNK_SIZE_X);
	int chunksY = ceil((float)height_ / (float)VOXEL_CHUNK_SIZE_Y);
	int chunksZ = ceil((float)depth_ / (float)VOXEL_CHUNK_SIZE_Z);
	int sizeX = width_;
	for (unsigned int x = 0; x < chunksX; ++x)
	{
		int sizeZ = depth_;
		for (unsigned int z = 0; z < chunksZ; ++z)
		{
			int sizeY = height_;
			for (unsigned int y = 0; y < chunksY; ++y)
			{
				SharedPtr<Node> chunkNode(node_->CreateChild("VoxelChunk_" + String(x) + "_" + String(y) + "_" + String(z)));
				chunkNode->SetTemporary(true);
				chunkNode->SetPosition(Vector3(width_ - sizeX, height_ - sizeY, depth_ - sizeZ));

				SharedPtr<VoxelChunk> chunk(chunkNode->CreateComponent<VoxelChunk>());
				chunk->SetSize(Min(sizeX, VOXEL_CHUNK_SIZE_X), Min(sizeY, VOXEL_CHUNK_SIZE_Y), Min(sizeZ, VOXEL_CHUNK_SIZE_Z));
				chunk->SetIndex(width_ - sizeX, height_ - sizeY, depth_ - sizeZ);
				chunks_.Push(chunk);

				sizeZ -= VOXEL_CHUNK_SIZE_Z;
			}
			sizeY -= VOXEL_CHUNK_SIZE_Y;
		}
		sizeX -= VOXEL_CHUNK_SIZE_X;
	}

	AllocateWorkerBuffers();
	// sort chunks
	for (int i = 0; i < chunks_.Size(); ++i)
		if (!BuildChunk(chunks_[i]))
			return;
}

VoxelWorkSlot* VoxelSet::GetFreeWorkSlot(VoxelChunk* chunk)
{
	MutexLock lock(slotGetterMutex);
	VoxelWorkSlot* slot = 0;
	for (int i = 0; i < workSlots_.Size(); ++i)
	{
		if (workSlots_[i].chunk == 0)
		{
			slot = &workSlots_[i];
			slot->chunk = chunk;
		}
		break;
	}
	return slot;
}

bool VoxelSet::BuildChunk(VoxelChunk* chunk, bool async)
{
	WorkQueue* workQueue = GetSubsystem<WorkQueue>();
	VoxelWorkSlot* slot = GetFreeWorkSlot(chunk);

	if (slot == 0)
		return false;

	int workloadsX = ceil((float)chunk->GetSizeX() / (float)VOXEL_WORKER_SIZE_X);
	int workloadsY = ceil((float)chunk->GetSizeY() / (float)VOXEL_WORKER_SIZE_Y);
	int workloadsZ = ceil((float)chunk->GetSizeZ() / (float)VOXEL_WORKER_SIZE_Z);
	int sizeX = chunk->GetSizeX();
	for (unsigned int x = 0; x < workloadsX; ++x)
	{
		int sizeY = chunk->GetSizeY();
		for (unsigned int y = 0; y < workloadsY; ++y)
		{
			int sizeZ = chunk->GetSizeZ();
			for (unsigned int z = 0; z < workloadsZ; ++z)
			{
				SharedPtr<WorkItem> workItem(new WorkItem());
				VoxelWorkload* workLoad = new VoxelWorkload();
				workLoad->workItem_ = workItem;
				workLoad->chunk_ = chunk;
				workLoad->set_ = this;
				workLoad->chunkX_ = x;
				workLoad->chunkY_ = y;
				workLoad->chunkZ_ = z;
				workLoad->sizeX_ = sizeX;
				workLoad->sizeY_ = sizeY;
				workLoad->sizeZ_ = sizeZ;
				workItem->aux_ = workLoad;
				workItem->workFunction_ = BuildChunkWorkHandler;
				slot->workItems_.Push(workItem);
				workQueue->AddWorkItem(workItem);
				sizeY -= VOXEL_WORKER_SIZE_Z;
			}
			sizeY -= VOXEL_WORKER_SIZE_Y;
		}
		sizeX -= VOXEL_WORKER_SIZE_X;
	}
	workQueue->Complete(0);
	return true;
}


void VoxelSet::BuildChunkWork(void* data, unsigned threadIndex)
{
	VoxelWorkload* workload = (VoxelWorkload*)data;
	VoxelWorkSlot* slot = workload->workSlot_;
	stbvox_input_description *map;
	stbvox_mesh_maker mm = meshMakers_[threadIndex];

	stbvox_set_input_stride(&mm, width_, depth_);

	map = stbvox_get_input_description(&mm);
	map->block_tex1_face = voxelDefinition_->blockTex1Face;
	map->block_tex2_face = voxelDefinition_->blockTex2Face;
	map->block_geometry = voxelDefinition_->blockGeometry;
	map->geometry = voxelDefinition_->geometry;

	stbvox_set_buffer(&mm, workload->chunk_->GetID(), 0, slot->buffer, VOXEL_CHUNK_WORK_BUFFER_SIZE);
	stbvox_set_input_range(
		&mm,
		workload->chunkX_ * VOXEL_CHUNK_SIZE_X,
		workload->chunkY_ * VOXEL_CHUNK_SIZE_Y,
		workload->chunkZ_ * VOXEL_CHUNK_SIZE_Z,
		workload->chunkX_ * VOXEL_CHUNK_SIZE_X + workload->sizeX_,
		workload->chunkY_ * VOXEL_CHUNK_SIZE_Y + workload->sizeY_,
		workload->chunkZ_ * VOXEL_CHUNK_SIZE_Z + workload->sizeZ_
	);
	stbvox_make_mesh(&mm);

	if (workload->workSlot_->DecrementWork(workload->workItem_) == 0)
	{
		VoxelChunk* chunk = slot->chunk;
		slot->numQuads = stbvox_get_quad_count(&mm, workload->chunk_->GetID());
		stbvox_set_mesh_coordinates(&mm, chunk->GetSizeX(), chunk->GetSizeZ(), chunk->GetSizeY());
		stbvox_get_transform(&mm, chunk->transform_);
		stbvox_get_bounds(&mm, chunk->bounds_);
		DecodeWorkBuffer(workload->workSlot_);
		stbvox_reset_buffers(&mm);
		delete workload;
	}
}

void VoxelSet::HandleWorkItemCompleted(StringHash eventType, VariantMap& eventData)
{
	using namespace WorkItemCompleted;
	SharedPtr<WorkItem> workItem((WorkItem*)eventData[P_ITEM].GetPtr());
	VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
}

}