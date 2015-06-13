#pragma once

#include "VoxelSet.h"
#include "Drawable.h"
#include "Geometry.h"
#include "VertexBuffer.h"
#include "../Container/Ptr.h"
#include "../Core/Mutex.h"
#include "TextureBuffer.h"
#include "Voxel.h"
#include "VoxelBuilder.h"

namespace Urho3D {

class URHO3D_API VoxelSet;
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
    VertexBuffer* GetVertexBuffer(unsigned index) const;
    IndexBuffer* GetFaceData(unsigned index) const;
    TextureBuffer* GetFaceBuffer(unsigned index) const;

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

    bool GetHasShaderParameters(unsigned index);
    void SetHasShaderParameters(unsigned index, bool isRequired);

    /// Set mesh selector material.
    Material* GetMaterial(unsigned selector) const;

    /// Gets the voxel definition
    VoxelMap* GetVoxelMap() const;

    /// Sets the voxel definition
    void SetVoxelMap(VoxelMap* voxelMap);

    /// Set owner terrain.
    void SetOwner(VoxelSet* voxelSet);

    /// Set material by mesh selector.
    void SetMaterial(unsigned selector, Material* material);

    /// Set local-space bounding box.
    void SetBoundingBox(const BoundingBox& box);

    void SetNumberOfMeshes(unsigned count);

    void Build();
    void BuildAsync();

	unsigned GetNumMeshes() const { return numMeshes_; }
	unsigned GetTotalQuads() const;
	unsigned GetNumQuads(unsigned index) const { return numQuads_[index]; }
	unsigned GetReducedNumQuads(unsigned index) const { return reducedQuadCount_[index]; }
    unsigned char GetIndexX();
    unsigned char GetIndexY();
    unsigned char GetIndexZ();
    unsigned char GetSizeX();
    unsigned char GetSizeY();
    unsigned char GetSizeZ();
    void SetIndex(unsigned char x, unsigned char y, unsigned char z);
    void SetSize(unsigned char x, unsigned char y, unsigned char z);
    void SetNeighbors(VoxelChunk* north, VoxelChunk* south, VoxelChunk* east, VoxelChunk* west);
    VoxelChunk* GetNeighborNorth() const;
    VoxelChunk* GetNeighborSouth() const;
    VoxelChunk* GetNeighborEast() const;
    VoxelChunk* GetNeighborWest() const;
    unsigned GetLodLevel() const;

    protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();

    private:
    void OnVoxelChunkCreated();
    // Voxel chunk geometry
    unsigned char index_[3];
    unsigned char size_[3];
    float priority_;
    unsigned lodLevel_;
    unsigned numMeshes_;
    WeakPtr<VoxelSet> owner_;
    WeakPtr<VoxelChunk> neighborNorth_;
    WeakPtr<VoxelChunk> neighborWest_;
    WeakPtr<VoxelChunk> neighborEast_;
    WeakPtr<VoxelChunk> neighborSouth_;

    SharedPtr<VoxelMap> voxelMap_;
    Vector<SharedPtr<Geometry> > geometries_;
    Vector<SharedPtr<Material> > materials_;
    Vector<SharedPtr<VertexBuffer> > vertexData_;
    Vector<SharedPtr<IndexBuffer> > faceData_;
    Vector<SharedPtr<TextureBuffer> > faceBuffer_;
    Vector<unsigned > reducedQuadCount_;
    Vector<unsigned> numQuads_;
    Vector<bool> hasMaterialParameters_;
};

}
