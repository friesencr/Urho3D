#pragma once

#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Container/Ptr.h"
#include "../Core/Mutex.h"
#include "../Graphics/TextureBuffer.h"

#include "VoxelDefs.h"
#include "VoxelMap.h"
#include "VoxelBuilder.h"
#include "VoxelSet.h"

namespace Urho3D
{
class VoxelMap;
struct VoxelJob;

struct URHO3D_API VoxelMesh
{
    SharedPtr<Geometry> geometry_;
    SharedPtr<Material> material_;
    unsigned numTriangles_;
    unsigned numVertices_;
    unsigned numIndicies_;
    bool dirtyShaderParameters_;
};

class URHO3D_API VoxelChunk : public Drawable
{
    OBJECT(VoxelChunk);
    friend class VoxelSet;
    friend class VoxelBuilder;

    public:
    VoxelChunk(Context* context);
    ~VoxelChunk();

    static void RegisterObject(Context* context);

    Geometry* GetGeometry(unsigned index) const;

    /// Process octree raycast. May be called from a worker thread.
    virtual void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results);
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame);
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update.)
    virtual void UpdateGeometry(const FrameInfo& frame);
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    virtual UpdateGeometryType GetUpdateGeometryType();
    /// Return the geometry for a specific LOD level.
    virtual Geometry* GetLodGeometry(unsigned batchIndex, unsigned level);
    /// Return number of occlusion geometry triangles.
    virtual unsigned GetNumOccluderTriangles();
    /// Draw to occlusion buffer. Return true if did not run out of triangles.
    virtual bool DrawOcclusion(OcclusionBuffer* buffer);

    //bool GetHasShaderParameters(unsigned index);
    //void SetHasShaderParameters(unsigned index, bool isRequired);

    /// Set mesh selector material.
    Material* GetMaterial(unsigned selector) const;

    /// Set material by mesh selector.
    void SetMaterial(unsigned selector, Material* material);

    /// Set local-space bounding box.
    void SetBoundingBox(const BoundingBox& box);

    void SetNumberOfMeshes(unsigned count);

    unsigned GetNumMeshes() const { return voxelMeshes_.Size(); }
    unsigned GetTotalTriangles() const;
    unsigned GetNumTriangles(unsigned index) const { return voxelMeshes_[index].numTriangles_; }
    unsigned char GetIndexX();
    unsigned char GetIndexY();
    unsigned char GetIndexZ();
    unsigned char GetSizeX() const { return VOXEL_CHUNK_SIZE_X; }
    unsigned char GetSizeY() const { return VOXEL_CHUNK_SIZE_Y; }
    unsigned char GetSizeZ() const { return VOXEL_CHUNK_SIZE_Z; }
    VoxelMesh& GetVoxelMesh(unsigned index) { return voxelMeshes_[index]; }
    void SetIndex(unsigned char x, unsigned char y, unsigned char z);
    bool Build(VoxelMap* voxelMap);

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();

private:
    void UpdateMaterialParameters(unsigned slot, VoxelMap* voxelMap);
    void OnVoxelChunkCreated();
    unsigned char index_[3];
    VoxelJob* voxelJob_;
    WeakPtr<VoxelMap> voxelMap_;
    Vector<VoxelMesh> voxelMeshes_;

};

}
