#pragma once

#include "VoxelSet.h"
#include "Drawable.h"
#include "Geometry.h"
#include "VertexBuffer.h"
#include "../Container/Ptr.h"
#include "../Core/Mutex.h"

namespace Urho3D {

class URHO3D_API VoxelSet;
class URHO3D_API VoxelChunk : public Drawable
{
    OBJECT(VoxelChunk);
    friend class VoxelSet;

public:
    VoxelChunk(Context* context);
    ~VoxelChunk();

    static void RegisterObject(Context* context);

    Geometry* GetGeometry() const;
    VertexBuffer* GetVertexBuffer() const;

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

    /// Set owner terrain.
    void SetOwner(VoxelSet* voxelSet);

    /// Set material.
    void SetMaterial(Material* material);

    /// Set local-space bounding box.
    void SetBoundingBox(const BoundingBox& box);

    int GetIndexX() const;
    int GetIndexY() const;
    int GetIndexZ() const;
    int GetSizeX() const;
    int GetSizeY() const;
    int GetSizeZ() const;
    unsigned GetLodLevel() const;

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();

private:
    void SetIndex(int x, int y, int z);
    void SetSize(int x, int y, int z);
    // Voxel chunk geometry
    SharedPtr<Geometry> geometry_;
    // Vertex data
    SharedPtr<VertexBuffer> vertexBuffer_;
    // X Index
    int indexX_;
    // Y Index
    int indexY_;
    // Z Index
    int indexZ_;
    // X Index
    int sizeX_;
    // Y Index
    int sizeY_;
    // Z Index
    int sizeZ_;
    float transform_[3][3];
    float bounds_[2][3];
    float priority_;
    int numQuads_;
    unsigned lodLevel_;
    WeakPtr<VoxelSet> owner_;
};

}
