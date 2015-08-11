#pragma once

#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Container/Vector.h"
#include "../Math/Vector3.h"
#include "../Math/Frustum.h"
#include "../Scene/Component.h"

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
    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();


	//unsigned GetNumberOfLoadedChunks() { return loadedMeshes_.Size(); }

    void CreateChunks(int x, int y,  int z, unsigned size, Vector3 cameraPosition, Frustum frustrum, float viewDistance);
    void AllocateAndSortVisibleChunks();
    void SortLoadedChunks();


private:
    unsigned maxInMemoryMeshPeak_;
    unsigned maxInMemoryMesh_;
    //PODVector<VoxelChunk*> loadedMeshes_;
    //PODVector<VoxelChunk*> buildQueue_;
    //Vector3 lastBuildVector;

};

}
