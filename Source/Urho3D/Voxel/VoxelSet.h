#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Container/Ptr.h"
#include "../Scene/Component.h"
#include "../Math/StringHash.h"
#include "../Graphics/Material.h"

#include "VoxelChunk.h"
#include "VoxelStore.h"

namespace Urho3D {

class VoxelChunk;
class VoxelStore;

class URHO3D_API VoxelSet : public Component
{
    OBJECT(VoxelSet);
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

    /// Gets Voxel Chunk at world position.
    VoxelChunk* GetVoxelChunkAtPosition(Vector3 position);

    /// Gets Voxel Chunk by index.
    VoxelChunk* GetVoxelChunk(unsigned x, unsigned y, unsigned z);

    void GetCoordinatesFromIndex(unsigned index, unsigned &x, unsigned &y, unsigned &z);
    unsigned GetChunkIndex(unsigned x, unsigned y, unsigned z) const;

    void LoadChunk(unsigned x, unsigned y, unsigned z, bool async = true);

    void UnloadChunk(unsigned x, unsigned y, unsigned z);

    bool GetIndexFromWorldPosition(Vector3 worldPosition, int &x, int &y, int &z);

    VoxelStore* GetVoxelStore() const { return voxelStore_; }
    void SetVoxelStore(VoxelStore* voxelStore);

    unsigned GetNumChunks() const { return numChunks_; }
    unsigned GetNumChunksX() const { return numChunksX_; }
    unsigned GetNumChunksY() const { return numChunksY_; }
    unsigned GetNumChunksZ() const { return numChunksZ_; }

    ///// Get Voxel Maps Attribute
    //const ResourceRefList& GetVoxelMapAttr() const { return voxelMapResourceRefList_; }

    ///// Set Voxel Maps Attribute
    //void SetVoxelMapsAttr(const ResourceRefList& value);


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
    void SetVoxelStoreInternal();
    VoxelChunk* FindOrCreateVoxelChunk(unsigned x, unsigned y, unsigned z);
    // Material.
    SharedPtr<Material> material_;
    // Voxel chunks.
    Vector<WeakPtr<VoxelChunk> > chunks_;
    BoundingBox setBox;
    unsigned numChunksX_;
    unsigned numChunksY_;
    unsigned numChunksZ_;
    unsigned chunkXStride_;
    unsigned chunkZStride_;
    unsigned numChunks_;
    Vector3 chunkSpacing_;
    SharedPtr<VoxelStore> voxelStore_;
};

}
