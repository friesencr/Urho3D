#include "VoxelSet.h"
#include "Drawable.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"

#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE  0
#include <STB/stb_voxel_render.h>

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY;

VoxelSet::VoxelSet(Context* context) :
Component(context),
sharedIndexBuffer_(new IndexBuffer(context))
{
	sharedIndexBuffer_->SetShadowed(true);
}

VoxelSet::~VoxelSet()
{

}

void VoxelSet::RegisterObject(Context* context)
{
	context->RegisterFactory<VoxelSet>(GEOMETRY_CATEGORY);
}

void VoxelSet::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
	Serializable::OnSetAttribute(attr, src);

	// Change of any non-accessor attribute requires recreation of the terrain
	//if (!attr.accessor_)
	//    recreateTerrain_ = true;
}

void VoxelSet::ApplyAttributes()
{
	//if (recreateTerrain_)
	//    CreateGeometry();
}

void VoxelSet::AllocateWorkerBuffers()
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();

	// the workers will build a chunk and then convert it to urho vertex buffers, while the vertex buffer is being built it will start more chunks
	// but stall if the mesh building gets too far ahead of the vertex converter this is based on chunk size and number of cpu threads
	unsigned numSlots = (unsigned)Min((float)numChunks, Max(1.0f, ceil((float)queue->GetNumThreads() / (float)VOXEL_MAX_NUM_WORKERS_PER_CHUNK)) * 2.0f);
	workSlots_.Resize(numSlots);
}

void VoxelSet::FreeWorkerBuffers()
{
	workSlots_.Clear();
}

void VoxelSet::InitializeIndexBuffer()
{
	PODVector<int> data(VOXEL_CHUNK_SIZE * 6);
	// triangulates the quads
	for (int i = 0; i < (int)VOXEL_CHUNK_SIZE; i++)
	{
		data[i * 6] = i * 6;
		data[i * 6 + 1] = i * 6 + 1;
		data[i * 6 + 2] = i * 6 + 2;
		data[i * 6 + 3] = i * 6 + 2;
		data[i * 6 + 4] = i * 6 + 3;
		data[i * 6 + 5] = i * 6;
	}
	sharedIndexBuffer_ = new IndexBuffer(context_);
	sharedIndexBuffer_->SetSize(VOXEL_CHUNK_SIZE * 6, true, false);
	sharedIndexBuffer_->SetData(&data.Front());
}

// Decode to vertex buffer
void VoxelSet::DecodeWorkBuffer(VoxelWorkSlot* workSlot, VoxelChunk* chunk)
{
	if (sharedIndexBuffer_->GetIndexSize() == 0)
		InitializeIndexBuffer();

	int vertexSize = sizeof(float) * 5; // position / uv1
	int numVertices = chunk->numQuads_ * 4; // 4 verts in a quad
	int numIndices = chunk->numQuads_ / 4 * 6;

	SharedPtr<VertexBuffer> vb(chunk->GetVertexBuffer());
	SharedArrayPtr<unsigned char> cpuData(new unsigned char[numVertices * sizeof(Vector3)]);
	Geometry* geo = chunk->GetGeometry();
	vb->SetSize(chunk->numQuads_ * 4, MASK_POSITION | MASK_TEXCOORD1);
	float* vertexData = (float*)vb->Lock(0, vb->GetVertexCount());
	float* positionData = (float*)cpuData.Get();
	BoundingBox box(2.0, 3.0);

	const unsigned* workData = (unsigned*)workSlot->buffer;
	for (int i = 0; i < numVertices; ++i)
	{
		unsigned int v1 = *workData++;
		unsigned int v2 = *workData++;
		Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0, (float)((v1 >> 7u) & 127u));
		box.Merge(position);

		*vertexData++ = position.x_;
		*vertexData++ = position.y_;
		*vertexData++ = position.z_;
		*positionData++ = position.x_;
		*positionData++ = position.y_;
		*positionData++ = position.z_;

		if (i % 4 == 0)
		{
			*vertexData++ = 0.0;
			*vertexData++ = 0.0;
		}
		else if (i % 4 == 1)
		{
			*vertexData++ = 1.0;
			*vertexData++ = 0.0;
		}
		else if (i % 4 == 2)
		{
			*vertexData++ = 1.0;
			*vertexData++ = 1.0;
		}
		else if (i % 4 == 3)
		{
			*vertexData++ = 0.0;
			*vertexData++ = 1.0;
		}
	}

	vb->Unlock();
	chunk->SetBoundingBox(box);

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	chunk->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
	geo->SetIndexBuffer(sharedIndexBuffer_);
	geo->SetDrawRange(TRIANGLE_LIST, 0, numIndices, false);
	geo->SetRawVertexData(cpuData, sizeof(Vector3), MASK_POSITION);
}

