#pragma once

#include "../../ThirdParty/STB/stb_voxel_render.h"

#include "../Graphics/Material.h"
#include "../Scene/Component.h"
#include "../Container/Ptr.h"
#include "../IO/VectorBuffer.h"
#include "../Core/Variant.h"
#include "../Math/StringHash.h"
#include "../Core/WorkQueue.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"

#include "Voxel.h"
#include "VoxelChunk.h"
 
namespace Urho3D {

class VoxelWorkSlot {
public:
	unsigned char* buffer;
	VoxelChunk* chunk;
	Mutex slotMutex;
	Vector<SharedPtr<WorkItem> > workItems_;
	int DecrementWork(SharedPtr<WorkItem> workItem)
	{
		MutexLock lock(slotMutex);
		workItems_.Remove(workItem);
		return workItems_.Size();
	}
};

class URHO3D_API VoxelChunk;
class URHO3D_API VoxelSet : public Component
{
public:
    static const unsigned int MAX_VOXEL_SET_PATCH_SIZE = 64;
	static const unsigned int VOXEL_WORKER_SIZE_X = 32;
	static const unsigned int VOXEL_WORKER_SIZE_Y = 16;
	static const unsigned int VOXEL_WORKER_SIZE_Z = 32;
	static const unsigned int VOXEL_WORKER_SIZE = VOXEL_WORKER_SIZE_X * VOXEL_WORKER_SIZE_Y * VOXEL_WORKER_SIZE_Z;
	static const unsigned int VOXEL_CHUNK_SIZE_X = VOXEL_WORKER_SIZE_X * 2;
	static const unsigned int VOXEL_CHUNK_SIZE_Y = VOXEL_WORKER_SIZE_Y;
	static const unsigned int VOXEL_CHUNK_SIZE_Z = VOXEL_WORKER_SIZE_Z * 2;
    static const unsigned int VOXEL_CHUNK_SIZE = VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Y * VOXEL_CHUNK_SIZE_Z;
	static const unsigned int VOXEL_CHUNK_WORK_BUFFER_SIZE = VOXEL_CHUNK_SIZE * 4 * 8;

	// This math only works if the workloads are broken up evenly
	static const unsigned int VOXEL_MAX_NUM_WORKERS_PER_CHUNK = VOXEL_CHUNK_SIZE / VOXEL_WORKER_SIZE;

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

    void BuildGeometry();

	VoxelDefinition* GetVoxelDefinition() const;
	void SetVoxelDefinition(VoxelDefinition* voxelDefinition);

    // Builds a unit of work
    void BuildChunkWork(void* work, unsigned threadIndex=0);

private:
	static SharedPtr<IndexBuffer> sharedIndexBuffer_;
    // Builds a voxel chunk.
    bool BuildChunk(VoxelChunk* chunk, bool async=false);
    // Upload Mesh
    void UploadMesh();
	void DecodeWorkBuffer(VoxelWorkSlot* slot);
	void UploadToGraphics();
	void AllocateWorkerBuffers();
	void FreeWorkerBuffers();
	void HandleWorkItemCompleted(StringHash eventType, VariantMap& eventData);
	void BuildChunkCompleted(VoxelWorkSlot* slot);
	VoxelWorkSlot* GetFreeWorkSlot(VoxelChunk* chunk);

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

	SharedPtr<VoxelDefinition> voxelDefinition_;

	bool workmemLoaded_;
    Vector<stbvox_mesh_maker> meshMakers_;
    Vector<SharedPtr<WorkItem>> workItems_;
	Vector<VoxelWorkSlot> workSlots_;
	mutable Mutex slotGetterMutex;
	int numChunksX;
	int numChunksY;
	int numChunksZ;
	int numChunks;
};

}
