#include "VoxelSet.h"
#include "../../ThirdParty/STB/stb_voxel_render.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"
#include "../Graphics/Drawable.h"

#define STBVOX_CONFIG_MODE  0
#define STBVOX_CONFIG_DISABLE_TEX2
#define STB_VOXEL_RENDER_IMPLEMENTATION

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY = "Geometry";

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

void VoxelSet::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelSet>(GEOMETRY_CATEGORY);
}

void VoxelSet::AllocateWorkerBuffers()
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();

	// the workers will build a chunk and then convert it to urho vertex buffers, while the vertex buffer is being built it will start more chunks
	// but stall if the mesh building gets too far ahead of the vertex converter this is based on chunk size and number of cpu threads
	int numSlots = Min(numChunks, Max(1.0, ceil((float)queue->GetNumThreads() / (float)VOXEL_MAX_NUM_WORKERS_PER_CHUNK)) * 2.0);
	workSlots_.Resize(numSlots);
}

void VoxelSet::FreeWorkerBuffers()
{
	for (int i = 0; i < workSlots_.Size(); ++i)
		delete workSlots_[i].buffer;
}

// Decode to vertex buffer
void VoxelSet::DecodeWorkBuffer(VoxelWorkSlot* workSlot)
{
	VoxelChunk* chunk = workSlot->chunk;
	SharedPtr<VertexBuffer> vb(new VertexBuffer(context_));
	vb->SetSize(chunk->numQuads_, MASK_POSITION, true);
	
	if (sharedIndexBuffer_.Null())
	{
	}
//const unsigned VertexBuffer::elementSize[] =
//{
//    3 * sizeof(float), // Position
//    3 * sizeof(float), // Normal
//    4 * sizeof(unsigned char), // Color
//    2 * sizeof(float), // Texcoord1
//    2 * sizeof(float), // Texcoord2
//    3 * sizeof(float), // Cubetexcoord1
//    3 * sizeof(float), // Cubetexcoord2
//    4 * sizeof(float), // Tangent
//    4 * sizeof(float), // Blendweights
//    4 * sizeof(unsigned char), // Blendindices
//    4 * sizeof(float), // Instancematrix1
//    4 * sizeof(float), // Instancematrix2
//    4 * sizeof(float) // Instancematrix3
//};
	
	union PositionData {
		float f;
		unsigned char c;
	};
	int quadSize = VertexBuffer::elementSize[0]; //+ VertexBuffer::elementSize
	unsigned char* data = new unsigned char[chunk->numQuads_ * quadSize];
	for (int i = 0; i < chunk->numQuads_; i+=2)
	{
		unsigned int d1 = workSlot->buffer[i];
		unsigned int d2 = workSlot->buffer[i+1];
		PositionData p;
		p.f = *Vector3(d1 & 127u, (d1 << 14u) & 511u, (d1 >> 14u) & 127u).Data();
		data[i * quadSize] = p.c;
	}
	vb->SetData(data);

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
	numChunksX = ceil((float)width_ / (float)VOXEL_CHUNK_SIZE_X);
	numChunksY = ceil((float)height_ / (float)VOXEL_CHUNK_SIZE_Y);
	numChunksZ = ceil((float)depth_ / (float)VOXEL_CHUNK_SIZE_Z);
	numChunks = numChunksX * numChunksY * numChunksZ;

	int sizeX = width_;
	for (unsigned int x = 0; x < numChunksX; ++x)
	{
		int sizeZ = depth_;
		for (unsigned int z = 0; z < numChunksZ; ++z)
		{
			int sizeY = height_;
			for (unsigned int y = 0; y < numChunksY; ++y)
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
		chunk->numQuads_ = stbvox_get_quad_count(&mm, workload->chunk_->GetID());
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


int VoxelWorkSlot::DecrementWork(SharedPtr<WorkItem> workItem)
{
		MutexLock lock(slotMutex);
		workItems_.Remove(workItem);
		return workItems_.Size();
}

}