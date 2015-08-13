#pragma once

#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Container/Vector.h"
#include "../Math/Vector3.h"
#include "../Math/Frustum.h"
#include "../Scene/Component.h"
#include "VoxelSet.h"
#include "VoxelChunk.h"
#include "VoxelBuilder.h"

namespace Urho3D 
{ 

struct VoxelStreamQueueItem {
    unsigned index_;
    unsigned x_;
    unsigned y_;
    unsigned z_;
    float distance_;
    bool visible_;
};

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
    void Build();
	//unsigned GetNumberOfLoadedChunks() { return loadedMeshes_.Size(); }
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
    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    void UpdateSubscriptions();

    unsigned maxInMemoryMeshPeak_;
    unsigned maxInMemoryMesh_;
    WeakPtr<VoxelSet> voxelSet_;
    PODVector<VoxelStreamQueueItem> buildQueue_;
    PODVector<VoxelStreamQueueItem> loadedChunks_;
};

}
