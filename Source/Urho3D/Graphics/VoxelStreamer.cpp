#include "VoxelStreamer.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;

VoxelStreamer::VoxelStreamer(Context* context) : Component(context)
{

}

VoxelStreamer::~VoxelStreamer()
{

}

void VoxelStreamer::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelStreamer>(GEOMETRY_CATEGORY);
}

void VoxelStreamer::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);
}

void VoxelStreamer::ApplyAttributes()
{

}

void VoxelStreamer::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();
}

}

//void VoxelStreamer::BuildInternal(bool async)
//{
//    PROFILE(BuildInternal);
//
//	if (numChunks == 0)
//		return;
//
//    AllocateAndSortVisibleChunks();
//
//    if (loadedMeshes_.Size() > maxInMemoryMeshPeak_)
//    {
//        SortLoadedChunks();
//        Sort(loadedMeshes_.Begin(), loadedMeshes_.End(), CompareChunks);
//        while (loadedMeshes_.Size() > maxInMemoryMesh_)
//        {
//            VoxelChunk* chunk = loadedMeshes_[maxInMemoryMesh_];
//            bool buildVisible = chunk->buildVisible_;
//            float buildPriority = chunk->buildPrioirty_;
//            chunks_[GetIndex(chunk->GetIndexX(), chunk->GetIndexY(), chunk->GetIndexZ())] = 0;
//            chunk->Unload();
//            chunk->GetNode()->Remove();
//            loadedMeshes_.Erase(maxInMemoryMesh_);
//            buildQueue_.Remove(chunk);
//        }
//    }
//
//    Sort(buildQueue_.Begin(), buildQueue_.End(), CompareChunks);
//	int buildCounter = 0;
//	VoxelBuilder* voxelBuilder = GetSubsystem<VoxelBuilder>();
//  //  while (buildQueue_.Size())
//  //  {
//  //      VoxelChunk* chunk = buildQueue_[0];
//		//buildQueue_.Erase(0);
//		//chunk->BuildAsync();
//		//loadedMeshes_.Push(chunk);
//  //  }
//	for (unsigned i = 0; i < buildQueue_.Size(); ++i)
//    {
//        VoxelChunk* chunk = buildQueue_[i];
//		chunk->BuildAsync();
//		loadedMeshes_.Push(chunk);
//    }
//	buildQueue_.Clear();
//	voxelBuilder->CompleteWork();
//}
//
//
//void VoxelStreamer::SortLoadedChunks()
//{
//    PROFILE(SortLoadedChunks);
//
//    Scene* scene = GetScene();
//    if (!scene)
//        return;
//
//    Node* setNode = GetNode();
//    if (!setNode)
//        return;
//
//    Renderer* renderer = GetSubsystem<Renderer>();
//    unsigned viewports = renderer->GetNumViewports();
//
//    for (unsigned i = 0; i < loadedMeshes_.Size(); ++i)
//    {
//        loadedMeshes_[i]->buildPrioirty_ = BUILD_UNSET;
//        loadedMeshes_[i]->buildVisible_ = false;
//    }
//
//    for (unsigned i = 0; i < viewports; ++i)
//    {
//        Viewport* viewport = renderer->GetViewport(i);
//        Camera* camera = viewport->GetCamera();
//        if (!camera)
//            continue;
//
//        Node* cameraNode = camera->GetNode();
//        if (!cameraNode)
//            continue;
//
//        Frustum visibleTest = camera->GetFrustum();
//        float viewDistance = camera->GetFarClip();
//
//        for (unsigned i = 0; i < loadedMeshes_.Size(); ++i)
//        {
//            VoxelChunk* chunk = loadedMeshes_[i];
//            Node* chunkNode = chunk->GetNode();
//            float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
//            chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance);
//            chunk->buildVisible_ = chunk->buildVisible_ || visibleTest.IsInside(chunk->GetBoundingBox()) != OUTSIDE;
//        }
//    }
//
//}
//
//void VoxelStreamer::AllocateAndSortVisibleChunks()
//{
//    PROFILE(BuildSet);
//
//    Scene* scene = GetScene();
//    if (!scene)
//        return;
//
//    Node* setNode = GetNode();
//    if (!setNode)
//        return;
//
//    Renderer* renderer = GetSubsystem<Renderer>();
//    unsigned viewports = renderer->GetNumViewports();
//
//    //for (unsigned i = 0; i < buildQueue_.Size(); ++i)
//    //{
//    //    buildQueue_[i]->buildPrioirty_ = BUILD_UNSET;
//    //    buildQueue_[i]->buildVisible_ = false;
//    //}
//
//    for (unsigned i = 0; i < viewports; ++i)
//    {
//        Viewport* viewport = renderer->GetViewport(i);
//        if (!viewport)
//            continue;
//
//        Camera* camera = viewport->GetCamera();
//        if (!camera)
//            continue;
//
//        Node* cameraNode = camera->GetNode();
//        if (!cameraNode)
//            continue;
//
//        // make frustrum 1.2x as long as camera
//        Frustum visibleTest;
//        float viewDistance = camera->GetFarClip() * 1.0f;
//        visibleTest.Define(camera->GetFov() * 1.0f, camera->GetAspectRatio(), camera->GetZoom(), 
//            camera->GetNearClip(), viewDistance, camera->GetEffectiveWorldTransform());
//
//        int currentX = 0;
//        int currentY = 0;
//        int currentZ = 0;
//
//        if (GetIndexFromWorldPosition(cameraNode->GetWorldPosition(), currentX, currentY, currentZ))
//        {
//            unsigned radius = (unsigned)ceilf(viewDistance / Max(chunkSpacing_.x_, chunkSpacing_.z_));
//            CreateChunks(currentX, currentY, currentZ, radius, cameraNode->GetPosition(), visibleTest, viewDistance);
//        }
//
//        //for (unsigned i = 0; i < buildQueue_.Size(); ++i)
//        //{
//        //    VoxelChunk* chunk = buildQueue_[i];
//        //    Node* chunkNode = chunk->GetNode();
//        //    float chunkDistance = (cameraNode->GetPosition() - chunkNode->GetPosition()).Length();
//        //    chunk->buildPrioirty_ = Min(chunk->buildPrioirty_, chunkDistance);
//        //    chunk->buildVisible_ = chunk->buildVisible_ || visibleTest.IsInside(chunk->GetBoundingBox()) != OUTSIDE;
//        //}
//    }
//}
//
//void VoxelStreamer::CreateChunks(int indexX, int indexY, int indexZ, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance)
//{
//    PROFILE(CreateChunks);
//    Node* voxelSetNode = GetNode();
//    if (!voxelSetNode)
//        return;
//
//    unsigned startX = Max(0, indexX - size);
//    unsigned startZ = Max(0, indexZ - size);
//    unsigned endX = Max(0, indexX + size + 1);
//    unsigned endZ = Max(0, indexZ + size + 1);
//    
//    if (endX == 0 || endZ == 0)
//        return;
//
//    for (unsigned x = startX; x < endX; ++x)
//    {
//        for (unsigned z = startZ; z < endZ; ++z)
//        {
//            BoundingBox chunkBox = BoundingBox(chunkSpacing_ * Vector3((float)x, 0.0, (float)z), chunkSpacing_ * Vector3((float)x, 0.0, (float)z) + chunkSpacing_);
//            Vector3 position = chunkBox.Center();
//            if (frustrum.IsInside(chunkBox) == OUTSIDE)
//                continue;
//
//            VoxelMap* voxelMap = GetVoxelMap(x, 0, z);
//            if (voxelMap)
//                FindOrCreateVoxelChunk(x, 0, z, voxelMap);
//        }
//    }
//}
//
//void VoxelStreamer::OnSetEnabled()
//{
//	using namespace SceneUpdate;
//	Scene* scene = GetScene();
//	if (IsEnabled())
//		SubscribeToEvent(scene, E_SCENEUPDATE, HANDLER(VoxelStreamer, HandleSceneUpdate));
//	else
//		UnsubscribeFromEvent(scene, E_SCENEUPDATE);
//}
//
//void VoxelStreamer::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
//{
//	if (loadedMeshes_.Size() == numChunks)
//        return;
//
//    BuildInternal(true);
//}