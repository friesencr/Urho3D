#include "VoxelStreamer.h"
#include "../Core/Profiler.h"
#include "Camera.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "Renderer.h"
#include "../Scene/SceneEvents.h"
#include "../IO/Log.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;
static const float BUILD_UNSET = 100000000.0;

inline bool CompareChunks(const VoxelStreamQueueItem& lhs, const VoxelStreamQueueItem& rhs)
{
    //return lhs.visible_ > rhs.visible_ || lhs.distance_ > rhs.distance_;
    return true;
}

VoxelStreamer::VoxelStreamer(Context* context) 
    : Component(context),
    maxInMemoryMesh_(700),
    maxInMemoryMeshPeak_(1000)
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
    if (loadedChunks_.Size() > maxInMemoryMeshPeak_)
    {
        SortLoadedChunks();
        for (unsigned i = maxInMemoryMesh_; i < loadedChunks_.Size(); ++i)
        {
            VoxelStreamQueueItem& chunk = loadedChunks_[maxInMemoryMesh_];
            voxelSet_->UnloadChunk(chunk.x_, chunk.y_, chunk.z_);
            //buildQueue_.Remove(chunk);
        }
        loadedChunks_.Resize(maxInMemoryMesh_);
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
    if (!voxelSet_ || voxelSet_->GetNumberOfChunks() == 0)
        return;

    CreateAndSortVisibleChunks();
    //UnloadBestChunks();

    // build voxels
	VoxelBuilder* voxelBuilder = GetSubsystem<VoxelBuilder>();
    {
        PROFILE(BuildStreamVoxels);
        for (unsigned i = 0; i < buildQueue_.Size(); ++i)
        {
            const VoxelStreamQueueItem& buildItem = buildQueue_[i];
            voxelSet_->LoadChunk(buildItem.x_, buildItem.y_, buildItem.z_);
            loadedChunks_.Push(buildItem);
            voxelBuilder->CompleteWork();
        }
        voxelBuilder->CompleteWork();
        buildQueue_.Clear();
    }
    // cleanup
}


void VoxelStreamer::SortLoadedChunks()
{
    PROFILE(SortLoadedChunks);

    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();
    Vector3 chunkSpacing = voxelSet_->GetChunkSpacing();

    for (unsigned i = 0; i < loadedChunks_.Size(); ++i)
    {
        loadedChunks_[i].distance_ = BUILD_UNSET;
        loadedChunks_[i].visible_ = false;
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

        Vector3 cameraPosition = cameraNode->GetPosition();
        Frustum frustrum = camera->GetFrustum();
        float viewDistance = camera->GetFarClip();

        for (unsigned i = 0; i < loadedChunks_.Size(); ++i)
        {
            VoxelStreamQueueItem& chunk = loadedChunks_[i];
            BoundingBox chunkBox = BoundingBox(chunkSpacing * Vector3((float)chunk.x_, 0.0, (float)chunk.z_), chunkSpacing * Vector3((float)chunk.x_, 0.0, (float)chunk.z_) + chunkSpacing);
            Vector3 position = chunkBox.Center();
            if (voxelSet_->GetVoxelChunk(chunk.x_, 0, chunk.z_))
            {
                chunk.distance_ = Min(chunk.distance_, (cameraPosition - position).Length());
                chunk.visible_ = chunk.visible_ || frustrum.IsInside(chunkBox) != OUTSIDE;
            }
        }
    }
    //Sort(loadedChunks_.Begin(), loadedChunks_.End(), CompareChunks);
}

void VoxelStreamer::CreateAndSortVisibleChunks()
{
    PROFILE(StreamNewChunk);
    Renderer* renderer = GetSubsystem<Renderer>();
    unsigned viewports = renderer->GetNumViewports();

    for (unsigned i = 0; i < buildQueue_.Size(); ++i)
    {
        buildQueue_[i].distance_ = BUILD_UNSET;
        buildQueue_[i].visible_ = false;
    }

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
        Frustum visibleTest = camera->GetFrustum();
        float viewDistance = camera->GetFarClip() * 1.0f;
        //visibleTest.Define(camera->GetFov() * 1.0f, camera->GetAspectRatio(), camera->GetZoom(), 
        //    camera->GetNearClip(), viewDistance, camera->GetEffectiveWorldTransform());

        int currentX = 0;
        int currentY = 0;
        int currentZ = 0;

        if (voxelSet_->GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), currentX, currentY, currentZ))
        {
            unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing.x_, chunkSpacing.z_));
            CreateChunksWithGrid(currentX, currentY, currentZ, radius, cameraNode->GetPosition(), visibleTest, viewDistance);
        }
    }

    //Sort(buildQueue_.Begin(), buildQueue_.End(), CompareChunks);
}


void VoxelStreamer::CreateChunksWithGrid(int indexX, int indexY, int indexZ, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance)
{
    Node* voxelSetNode = GetNode();
    if (!voxelSetNode)
        return;

    unsigned startX = Clamp(indexX - size, 0, voxelSet_->GetNumberOfChunksX());
    unsigned startZ = Clamp(indexZ - size, 0, voxelSet_->GetNumberOfChunksZ());
    unsigned endX   = Clamp(indexX + size + 1, 0, voxelSet_->GetNumberOfChunksZ());
    unsigned endZ   = Clamp(indexZ + size + 1, 0, voxelSet_->GetNumberOfChunksZ());
    
    if (endX == 0 || endZ == 0)
        return;

    Vector3 chunkSpacing = voxelSet_->GetChunkSpacing();

    for (unsigned x = startX; x < endX; ++x)
    {
        for (unsigned z = startZ; z < endZ; ++z)
        {
            VoxelChunk* chunk = voxelSet_->GetVoxelChunk(x, 0, z);
            if (!chunk)
            {
                BoundingBox chunkBox = BoundingBox(
                    chunkSpacing * Vector3((float)x, 0.0, (float)z), 
                    chunkSpacing * Vector3((float)(x+1), 1.0, (float)(z+1))
                );
                Vector3 position = chunkBox.Center();
                bool visible = frustrum.IsInside(chunkBox) != OUTSIDE;
                if (!visible)
                    continue;

                VoxelStreamQueueItem buildItem;
                buildItem.distance_ = (cameraPosition - position).Length();
                buildItem.visible_ = visible;
                buildItem.x_ = x;
                buildItem.y_ = 0;
                buildItem.z_ = z;
                buildItem.index_ = voxelSet_->GetIndex(x,0,z);
                // TODO: Check build queue for existing item from different viewports
                buildQueue_.Push(buildItem);
            }
        }
    }
    if (buildQueue_.Size())
        LOGINFO("BUILDING " + String(buildQueue_.Size()) + " chunks.");
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