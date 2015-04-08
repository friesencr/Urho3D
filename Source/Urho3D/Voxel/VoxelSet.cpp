#include "VoxelSet.h"
#include "../../ThirdParty/STB/stb_voxel_render.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"

namespace Urho3D {

struct VoxelWorkload {
	SharedPtr<VoxelSet> set_;
	SharedPtr<VoxelChunk> chunk_;
	int numQuads;
	int index;
	char* work_buffer;
};


VoxelSet::VoxelSet(Context* context) :
	Component(context)
{
	WorkQueue* queue = GetSubsystem<WorkQueue>();
	mesh_makers_.Resize(queue->GetNumThreads());
	for (unsigned i = 0; i < mesh_makers_.Size(); ++i)
		stbvox_init_mesh_maker(&mesh_makers_[i]);
}


// Decode to vertex buffer
void DecodeVoxelWorkToVertexBuffer(VoxelWorkload* worker)
{

}

void BuildChunkWorkHandler(const WorkItem* item, unsigned threadIndex)
{
	VoxelWorkload* workLoad = (VoxelWorkload*)item->aux_;
	workLoad->set_->BuildChunkWork(workLoad->chunk_, threadIndex);
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

void VoxelSet::BuildChunk(VoxelChunk* chunk)
{
	WorkQueue* workQueue = GetSubsystem<WorkQueue>();
	SharedPtr<WorkItem> workItem(new WorkItem());
	VoxelWorkload* workLoad = new VoxelWorkload();
	workItem->aux_ = workLoad;
	workItem->workFunction_ = BuildChunkWorkHandler;
	workQueue->AddWorkItem(workItem);
}

void VoxelSet::SetVoxels(PODVector<Voxel> voxels)
{
	voxels_ = voxels;
}

void VoxelSet::SetVoxelDefinitions(Vector<VoxelDefinition> voxelDefinitions)
{
	for (unsigned i = 0; i < voxelDefinitions.Size(); i++)
	{
		voxelTextureMapping_[i][0] = voxelDefinitions[i].textureBlock.east;
		voxelTextureMapping_[i][1] = voxelDefinitions[i].textureBlock.north;
		voxelTextureMapping_[i][2] = voxelDefinitions[i].textureBlock.west;
		voxelTextureMapping_[i][3] = voxelDefinitions[i].textureBlock.south;
		voxelTextureMapping_[i][4] = voxelDefinitions[i].textureBlock.up;
		voxelTextureMapping_[i][5] = voxelDefinitions[i].textureBlock.down;
		voxelGeometries_[i] = STBVOX_MAKE_GEOMETRY(voxelDefinitions[i].type, voxelDefinitions[i].rotation, 0);
	}
}

void VoxelSet::BuildChunkWork(void* data, unsigned threadIndex)
{
	VoxelWorkload* workload = (VoxelWorkload*)data;
	stbvox_input_description *map;
	stbvox_mesh_maker mm = mesh_makers_[threadIndex];

	stbvox_set_input_stride(&mm, width_, depth_);

	map = stbvox_get_input_description(&mm);
	map->block_tex1_face = voxelTextureMapping_;
	map->block_geometry = voxelGeometries_;

	/* stbvox_reset_buffers(&rm->mm); */
	/* stbvox_set_buffer(&rm->mm, 0, 0, rm->build_buffer, BUILD_BUFFER_SIZE); */

	/* map->blocktype = &rm->sv_blocktype[1][1][1]; // this is (0,0,0), but we need to be able to query off the edges */
	/* map->lighting = &rm->sv_lighting[1][1][1]; */

	/* for (z=sizeY_- */

	/* for (z=256-16; z >= SKIP_TERRAIN; z -= 16) */
	/* { */
	/*     int z0 = z; */
	/*     int z1 = z+16; */
	/*     if (z1 == 256) z1 = 255; */

	/*     map->blocktype = &rm->sv_blocktype[1][1][1-z]; // specify location of 0,0,0 so that accessing z0..z1 gets right data */
	/*     map->lighting = &rm->sv_lighting[1][1][1-z]; */

	/*     { */
	/*         stbvox_set_input_range(&rm->mm, 0,0,z0, 32,32,z1); */
	/*         stbvox_set_default_mesh(&rm->mm, 0); */
	/*         stbvox_make_mesh(&rm->mm); */
	/*     } */
	/* } */

	/* stbvox_set_mesh_coordinates(&rm->mm, chunk_x*16, chunk_y*16, 0); */
	/* stbvox_get_transform(&rm->mm, rm->transform); */

	/* stbvox_set_input_range(&rm->mm, 0,0,0, 32,32,255); */
	/* stbvox_get_bounds(&rm->mm, rm->bounds); */

	/* rm->num_quads = stbvox_get_quad_count(&rm->mm, 0); */
}

}