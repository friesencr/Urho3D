#include "VoxelSet.h"
#include "Drawable.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "Renderer.h"
#include "Camera.h"
#include "OctreeQuery.h"
#include "Octree.h"
#include "Core/Profiler.h"
#include "VoxelBuilder.h"
#include "../Physics/CollisionShape.h"
#include "../Physics/RigidBody.h"

#include "../DebugNew.h"

namespace Urho3D {

VoxelSet::VoxelSet(Context* context) :
    Component(context),
    numChunks_(0),
    numChunksX_(0),
    numChunksY_(0),
    numChunksZ_(0),
    chunkXStride_(0),
    chunkZStride_(0)
{

}

VoxelSet::~VoxelSet()
{
}

void VoxelSet::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelSet>("Voxel");
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

void VoxelSet::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    for (unsigned i = 0; i < chunks_.Size(); ++i)
    {
        if (chunks_[i])
            chunks_[i]->SetEnabled(enabled);
    }
}

unsigned VoxelSet::GetChunkIndex(unsigned x, unsigned y, unsigned z) const
{
    return x * chunkXStride_ + y + z * chunkZStride_;
}

void VoxelSet::GetCoordinatesFromIndex(unsigned index, unsigned &x, unsigned &y, unsigned &z)
{
    x = index / chunkXStride_;
    z = (index - (x * chunkXStride_)) / chunkZStride_;
    y = index - (x * chunkXStride_) - (z * chunkZStride_);
}

VoxelChunk* VoxelSet::GetVoxelChunk(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return 0;
    return chunks_[GetChunkIndex(x, y, z)];
}

void VoxelSet::SetVoxelStore(VoxelStore* voxelStore)
{
    voxelStore_ = voxelStore;
    SetVoxelStoreInternal();
}

void VoxelSet::SetVoxelStoreInternal()
{
    if (!voxelStore_)
        return;

    // chunk settings
    numChunks_ = voxelStore_->GetNumChunks();
    numChunksX_ = voxelStore_->GetNumChunksX();
    numChunksY_ = voxelStore_->GetNumChunksY();
    numChunksZ_ =  voxelStore_->GetNumChunksZ();
    chunkZStride_ = numChunksY_;
    chunkXStride_ = numChunksY_ * numChunksZ_;

    chunkSpacing_ = Vector3(
        (float)VOXEL_STORE_CHUNK_SIZE_X,
        (float)VOXEL_STORE_CHUNK_SIZE_Y,
        (float)VOXEL_STORE_CHUNK_SIZE_Z
    );

    setBox = BoundingBox(Vector3(0.0, 0.0, 0.0), Vector3((float)numChunksX_, (float)numChunksY_, (float)numChunksZ_) * chunkSpacing_);
    chunks_.Resize(numChunks_);
}

VoxelChunk* VoxelSet::GetVoxelChunkAtPosition(Vector3 position)
{
    int x = 0;
    int y = 0;
    int z = 0;
    if (GetIndexFromWorldPosition(position, x, y, z))
        return chunks_[GetChunkIndex(x, y, z)];
    else
        return 0;
}

void VoxelSet::Build()
{
    BuildInternal();
}

void VoxelSet::LoadChunk(unsigned x, unsigned y, unsigned z, bool async)
{
    SharedPtr<VoxelMap> map;
    {
        PROFILE(LoadChunk);
        map = voxelStore_->GetVoxelMap(x, y, z);
        if (!map)
        {
            LOGERROR("Could not load chunk.  Missing voxel map");
            return;
        }
    }

    {
        PROFILE(BuildChunk);
        VoxelChunk* chunk = FindOrCreateVoxelChunk(x, y, z);
        VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
        builder->BuildVoxelChunk(chunk, map, async);
    }
}

void VoxelSet::UnloadChunk(unsigned x, unsigned y, unsigned z)
{
    unsigned index = GetChunkIndex(x, y, z);
    if (index >= numChunks_)
        return;

    VoxelChunk* voxelChunk = chunks_[index];
    if (voxelChunk)
    {
        Node* node = voxelChunk->GetNode();
        node->Remove();
        chunks_[index] = 0;
    }
}

void VoxelSet::BuildInternal()
{
    PROFILE(BuildInternal);

	if (numChunks_ == 0)
		return;

	int buildCounter = 0;
	VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();

    for (unsigned x = 0; x < numChunksX_; ++x)
    {
        for (unsigned z = 0; z < numChunksZ_; ++z)
        {
            for (unsigned y = 0; y < numChunksY_; ++y)
            {
                LoadChunk(x, y, z, false);
            }
        }
    }
}

bool VoxelSet::GetIndexFromWorldPosition(Vector3 worldPosition, int &x, int &y, int &z)
{
    Node* node = GetNode();
    if (!node)
        return false;

    Matrix3x4 inverse = node->GetWorldTransform().Inverse();
    Vector3 localPosition = inverse * worldPosition;
    Vector3 voxelPosition = localPosition / chunkSpacing_;
    x = (int)voxelPosition.x_;
    y = (int)voxelPosition.y_;
    z = (int)voxelPosition.z_;
    return true;
}

VoxelChunk* VoxelSet::FindOrCreateVoxelChunk(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return 0;

    unsigned index = GetChunkIndex(x, y, z);
    VoxelChunk* chunk = chunks_[index];
    if (chunk)
        return chunk;

    Node* chunkNode = GetNode()->CreateChild();
    chunkNode->SetPosition(Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
    chunk = chunkNode->CreateComponent<VoxelChunk>();
    chunk->SetIndex(x, y, z);
    chunk->SetSize((unsigned char)chunkSpacing_.x_, (unsigned char)chunkSpacing_.y_, (unsigned char)chunkSpacing_.z_);
    //chunk->SetCastShadows(false);
	//CollisionShape* cs = chunkNode->CreateComponent<CollisionShape>();
	//cs->SetVoxelTriangleMesh(true);
    chunks_[index] = chunk;
    return chunk;
}

}