void BuildChunkWorkHandler(const WorkItem* workItem, unsigned threadIndex)
{
	VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
	workload->workSlot_->set_->BuildChunkWork(workload, threadIndex);
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
	if (!(height_ > 0 && width_ > 0 && depth_ > 0 && voxelDefinition_.NotNull()))
		return;

	numChunksX = (int)ceil((float)width_ / (float)VOXEL_CHUNK_SIZE_X);
	numChunksY = (int)ceil((float)height_ / (float)VOXEL_CHUNK_SIZE_Y);
	numChunksZ = (int)ceil((float)depth_ / (float)VOXEL_CHUNK_SIZE_Z);
	numChunks = numChunksX * numChunksY * numChunksZ;

	for (unsigned int x = 0; x < numChunksX; ++x)
	{
		for (unsigned int z = 0; z < numChunksZ; ++z)
		{
			for (unsigned int y = 0; y < numChunksY; ++y)
			{
				SharedPtr<Node> chunkNode(node_->CreateChild("VoxelChunk_" + String(x) + "_" + String(y) + "_" + String(z)));
				chunkNode->SetTemporary(true);
				chunkNode->SetPosition(Vector3((float)(x * VOXEL_CHUNK_SIZE_X), (float)(y * VOXEL_CHUNK_SIZE_Z), (float)(z * VOXEL_CHUNK_SIZE_Z)));
				VoxelChunk* chunk = chunkNode->CreateComponent<VoxelChunk>();
				chunk->SetSize(
					Min((int)width_, x * VOXEL_CHUNK_SIZE_X + VOXEL_CHUNK_SIZE_X) - x * VOXEL_CHUNK_SIZE_X, 
					Min((int)height_, y * VOXEL_CHUNK_SIZE_Y + VOXEL_CHUNK_SIZE_Y) - y * VOXEL_CHUNK_SIZE_Y, 
					Min((int)depth_, z * VOXEL_CHUNK_SIZE_Z + VOXEL_CHUNK_SIZE_Z) - z * VOXEL_CHUNK_SIZE_Z
				);
				chunk->SetIndex(x, y, z);
				chunks_.Push(WeakPtr<VoxelChunk>(chunk));
			}
		}
	}

	AllocateWorkerBuffers();
	// sort chunks
	for (unsigned i = 0; i < chunks_.Size(); ++i)
		if (!BuildChunk(chunks_[i]))
			return;
}

VoxelWorkSlot* VoxelSet::GetFreeWorkSlot()
{
	MutexLock lock(slotGetterMutex);
	VoxelWorkSlot* slot = 0;
	for (unsigned i = 0; i < workSlots_.Size(); ++i)
	{
		if (workSlots_[i].free)
		{
			slot = &workSlots_[i];
			slot->free = false;
			slot->numQuads = 0;
			slot->workCounter = 0;
			slot->workItems_.Clear();
			slot->chunk_ = 0;
		}
		break;
	}
	return slot;
}

bool VoxelSet::BuildChunk(VoxelChunk* chunk, bool async)
{
	WorkQueue* workQueue = GetSubsystem<WorkQueue>();
	VoxelWorkSlot* slot = GetFreeWorkSlot();
	slot->chunk_ = chunk;
	slot->set_ = this;

	if (slot == 0)
		return false;

	unsigned workloadsX = (unsigned)ceil((float)chunk->GetSizeX() / (float)VOXEL_WORKER_SIZE_X);
	unsigned workloadsY = (unsigned)ceil((float)chunk->GetSizeY() / (float)VOXEL_WORKER_SIZE_Y);
	unsigned workloadsZ = (unsigned)ceil((float)chunk->GetSizeZ() / (float)VOXEL_WORKER_SIZE_Z);
	for (unsigned x = 0; x < workloadsX; ++x)
	{
		for (unsigned y = 0; y < workloadsY; ++y)
		{
			for (unsigned z = 0; z < workloadsZ; ++z)
			{
				SharedPtr<WorkItem> workItem(new WorkItem());
				VoxelWorkload* workLoad = new VoxelWorkload();
				workItem->priority_ = M_MAX_UNSIGNED;
				workLoad->workSlot_ = slot;
				workLoad->indexX_ = x;
				workLoad->indexY_ = y;
				workLoad->indexZ_ = z;
				workLoad->sizeX_ = Min((int)chunk->GetSizeX(), x * VOXEL_WORKER_SIZE_X + VOXEL_WORKER_SIZE_X) - x * VOXEL_WORKER_SIZE_X;
				workLoad->sizeY_ = Min((int)chunk->GetSizeY(), y * VOXEL_WORKER_SIZE_Y + VOXEL_WORKER_SIZE_Y) - y * VOXEL_WORKER_SIZE_X;
				workLoad->sizeZ_ = Min((int)chunk->GetSizeZ(), z * VOXEL_WORKER_SIZE_Z + VOXEL_WORKER_SIZE_Z) - z * VOXEL_WORKER_SIZE_X;
				workItem->aux_ = workLoad;
				workItem->workFunction_ = BuildChunkWorkHandler;
				slot->workItems_.Push(workItem);
				slot->workCounter++;
				workQueue->AddWorkItem(workItem);
			}
		}
	}
	workQueue->Complete(M_MAX_UNSIGNED);
	int a = 1;
	chunk->numQuads_ = slot->numQuads;
	//stbvox_mesh_maker* mm = &meshMakers_[0];
	//stbvox_set_buffer(mm, 0, 0, slot->buffer, VOXEL_CHUNK_WORK_BUFFER_SIZE);
	//stbvox_set_input_range(
	//	mm,
	//	chunk->GetIndexX() * VOXEL_CHUNK_SIZE_X,
	//	chunk->GetIndexZ() * VOXEL_CHUNK_SIZE_Z,
	//	chunk->GetIndexY() * VOXEL_CHUNK_SIZE_Y,
	//	chunk->GetIndexX() * VOXEL_CHUNK_SIZE_X + chunk->GetSizeX(),
	//	chunk->GetIndexZ() * VOXEL_CHUNK_SIZE_Z + chunk->GetSizeZ(),
	//	chunk->GetIndexY() * VOXEL_CHUNK_SIZE_Y + chunk->GetSizeY()
	//);
	//stbvox_set_mesh_coordinates(mm, 
	//	chunk->GetIndexX() * VOXEL_CHUNK_SIZE_X, 
	//	chunk->GetSizeZ() * VOXEL_CHUNK_SIZE_Z, 
	//	chunk->GetSizeY() * VOXEL_CHUNK_SIZE_Y
	//);
	if (chunk->numQuads_ > 0)
	{
		DecodeWorkBuffer(slot, chunk);
	}

	FreeWorkSlot(slot);

	return true;
}


