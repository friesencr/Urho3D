#pragma once

#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Container/Vector.h"
#include "../Math/Vector3.h"
#include "../Math/Frustum.h"
#include "../Scene/Component.h"

#include "VoxelSet.h"

namespace Urho3D 
{ 

/// Heightmap terrain component.
class URHO3D_API VoxelStreamer : public Component
{
    OBJECT(VoxelStreamer);

public:
    /// Construct.
    VoxelStreamer(Context* context);
    /// Destruct.
    ~VoxelStreamer();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Handle attribute write access.
    virtual void OnSetAttribute(const AttributeInfo& attr, const Variant& src);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes();
    /// Builds appropriate chunks.
    //void Build();

    unsigned GetNumberOfLoadedChunks() { return loadedChunks_.Size(); }
protected:
    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();
    /// Handle node set
    virtual void OnNodeSet(Node* node);

private:
    void BuildInternal();
    void CreateChunksWithGrid(int x, int y,  int z, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance);
    void CreateAndSortVisibleChunks();
    void SortLoadedChunks();
    void UnloadBestChunks();
    void BuildChunks();
    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    void UpdateSubscriptions();
    //unsigned ChunkInCameraGrid(int x, int y, int z, unsigned viewport);
    //unsigned ChunkInCameraGrid(int x, int y, int z, const Frustum& frustrum);

    unsigned maxInMemoryMeshPeak_;
    unsigned maxInMemoryMesh_;
    unsigned maxBuildsPerFrame_;
    unsigned maxBuildFrameTime_;
    unsigned maxPriorityBuildFrameTime_;
    WeakPtr<VoxelSet> voxelSet_;
    PODVector<unsigned> buildQueue_;
    PODVector<unsigned> priorityBuildQueue_;
    PODVector<Pair<unsigned, bool> > loadedChunks_;
};

}
