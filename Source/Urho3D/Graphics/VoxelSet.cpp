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
    maxInMemoryMeshPeak_(4000),
    maxInMemoryMesh_(3950),
    maxInMemoryMapPeak_(1000),
    maxInMemoryMap_(950),
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
    maxInMemoryMap_ = maxInMemoryChunks;
}

void VoxelSet::SetMaxInMemoryChunksPeak(unsigned maxInMemoryChunksPeak)
{
    maxInMemoryMapPeak_ = maxInMemoryChunksPeak;
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
    if (loadedMaps_.Size() == chunks_.Size())
        return;

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
            BoundingBox chunkBox = BoundingBox(chunkSpacing_ * Vector3((float)x, 0.0, (float)z), chunkSpacing_ * Vector3((float)x, 0.0, (float)z) + chunkSpacing_);
            Vector3 position = chunkBox.Center();
            if ((cameraPosition - position).Length() > viewDistance)
                continue;

            if (frustrum.IsInsideFast(chunkBox) == OUTSIDE)
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
    setBox = BoundingBox(Vector3(0.0, 0.0, 0.0), Vector3((float)x, (float)y, (float)z) * chunkSpacing_);
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
    SubscribeToEvent(E_SCENEUPDATE, HANDLER(VoxelSet, HandleSceneUpdate));
    BuildInternal(false);

    for (unsigned x = 0; x < numChunksX; ++x)
    {
        for (unsigned z = 0; z < numChunksZ; ++z)
        {
            for (unsigned y = 0; y < numChunksY; ++y)
            {
                VoxelChunk* chunk = FindOrCreateVoxelChunk(x, y, z, GetVoxelMap(x, y, z));
                chunk->BuildAsync();
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

    if (loadedMaps_.Size() > maxInMemoryMapPeak_)
    {
        SortLoadedChunks();
        Sort(loadedMaps_.Begin(), loadedMaps_.End(), CompareChunks);
        while (loadedMaps_.Size() > maxInMemoryMap_)
        {
            VoxelChunk* chunk = loadedMaps_[maxInMemoryMap_];
            bool buildVisible = chunk->buildVisible_;
            float buildPriority = chunk->buildPrioirty_;
            chunks_[GetIndex(chunk->GetIndexX(), chunk->GetIndexY(), chunk->GetIndexZ())] = 0;
            chunk->Unload();
            chunk->GetNode()->Remove();
            loadedMaps_.Erase(maxInMemoryMap_);
            bool inBuild = buildQueue_.Remove(chunk);
            if (inBuild)
                bool a = true;
        }
    }

    for (unsigned i = 0; i < buildQueue_.Size(); ++i)
    {
        VoxelChunk* chunk = buildQueue_[i];
        if (chunk->GetSizeX() != 64)
            int a = 1;
    }

    for (PODVector<VoxelChunk*>::ConstIterator i = buildQueue_.Begin(); i != buildQueue_.End(); ++i)
    {
        VoxelChunk* chunk = *i;
        if (chunk->GetSizeX() != 64)
            int a = 1;
    }

    Sort(buildQueue_.Begin(), buildQueue_.End(), CompareChunks);
    while (buildQueue_.Size())
    {
        VoxelChunk* chunk = buildQueue_[0];
        VoxelMap* north = chunk->GetNeighborNorth();
        VoxelMap* south = chunk->GetNeighborSouth();
        VoxelMap* east = chunk->GetNeighborEast();
        VoxelMap* west = chunk->GetNeighborWest();
        if (north && !north->IsLoaded())
            north->Reload();
        if (south && !south->IsLoaded())
            south->Reload();
        if (east && !east->IsLoaded())
            east->Reload();
        if (west && !west->IsLoaded())
            west->Reload();
        chunk->BuildAsync();
        buildQueue_.Erase(0);
    }

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
    chunk->SetSize(64, 128, 64);
    chunk->SetVoxelMap(map);
    chunk->SetCastShadows(true);
    chunk->buildPrioirty_ = BUILD_UNSET;
    VoxelMap* north = GetVoxelMap(x, 0, z+1);
    VoxelMap* south = GetVoxelMap(x, 0, z-1);
    VoxelMap* east  = GetVoxelMap(x+1, 0, z);
    VoxelMap* west  = GetVoxelMap(x-1, 0, z);
    chunk->SetNeighbors(north, south, east, west);
    chunks_[GetIndex(x, y, z)] = chunk;
    loadedMaps_.Push(chunk);
    buildQueue_.Push(chunk);
    return chunk;
}

void VoxelSet::SortLoadedChunks()
{
    PROFILE(SortLoadedChunks);

    Scene* scene = GetScene();
    if (!scene)
        return;

    Node* setNode = GetNode();
    if (!setNode)
        return;

    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();

    for (unsigned i = 0; i < loadedMaps_.Size(); ++i)
    {
        loadedMaps_[i]->buildPrioirty_ = BUILD_UNSET;
        loadedMaps_[i]->buildVisible_ = false;
    }

    for (unsigned i = 0; i < viewports; ++i)
    {
        Viewport* viewport = renderer->GetViewport(i);
        Camera* camera = viewport->GetCamera();
        if (!camera)
            continue;

        Node* cameraNode = camera->GetNode();
        if (!cameraNode)
            continue;

        Frustum visibleTest = camera->GetFrustum();
        float viewDistance = camera->GetFarClip();

        for (unsigned i = 0; i < loadedMaps_.Size(); ++i)
        {
            VoxelChunk* chunk = loadedMaps_[i];
            Node* chunkNode = chunk->GetNode();
            float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
            chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance);
            chunk->buildVisible_ = chunk->buildVisible_ || visibleTest.IsInsideFast(chunk->GetBoundingBox()) != OUTSIDE;
        }
    }

}

void VoxelSet::AllocateAndSortVisibleChunks()
{
    PROFILE(BuildSet);

    Scene* scene = GetScene();
    if (!scene)
        return;

    Node* setNode = GetNode();
    if (!setNode)
        return;

    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();

    for (unsigned i = 0; i < buildQueue_.Size(); ++i)
    {
        buildQueue_[i]->buildPrioirty_ = BUILD_UNSET;
        buildQueue_[i]->buildVisible_ = false;
    }

    for (unsigned i = 0; i < viewports; ++i)
    {
        Viewport* viewport = renderer->GetViewport(i);
        if (!viewport)
            continue;

        Camera* camera = viewport->GetCamera();
        if (!camera)
            continue;

        Node* cameraNode = camera->GetNode();
        if (!cameraNode)
            continue;

        // make frustrum 1.2x as long as camera
        Frustum visibleTest;
        float viewDistance = camera->GetFarClip() * 1.2f;
        visibleTest.Define(camera->GetFov(), camera->GetAspectRatio(), camera->GetZoom(), 
            camera->GetNearClip(), viewDistance, camera->GetEffectiveWorldTransform());

        int currentX = 0;
        int currentY = 0;
        int currentZ = 0;

        if (GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), currentX, currentY, currentZ))
        {
            unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing_.x_, chunkSpacing_.z_));
            CreateChunks(currentX, currentY, currentZ, radius, cameraNode->GetPosition(), visibleTest, viewDistance);
        }

        for (unsigned i = 0; i < buildQueue_.Size(); ++i)
        {
            VoxelChunk* chunk = loadedMaps_[i];
            Node* chunkNode = chunk->GetNode();
            float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
            chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance);
            chunk->buildVisible_ = chunk->buildVisible_ || visibleTest.IsInsideFast(chunk->GetBoundingBox()) != OUTSIDE;
        }
    }
}

}
