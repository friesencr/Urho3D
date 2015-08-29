#include "../Core/Profiler.h"
#include "../Graphics/Camera.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Graphics/Renderer.h"
#include "../Scene/SceneEvents.h"
#include "../IO/Log.h"

#include "VoxelSet.h"
#include "VoxelStreamer.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;

VoxelStreamer::VoxelStreamer(Context* context) 
    : Component(context),
    maxInMemoryMesh_(1600),
    maxInMemoryMeshPeak_(2000),
    maxBuildFrameTime_(0),
    maxBuildsPerFrame_(0)
{

}

VoxelStreamer::~VoxelStreamer()
{

}

void VoxelStreamer::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelStreamer>(GEOMETRY_CATEGORY);
}

void VoxelStreamer::OnNodeSet(Node* node)
{
    if (node)
        node->AddListener(this);

    UpdateSubscriptions();
}


void VoxelStreamer::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);
}

void VoxelStreamer::ApplyAttributes()
{

}

void VoxelStreamer::UpdateSubscriptions()
{
	using namespace SceneUpdate;
	Scene* scene = GetScene();
    Node* node = GetNode();
    if (scene && scene && IsEnabled())
        SubscribeToEvent(scene, E_SCENEUPDATE, HANDLER(VoxelStreamer, HandleSceneUpdate));
    else
        UnsubscribeFromEvent(scene, E_SCENEUPDATE);
}

void VoxelStreamer::UnloadBestChunks()
{
    PROFILE(UnloadingChunks);
    if (loadedChunks_.Size() <= maxInMemoryMeshPeak_)
        return;

    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();
    Vector3 chunkSpacing = voxelSet_->GetChunkSpacing();

    for (unsigned v = 0; v < viewports; ++v)
    {
        Viewport* viewport = renderer->GetViewport(v);
        Camera* camera = viewport->GetCamera();
        if (!camera)
            continue;

        Node* cameraNode = camera->GetNode();
        if (!cameraNode)
            continue;

        Vector3 cameraPosition = cameraNode->GetPosition();
        float viewDistance = camera->GetFarClip();
        unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing.x_, chunkSpacing.z_)) + 2;

        int indexX = 0;
        int indexY = 0;
        int indexZ = 0;

        if (voxelSet_->GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), indexX, indexY, indexZ))
        {
            unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing.x_, chunkSpacing.z_)) + 2;
            unsigned startX = Clamp(indexX - radius, 0, voxelSet_->GetNumChunksX());
            unsigned startZ = Clamp(indexZ - radius, 0, voxelSet_->GetNumChunksZ());
            unsigned endX   = Clamp(indexX + radius + 1, 0, voxelSet_->GetNumChunksZ());
            unsigned endZ   = Clamp(indexZ + radius + 1, 0, voxelSet_->GetNumChunksZ());

            unsigned x = 0;
            unsigned y = 0;
            unsigned z = 0;
            for (PODVector<Pair<unsigned, bool> >::Iterator i = loadedChunks_.Begin(); i != loadedChunks_.End(); ++i)
            {
                voxelSet_->GetCoordinatesFromIndex(i->first_, x, y, z);
                i->second_ = !(x >= startX && x <= endX && z >= startZ && z <= endZ);
            }
        }
    }

    for (PODVector<Pair<unsigned, bool> >::Iterator i = loadedChunks_.End(); i != loadedChunks_.Begin(); --i)
    {
        if (i->second_)
        {
            unsigned x = 0;
            unsigned y = 0;
            unsigned z = 0;
            voxelSet_->GetCoordinatesFromIndex(i->first_, x, y, z);
            voxelSet_->UnloadChunk(x,y,z);
            loadedChunks_.Erase(i);
        }
    }
}

