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

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY;

inline bool CompareChunks(VoxelChunk* lhs, VoxelChunk* rhs)
{
    if (lhs->GetBuildVisible() == rhs->GetBuildVisible())
        return lhs->GetBuildPriority() > rhs->GetBuildPriority();
    else
        return lhs->GetBuildVisible();
}

VoxelSet::VoxelSet(Context* context) :
    Component(context),
    maxInMemoryChunks_(M_MAX_UNSIGNED),
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

void VoxelSet::SetMaxInMemoryChunks(unsigned maxInMemoryChunks)
{
    maxInMemoryChunks_ = maxInMemoryChunks;
}

void VoxelSet::ApplyAttributes()
{
	//if (recreateTerrain_)
	//    CreateGeometry();
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
    BuildInternal(true);
}

void VoxelSet::CreateChunks(int indexX, int indexY, int indexZ, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance)
{
    PROFILE(CreateChunks);
    Node* voxelSetNode = GetNode();
    if (!voxelSetNode)
        return;

    unsigned startX = Max(0, indexX - size);
    unsigned startZ = Max(0, indexZ - size);
    unsigned endX = Max(0, indexX + size + 1);
    unsigned endZ = Max(0, indexZ + size + 1);
    
    if (endX == 0 || endZ == 0)
        return;

    for (unsigned x = startX; x < endX; ++x)
    {
        for (unsigned z = startZ; z < endZ; ++z)
        {
            BoundingBox chunkBox = BoundingBox(chunkSpacing_ * Vector3(x, 0, z), chunkSpacing_ * Vector3(x, 0, z) + chunkSpacing_);
            Vector3 position = chunkBox.Center();
            if ((cameraPosition - position).Length() > viewDistance)
                continue;

            if (!frustrum.IsInsideFast(chunkBox))
                continue;

            VoxelMap* voxelMap = GetVoxelMap(x, 0, z);
            if (voxelMap)
                FindOrCreateVoxelChunk(x, 0, z, voxelMap);
        }
    }
}

inline unsigned VoxelSet::GetIndex(unsigned x, unsigned y, unsigned z)
{
    return x * chunkXStride + y + z * chunkZStride;
}

bool VoxelSet::SetVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return false;

    voxelMaps_[GetIndex(x,y,z)] = voxelMap;
    return true;
}

VoxelMap* VoxelSet::GetVoxelMap(unsigned x, unsigned y, unsigned z)
{
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return 0;
    return voxelMaps_[GetIndex(x, y, z)];
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
    setBox = BoundingBox(Vector3(0, 0, 0), Vector3(x, y, z) * chunkSpacing_);
    chunks_.Resize(numChunks);
    for (unsigned i = 0; i < numChunks; ++i)
        chunks_[i] = 0;

    voxelMaps_.Resize(numChunks);
    for (unsigned i = 0; i < numChunks; ++i)
        voxelMaps_[i] = 0;
}

void VoxelSet::SetChunkSpacing(Vector3 chunkSpacing)
{
    chunkSpacing_ = chunkSpacing;
}

VoxelChunk* VoxelSet::GetVoxelChunkAtPosition(Vector3 position) const
{
    Node* node = GetNode();
    if (!node)
        return 0;

    Matrix3x4 inverse = node->GetWorldTransform().Inverse();
    Vector3 localPosition = inverse * position;
    Vector3 voxelPosition = localPosition / Vector3(numChunksX, numChunksY, numChunksZ);
}

void VoxelSet::Build()
{
    SubscribeToEvent(E_SCENEUPDATE, HANDLER(VoxelSet, HandleSceneUpdate));
    BuildInternal(false);

    for (unsigned x = 0; x < numChunksX; ++x)
    {
        for (unsigned z = 0; z < numChunksZ; ++z)
        {
            for (unsigned y = 0; y < numChunksY; ++y)
            {
                FindOrCreateVoxelChunk(x, y, z, GetVoxelMap(x, y, z));
            }
        }
    }
    VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
    builder->CompleteWork();
}

void VoxelSet::BuildAsync()
{
    SubscribeToEvent(E_SCENEUPDATE, HANDLER(VoxelSet, HandleSceneUpdate));
    BuildInternal(true);
}

