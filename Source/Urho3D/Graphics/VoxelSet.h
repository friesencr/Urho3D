#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"
#include "../Scene/Component.h"
#include "../IO/VectorBuffer.h"
#include "../Math/StringHash.h"
#include "Material.h"
#include "../IO/MemoryBuffer.h"

#include "VoxelChunk.h"
#include "Voxel.h"

namespace Urho3D {

class URHO3D_API VoxelChunk;
class URHO3D_API VoxelSet : public Component
{
public:
    // Construct.
    VoxelSet(Context* context);
    // Destruct.
    ~VoxelSet();
    /// Register object factory
    static void RegisterObject(Context* context);
    /// Handle attribute write access.
    virtual void OnSetAttribute(const AttributeInfo& attr, const Variant& src);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes();
    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();

    /// Gets chunk spacing
    Vector3 GetChunkSpacing() const { return chunkSpacing_; }

    /// Sets chunk spacing
    void SetChunkSpacing(Vector3 spacing);
    /// Sets number of chunks in the set
    void SetNumberOfChunks(unsigned x, unsigned y, unsigned z);
    /// Gets number of chunks in the set
    unsigned GetNumberOfChunks() const { return numChunks_; }
    /// Gets number of chunks in the set
    unsigned GetNumberOfChunksX() const { return numChunksX_; }
    /// Gets number of chunks in the set
    unsigned GetNumberOfChunksY() const { return numChunksY_; }
    /// Gets number of chunks in the set
    unsigned GetNumberOfChunksZ() const { return numChunksZ_; }
    /// Gets Voxel Chunk at world position.
    VoxelChunk* GetVoxelChunkAtPosition(Vector3 position);
    /// Gets Voxel Chunk by index.
    VoxelChunk* GetVoxelChunk(unsigned x, unsigned y, unsigned z);

    VoxelMap* GetVoxelMap(unsigned x, unsigned y, unsigned z);

    void SetVoxelMap(unsigned x, unsigned y, unsigned z, VoxelMap* voxelMap);

    unsigned GetIndex(unsigned x, unsigned y, unsigned z) const;

    void GetCoordinatesFromIndex(unsigned index, unsigned &x, unsigned &y, unsigned &z);

    void LoadChunk(unsigned x, unsigned y, unsigned z);

    void UnloadChunk(unsigned x, unsigned y, unsigned z);

    bool GetIndexFromWorldPosition(Vector3 worldPosition, int &x, int &y, int &z);

    /// Set Voxel Maps
    void SetVoxelMapResourceRefList(const ResourceRefList& value);

    void SetVoxelMapResource(unsigned x, unsigned y, unsigned z, const String& name);


    ///// Set material.
    //void SetMaterial(Material* material);
    ///// Sets size of set.
    //void SetSize(unsigned int height, unsigned int width, unsigned int depth);
    ///// Set draw distance for patches.
    //void SetDrawDistance(float distance);
    ///// Set shadow draw distance for patches.
    //void SetShadowDistance(float distance);
    ///// Set view mask for patches. Is and'ed with camera's view mask to see if the object should be rendered.
    //void SetViewMask(unsigned mask);
    ///// Set light mask for patches. Is and'ed with light's and zone's light mask to see if the object should be lit.
    //void SetLightMask(unsigned mask);
    ///// Set shadow mask for patches. Is and'ed with light's light mask and zone's shadow mask to see if the object should be rendered to a shadow map.
    //void SetShadowMask(unsigned mask);
    ///// Set zone mask for patches. Is and'ed with zone's zone mask to see if the object should belong to the zone.
    //void SetZoneMask(unsigned mask);
    ///// Set maximum number of per-pixel lights for patches. Default 0 is unlimited.
    //void SetMaxLights(unsigned num);
    ///// Set shadowcaster flag for patches.
    //void SetCastShadows(bool enable);
    ///// Set occlusion flag for patches. Occlusion uses the coarsest LOD and may potentially be too aggressive, so use with caution.
    //void SetOccluder(bool enable);
    ///// Set occludee flag for patches.
    //void SetOccludee(bool enable);
    void Build();

private:
    void BuildInternal();
    VoxelChunk* FindOrCreateVoxelChunk(unsigned x, unsigned y, unsigned z);
    // Material.
    SharedPtr<Material> material_;
    // Voxel chunks.
    Vector<WeakPtr<VoxelChunk> > chunks_;
    Vector<WeakPtr<VoxelMap> > voxelMaps_;
    ResourceRefList voxelMapResourceRefList_;
    BoundingBox setBox;
    unsigned numChunksX_;
    unsigned numChunksY_;
    unsigned numChunksZ_;
    unsigned chunkXStride_;
    unsigned chunkZStride_;
    unsigned numChunks_;
    Vector3 chunkSpacing_;
};

}