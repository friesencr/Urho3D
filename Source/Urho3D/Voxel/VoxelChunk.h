#pragma once

#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Container/Ptr.h"
#include "../Core/Mutex.h"
#include "../Graphics/TextureBuffer.h"

#include "VoxelMap.h"
#include "VoxelSet.h"

namespace Urho3D {

URHO3D_API class VoxelMap;
struct VoxelJob;

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

    //bool GetHasShaderParameters(unsigned index);
    //void SetHasShaderParameters(unsigned index, bool isRequired);

    /// Set mesh selector material.
    Material* GetMaterial(unsigned selector) const;

    /// Set material by mesh selector.
    void SetMaterial(unsigned selector, Material* material);

    /// Set local-space bounding box.
    void SetBoundingBox(const BoundingBox& box);

    void SetNumberOfMeshes(unsigned count);

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
    bool Build(VoxelMap* voxelMap, VoxelMap* northMap, VoxelMap* southMap, VoxelMap* eastMap, VoxelMap* westMap);

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();

private:
    void UpdateMaterialParameters(unsigned slot, VoxelMap* voxelMap);
    bool BuildInternal(bool async);
    void OnVoxelChunkCreated();
    // Voxel chunk geometry
    unsigned char index_[3];
    unsigned char size_[3];
    unsigned numMeshes_;
    VoxelJob* voxelJob_;
    WeakPtr<VoxelMap> voxelMap_;
    ResourceRef resourceRef_;

    Vector<SharedPtr<Geometry> > geometries_;
    Vector<SharedPtr<Material> > materials_;
    Vector<SharedPtr<VertexBuffer> > vertexData_;
    Vector<SharedPtr<IndexBuffer> > faceData_;
    Vector<SharedPtr<TextureBuffer> > faceBuffer_;
    Vector<unsigned > reducedQuadCount_;
    Vector<unsigned> numQuads_;
    Vector<bool> updateMaterialParameters_;
};

}