void VoxelSet::BuildInternal(bool async)
{
    PROFILE(BuildInternal);

    AllocateAndSortVisibleChunks();

    return;

    //Sort(loadedChunks_.Begin(), loadedChunks_.End(), CompareChunks);
    while (loadedChunks_.Size() > maxInMemoryChunks_)
    {
        chunks_[GetIndex(loadedChunks_[0]->GetIndexX(), loadedChunks_[0]->GetIndexY(), loadedChunks_[0]->GetIndexZ())] = 0;
        loadedChunks_[0]->Unload();
        loadedChunks_.Erase(0);
    }

    if (!async)
        GetSubsystem<VoxelBuilder>()->CompleteWork();

    //for (unsigned i = 0; i < loadedChunks_.Size(); i++)
    //{
    //    VoxelChunk* chunk = loadedChunks_[i];
    //    if (chunk->buildStatus_ != VOXEL_BUILD_FRESH)
    //        continue;
    //    
    //    float priorty = chunk->GetBuildPriority();
    //    chunk->BuildAsync();
    //}
}

bool VoxelSet::GetIndexFromWorldPosition(Vector3 worldPosition, int &x, int &y, int &z)
{
    Node* node = GetNode();
    if (!node)
        return 0;

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
    PROFILE(FindOrCreateVoxelChunk);
    if (x >= numChunksX || y >= numChunksY || z >= numChunksZ)
        return 0;

    VoxelChunk* chunk = chunks_[GetIndex(x, y, z)];
    if (chunk)
        return chunk;

    //LOGINFO("CREATED CHUNK: " + String(GetIndex(x,y,z)) + "_" + String(x) + "_" + String(y) + "_" + String(z));
    Node* chunkNode = GetNode()->CreateChild();
    chunkNode->SetPosition(Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
    chunk = chunkNode->CreateComponent<VoxelChunk>();
    chunk->SetIndex(x, y, z);
    chunk->SetVoxelMap(map);
    VoxelMap* north = GetVoxelMap(x, 0, z+1);
    VoxelMap* south = GetVoxelMap(x, 0, z-1);
    VoxelMap* east  = GetVoxelMap(x+1, 0, z);
    VoxelMap* west  = GetVoxelMap(x-1, 0, z);
    if (north && !north->IsLoaded())
        north->Reload();
    if (south && !south->IsLoaded())
        south->Reload();
    if (east && !east->IsLoaded())
        east->Reload();
    if (west && !west->IsLoaded())
        west->Reload();

    chunk->SetNeighbors(north, south, east, west);
    chunks_[GetIndex(x, y, z)] = chunk;
    loadedChunks_.Push(chunk);
    chunk->BuildAsync();
    return chunk;
}

void VoxelSet::AllocateAndSortVisibleChunks()
{
    PROFILE(BuildSet);

    Scene* scene = GetScene();
    if (!scene)
        return;

    Octree* octree = scene->GetComponent<Octree>();
    if (!octree)
        LOGERROR("No Octree component in scene, voxel set frame logic bypassed.");

    Node* setNode = GetNode();
    if (!setNode)
        return;

    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();

    for (unsigned i = 0; i < viewports; ++i)
    {
        Viewport* viewport = renderer->GetViewport(i);
        Camera* camera = viewport->GetCamera();
        if (!camera)
            continue;

        Node* cameraNode = camera->GetNode();
        if (!cameraNode)
            continue;


        //if ((cameraNode->GetPosition() - setNode->GetPosition()).Length() > viewDistance)
        //    return;

        // make frustrum 1.5x as long as camera
        Frustum visibleTest;
        visibleTest.Define(camera->GetFov(), camera->GetAspectRatio(), camera->GetZoom(), 
            camera->GetNearClip(), camera->GetFarClip() * 1.5, camera->GetEffectiveWorldTransform());
        float viewDistance = camera->GetFarClip() * 1.5;

        int currentX = 0;
        int currentY = 0;
        int currentZ = 0;

        if (GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), currentX, currentY, currentZ))
        {
            unsigned radius = 20;
            CreateChunks(currentX, currentY, currentZ, radius, cameraNode->GetPosition(), visibleTest, viewDistance);
        }

        //for (unsigned i = 0; i < loadedChunks_.Size(); ++i)
        //{
        //    VoxelChunk* chunk = loadedChunks_[i];
        //    Node* chunkNode = chunk->GetNode();
        //    float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
        //    //chunk->buildprioirty_ = min(chunk->buildprioirty_, chunkdistance);
        //    //chunk->buildvisible_ = chunk->buildvisible_ || visibletest.isinsidefast(chunk->getboundingbox()) == inside;
        //}
        
        //for (unsigned c = 0; c < chunks_.Size(); ++c)
        //{
        //    VoxelChunk* chunk = chunks_[c];
        //    if (!chunk)
        //        continue;

        //    Node* chunkNode = chunk->GetNode();
        //    if (!chunkNode)
        //        continue;

        //    float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
        //    chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance);
        //    BoundingBox box = chunk->GetBoundingBox();

        //    if (visibleTest.IsInsideFast(box) == INSIDE)
        //        chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance / 100000.0);
        //}
    }
}

}