void VoxelStreamer::BuildInternal()
{
    PROFILE(VoxelStream);

    Scene* scene = GetScene();
    if (!scene)
        return;

    Node* node = GetNode();
    if (!node)
        return;

    voxelSet_ = node->GetComponent<VoxelSet>();
    if (!voxelSet_ || voxelSet_->GetNumChunks() == 0)
        return;

    maxBuildsPerFrame_ = 0;
    maxBuildFrameTime_ = 1;
    maxPriorityBuildFrameTime_ = 0;

    // build voxels
    VoxelBuilder* voxelBuilder = GetSubsystem<VoxelBuilder>();
    {
        PROFILE(BuildStreamVoxels);
        Timer buildTimer;
        CreateAndSortVisibleChunks();
        UnloadBestChunks();

        unsigned totalBuilds = 0;
        for (unsigned q = 0; q < 2; ++q)
        {
            const PODVector<unsigned>& queue = q ? buildQueue_ : priorityBuildQueue_;
            unsigned i = 0;
            unsigned maxBuildTime = q ? maxBuildFrameTime_ : maxPriorityBuildFrameTime_;
            while (i < queue.Size() && (!maxBuildTime || buildTimer.GetMSec(false) < maxBuildTime) && (!maxBuildsPerFrame_ || totalBuilds < maxBuildsPerFrame_))
            {
                totalBuilds++;
                unsigned buildItem = queue[i++];
                unsigned x = 0;
                unsigned y = 0;
                unsigned z = 0;
                voxelSet_->GetCoordinatesFromIndex(buildItem, x, y, z);
                voxelSet_->LoadChunk(x,y,z);
                loadedChunks_.Push(Pair<unsigned, bool>(buildItem, false));
                if (i % 2 == 1)
                    voxelBuilder->CompleteWork();
            }
        }
        voxelBuilder->CompleteWork();
        priorityBuildQueue_.Clear();
        buildQueue_.Clear();
    }
    // cleanup
}


void VoxelStreamer::SortLoadedChunks()
{
    PROFILE(SortLoadedChunks);

}

void VoxelStreamer::CreateAndSortVisibleChunks()
{
    PROFILE(StreamNewChunk);
    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();

    Vector3 chunkSpacing = voxelSet_->GetChunkSpacing();
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
        float viewDistance = camera->GetFarClip() * 1.0f;
        Frustum visibleTest = camera->GetFrustum();
        //visibleTest.Define(camera->GetFov() * 1.0f, camera->GetAspectRatio(), camera->GetZoom(), 
        //    camera->GetNearClip(), viewDistance, camera->GetEffectiveWorldTransform());

        int currentX = 0;
        int currentY = 0;
        int currentZ = 0;

        if (voxelSet_->GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), currentX, currentY, currentZ))
        {
            unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing.x_, chunkSpacing.z_)) + 2;
            CreateChunksWithGrid(currentX, currentY, currentZ, radius, cameraNode->GetPosition(), visibleTest, viewDistance);
        }
    }
}


void VoxelStreamer::CreateChunksWithGrid(int indexX, int indexY, int indexZ, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance)
{
    unsigned startX = Clamp(indexX - size, 0, voxelSet_->GetNumChunksX());
    unsigned startZ = Clamp(indexZ - size, 0, voxelSet_->GetNumChunksZ());
    unsigned endX   = Clamp(indexX + size + 1, 0, voxelSet_->GetNumChunksZ());
    unsigned endZ   = Clamp(indexZ + size + 1, 0, voxelSet_->GetNumChunksZ());
    
    if (endX == 0 || endZ == 0)
        return;

    unsigned numBuilds = 0;
    Vector3 chunkSpacing = voxelSet_->GetChunkSpacing();
    for (unsigned x = startX; x < endX; ++x)
    {
        for (unsigned z = startZ; z < endZ; ++z)
        {
            VoxelChunk* chunk = voxelSet_->GetVoxelChunk(x, 0, z);
            if (!chunk)
            {
                numBuilds++;
                BoundingBox chunkBox = BoundingBox(
                    chunkSpacing * Vector3((float)x, 0.0, (float)z), 
                    chunkSpacing * Vector3((float)(x+1), 1.0, (float)(z+1))
                );
                Vector3 position = chunkBox.Center();
                bool visible = frustrum.IsInside(chunkBox) != OUTSIDE;
                unsigned index = voxelSet_->GetChunkIndex(x, 0, z);
                if (visible)
                    priorityBuildQueue_.Push(index);
                else
                    buildQueue_.Push(index);

                if (priorityBuildQueue_.Size() > maxBuildsPerFrame_)
                    return;
                // TODO: Check build queue for existing item from different viewports
            }
        }
    }
    if (numBuilds)
        LOGINFO("BUILDING " + String(numBuilds) + " chunks.");
}

void VoxelStreamer::OnSetEnabled()
{
    UpdateSubscriptions();
}

void VoxelStreamer::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    BuildInternal();
}

}
