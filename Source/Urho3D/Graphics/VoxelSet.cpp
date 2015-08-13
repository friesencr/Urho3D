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

extern const char* GEOMETRY_CATEGORY;

VoxelSet::VoxelSet(Context* context) :
    Component(context),
	numChunks_(0),
	numChunksX_(0),
	numChunksY_(0),
	numChunksZ_(0),
    chunkSpacing_(Vector3(64.0, 128.0, 64.0))
{
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

void VoxelSet::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    for (unsigned i = 0; i < chunks_.Size(); ++i)
    {
        if (chunks_[i])
            chunks_[i]->SetEnabled(enabled);
    }
}

unsigned VoxelSet::GetIndex(unsigned x, unsigned y, unsigned z) const
{
    return x * chunkXStride_ + y + z * chunkZStride_;
}

void VoxelSet::GetCoordinatesFromIndex(unsigned index, unsigned &x, unsigned &y, unsigned &z)
{
    x = index / chunkXStride_;
    z = (index - (x * chunkXStride_)) / chunkZStride_;
    y = index - (x * chunkXStride_) - (z * chunkZStride_);
}

VoxelMap* VoxelSet::GetVoxelMap(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return 0;

    unsigned index = GetIndex(x, y, z);

    VoxelMap* voxelMap = voxelMaps_[index];
    if (voxelMap)
        return voxelMap;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    const String& voxelMapReference = voxelMapResourceRefList_.names_[index];
    return cache->GetResource<VoxelMap>(voxelMapReference);
}

VoxelChunk* VoxelSet::GetVoxelChunk(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return 0;
    return chunks_[GetIndex(x, y, z)];
}

void VoxelSet::SetNumberOfChunks(unsigned x, unsigned y, unsigned z)
{
    numChunks_ = x * y * z;
    numChunksX_ = x;
    numChunksY_ = y;
    numChunksZ_ = z;
    chunkZStride_ = y;
    chunkXStride_ = y * z;
    setBox = BoundingBox(Vector3(0.0, 0.0, 0.0), Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
    chunks_.Resize(numChunks_);
    voxelMaps_.Resize(numChunks_);
    voxelMapResourceRefList_.names_.Resize(numChunks_);
    for (unsigned i = 0; i < numChunks_; ++i)
    {
        chunks_[i] = 0;
        voxelMaps_[i] = 0;
    }
}

void VoxelSet::SetChunkSpacing(Vector3 chunkSpacing)
{
    chunkSpacing_ = chunkSpacing;
}

VoxelChunk* VoxelSet::GetVoxelChunkAtPosition(Vector3 position)
{
    int x = 0;
    int y = 0;
    int z = 0;
    if (GetIndexFromWorldPosition(position, x, y, z))
        return chunks_[GetIndex(x, y, z)];
    else
        return 0;
}


void VoxelSet::Build()
{
    BuildInternal();
}

void VoxelSet::LoadChunk(unsigned x, unsigned y, unsigned z)
{
    PROFILE(LoadChunk);
	VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
    Vector<SharedPtr<VoxelMap> > maps(5);
    unsigned indexes[5] = {
        GetIndex(x    ,y,z    ),
        GetIndex(x    ,y,z + 1),
        GetIndex(x    ,y,z - 1),
        GetIndex(x + 1,y,z    ),
        GetIndex(x - 1,y,z    ),
    };
    bool resourceRefLoads[5];
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    for (unsigned i = 0; i < 5; ++i)
    {
        resourceRefLoads[i] = false;
        unsigned index = indexes[i];
        if (index >= numChunks_)
        {
            maps[i] = 0;
            continue;
        }

        VoxelMap* voxelMap = voxelMaps_[index];
        if (voxelMap)
        {
            maps[i] = voxelMap;
            continue;
        }

        const String voxelMapReference = voxelMapResourceRefList_.names_[index];
        if (!voxelMapReference.Empty())
            maps[i] = cache->GetResource<VoxelMap>(voxelMapResourceRefList_.names_[indexes[i]]);
    }

    if (maps[0])
    {
        VoxelChunk* chunk = FindOrCreateVoxelChunk(x, y, z);
        builder->BuildVoxelChunk(chunk, maps[0], maps[1], maps[2], maps[3], maps[4], false);
    }
}

void VoxelSet::UnloadChunk(unsigned x, unsigned y, unsigned z)
{
    VoxelChunk* voxelChunk = GetVoxelChunk(x, y, z);
    if (voxelChunk)
    {
        Node* node = voxelChunk->GetNode();
        node->Remove();
        chunks_[GetIndex(x, y, z)] = 0;
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
                LoadChunk(x, y, z);
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
    VoxelChunk* chunk = GetVoxelChunk(x, y, z);
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
    chunks_[GetIndex(x, y, z)] = chunk;
    return chunk;
}

void VoxelSet::SetVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return;

    unsigned index = GetIndex(x, y, z);
    voxelMaps_[index] = voxelMap;
}

void VoxelSet::SetVoxelMapResourceRefList(const ResourceRefList& resourceRefList)
{
    voxelMapResourceRefList_ = resourceRefList;
    voxelMapResourceRefList_.names_.Resize(numChunks_);
}

void VoxelSet::SetVoxelMapResource(unsigned x, unsigned y, unsigned z, const String& name)
{
    if (x >= numChunksX_ || y >= numChunksY_ || z >= numChunksZ_)
        return;

    unsigned index = GetIndex(x, y, z);
    voxelMapResourceRefList_.names_[index] = name;
}



}
