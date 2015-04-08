#include "../../ThirdParty/STB/stb_voxel_render.h"

#include "../Scene/Component.h"
#include "../Container/Ptr.h"
#include "../IO/VectorBuffer.h"
#include "../Core/Variant.h"
#include "../Math/StringHash.h"

#include "Voxel.h"
#include "VoxelChunk.h"
 
#define STBVOX_CONFIG_MODE  1
#define STB_VOXEL_RENDER_IMPLEMENTATION

namespace Urho3D {

class VoxelSet : public Component
{
    static const unsigned int MAX_VOXEL_SET_PATCH_SIZE = 64;
    static const unsigned int BUILD_SIZE_INCREMENT=16;

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

    /// Set material.
    void SetMaterial(Material* material);
    /// Sets size of set.
    void SetSize(unsigned int height, unsigned int width, unsigned int depth);
    /// Set draw distance for patches.
    void SetDrawDistance(float distance);
    /// Set shadow draw distance for patches.
    void SetShadowDistance(float distance);
    /// Set view mask for patches. Is and'ed with camera's view mask to see if the object should be rendered.
    void SetViewMask(unsigned mask);
    /// Set light mask for patches. Is and'ed with light's and zone's light mask to see if the object should be lit.
    void SetLightMask(unsigned mask);
    /// Set shadow mask for patches. Is and'ed with light's light mask and zone's shadow mask to see if the object should be rendered to a shadow map.
    void SetShadowMask(unsigned mask);
    /// Set zone mask for patches. Is and'ed with zone's zone mask to see if the object should belong to the zone.
    void SetZoneMask(unsigned mask);
    /// Set maximum number of per-pixel lights for patches. Default 0 is unlimited.
    void SetMaxLights(unsigned num);
    /// Set shadowcaster flag for patches.
    void SetCastShadows(bool enable);
    /// Set occlusion flag for patches. Occlusion uses the coarsest LOD and may potentially be too aggressive, so use with caution.
    void SetOccluder(bool enable);
    /// Set occludee flag for patches.
    void SetOccludee(bool enable);

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);

    void SetVoxelDefinitions(Vector<VoxelDefinition> voxelDefinitions);

    // Store Voxels in x, z, y order
    void SetVoxels(PODVector<Voxel> voxels);

    void BuildGeometry();

private:
    // Builds a voxel chunk.
    void BuildChunk(VoxelChunk* chunk);
    // Builds a unit of work
    void BuildChunkWork(void* work, unsigned threadIndex=0);
    // Upload Mesh
    void UploadMesh();
    // Material.
    SharedPtr<Material> material_;
    // Voxel chunks.
    Vector<WeakPtr<VoxelChunk> > chunks_;
    // Height.
    unsigned int height_;
    // Width.
    unsigned int width_;
    // Depth.
    unsigned int depth_;

    unsigned char voxelTextureMapping_[256][6];
    unsigned char voxelGeometries_[256];

    PODVector<Voxel> voxels_;
    Vector<stbvox_mesh_maker> mesh_makers_;
    Vector<WorkItem> workItems_;
};

}