void VoxelSet::BuildChunkWork(VoxelWorkload* workload, unsigned threadIndex)
{
	VoxelWorkSlot* slot = workload->workSlot_;
	stbvox_input_description *map;
	stbvox_mesh_maker mm;
	stbvox_init_mesh_maker(&mm);

	stbvox_set_input_stride(&mm, width_+2*depth_+2, depth_+2);

	map = stbvox_get_input_description(&mm);
	map->blocktype = &voxelDefinition_->blocktype.Front();
	map->block_tex1_face = &voxelDefinition_->blockTex1Face.Front();
	map->block_tex2_face = &voxelDefinition_->blockTex2Face.Front();
	map->block_geometry = &voxelDefinition_->blockGeometry.Front();
	map->block_vheight = &voxelDefinition_->blockVHeight.Front();
	map->geometry = &voxelDefinition_->geometry.Front();

	stbvox_set_default_mesh(&mm, 0);
	stbvox_set_buffer(&mm, 0, 0, slot->buffer, VOXEL_CHUNK_WORK_BUFFER_SIZE);
	stbvox_set_input_range(
		&mm,
		workload->indexX_ * VOXEL_WORKER_SIZE_X + 1,
		workload->indexZ_ * VOXEL_WORKER_SIZE_Z + 1,
		workload->indexY_ * VOXEL_WORKER_SIZE_Y + 1,
		workload->indexX_ * VOXEL_WORKER_SIZE_X + workload->sizeX_,
		workload->indexZ_ * VOXEL_WORKER_SIZE_Z + workload->sizeZ_,
		workload->indexY_ * VOXEL_WORKER_SIZE_Y + workload->sizeY_
	);

	if (stbvox_make_mesh(&mm) == 0)
	{
		int a = 1;
	}

	if (DecrementWorkSlot(slot, workload) == 0)
		slot->numQuads = stbvox_get_quad_count(&mm, 0);
}

void VoxelSet::HandleWorkItemCompleted(StringHash eventType, VariantMap& eventData)
{
	using namespace WorkItemCompleted;
	SharedPtr<WorkItem> workItem((WorkItem*)eventData[P_ITEM].GetPtr());
	VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
}

void VoxelSet::SetDimensions(unsigned width, unsigned height, unsigned depth)
{
	height_ = height;
	width_ = width;
	depth_ = depth;
}

void VoxelSet::SetVoxelDefinition(VoxelDefinition* voxelDefinition)
{
	voxelDefinition_ = voxelDefinition;
}


unsigned VoxelSet::DecrementWorkSlot(VoxelWorkSlot* slot, VoxelWorkload* workload)
{
	MutexLock lock(slot->slotMutex);
	slot->workItems_.Remove(workload->workItem_);
	slot->workCounter--;
	delete workload;
	return slot->workCounter;
}

void VoxelSet::FreeWorkSlot(VoxelWorkSlot* slot)
{
	slot->free = true;
	slot->chunk_ = 0;
	slot->set_ = 0;
	slot->workCounter = 0;
	slot->workItems_.Clear();
}

}
