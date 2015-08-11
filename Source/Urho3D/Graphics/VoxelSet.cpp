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

static const float BUILD_UNSET = 100000000.0;

inline bool CompareChunks(VoxelChunk* lhs, VoxelChunk* rhs)
{
    return lhs->GetBuildVisible() > rhs->GetBuildVisible() || lhs->GetBuildPriority() < rhs->GetBuildPriority();
}

VoxelSet::VoxelSet(Context* context) :
    Component(context),
	numChunks(0),
	numChunksX(0),
	numChunksY(0),
	numChunksZ(0),
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

void VoxelSet::RemoveChunk(unsigned x, unsigned y, unsigned z)
{

}

inline unsigned VoxelSet::GetIndex(unsigned x, unsigned y, unsigned z)
{
    return x * chunkXStride + y + z * chunkZStride;
}

VoxelMap* VoxelSet::GetVoxelMap(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return 0;

    unsigned index = GetIndex(x, y, z);

    VoxelMap* voxelMap = voxelMaps_[index];
    if (voxelMap)
        return voxelMap;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    const String voxelMapReference = voxelMapResourceRefList_.names_[index];
    return cache->GetResource<VoxelMap>(voxelMapReference);
}

VoxelChunk* VoxelSet::GetVoxelChunk(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return 0;
    return chunks_[GetIndex(x, y, z)];
}

void VoxelSet::SetNumberOfChunks(unsigned x, unsigned y, unsigned z)
{
    numChunks = x * y * z;
    numChunksX = x;
    numChunksY = y;
    numChunksZ = z;
    chunkZStride = y;
    chunkXStride = y * z;
    setBox = BoundingBox(Vector3(0.0, 0.0, 0.0), Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
    chunks_.Resize(numChunks);
    voxelMaps_.Resize(numChunks);
    voxelMapResourceRefList_.names_.Resize(numChunks);
    for (unsigned i = 0; i < numChunks; ++i)
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

void VoxelSet::BuildInternal()
{
    PROFILE(BuildInternal);

	if (numChunks == 0)
		return;

	int buildCounter = 0;
	VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();

    for (unsigned x = 0; x < numChunksX; ++x)
    {
        for (unsigned z = 0; z < numChunksZ; ++z)
        {
            for (unsigned y = 0; y < numChunksY; ++y)
            {
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
                    if (index >= numChunks)
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
                    {
                        cache->BackgroundLoadResource<VoxelMap>(voxelMapReference);
                        resourceRefLoads[i] = true;
                    }
                }

                for (unsigned i = 0; i < 5; ++i)
                {
                    if (resourceRefLoads[i])
                        maps[i] = cache->GetResource<VoxelMap>(voxelMapResourceRefList_.names_[indexes[i]]);
                }

                if (maps[0])
                {
                    VoxelChunk* chunk = FindOrCreateVoxelChunk(x, y, z, maps[0]);
                    builder->BuildVoxelChunk(chunk, maps[0], maps[1], maps[2], maps[3], maps[4], true);
                    if (++buildCounter % 2 == 0)
                        builder->CompleteWork();
                }
            }
        }
    }
    builder->CompleteWork();
}

bool VoxelSet::GetIndexFromWorldPosition(Vector3 worldPosition, int &x, int &y, int &z)
{
    Node* node = GetNode();
    if (!node)
        return false;

    Matrix3x4 inverse = node->GetWorldTransform().Inverse();
    Vector3 localPosition = inverse * worldPosition;
    Vector3 voxelPosition = localPosition / chunkSpacing_;
    x = (int)floorf(voxelPosition.x_);
    y = (int)floorf(voxelPosition.y_);
    z = (int)floorf(voxelPosition.z_);
    return true;
}

VoxelChunk* VoxelSet::FindOrCreateVoxelChunk(unsigned x, unsigned y, unsigned z, VoxelMap* map)
{
    VoxelChunk* chunk = GetVoxelChunk(x, y, z);
    if (chunk)
        return chunk;

    Node* chunkNode = GetNode()->CreateChild();
    chunkNode->SetPosition(Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
    chunk = chunkNode->CreateComponent<VoxelChunk>();
    chunk->SetIndex(x, y, z);
    chunk->SetSize(64, 128, 64);
    //chunk->SetCastShadows(false);
    chunk->buildPrioirty_ = BUILD_UNSET;
	//CollisionShape* cs = chunkNode->CreateComponent<CollisionShape>();
	//cs->SetVoxelTriangleMesh(true);
    chunks_[GetIndex(x, y, z)] = chunk;
    return chunk;
}

void VoxelSet::SetVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return;

    unsigned index = GetIndex(x, y, z);

    voxelMaps_[index] = voxelMap;
}

void VoxelSet::SetVoxelMapResourceRefList(const ResourceRefList& resourceRefList)
{
    voxelMapResourceRefList_ = resourceRefList;
    voxelMapResourceRefList_.names_.Resize(numChunks);
}

void VoxelSet::SetVoxelMapResource(unsigned x, unsigned y, unsigned z, const String& name)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return;

    unsigned index = GetIndex(x, y, z);
    voxelMapResourceRefList_.names_[index] = name;
}



}
