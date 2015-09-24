#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"
#include "../Graphics/IndexBuffer.h"

#include "VoxelMeshBuilder.h"

namespace Urho3D
{

struct VoxelWorkload;
struct VoxelBuildSlot;

struct TransvoxelWorkBuffer
{
    int fragmentTriangles[VOXEL_MAX_WORKERS];
    int fragmentVertices[VOXEL_MAX_WORKERS];
    BoundingBox box[VOXEL_MAX_WORKERS];
    unsigned char workVertexBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_VERTEX_BUFFER_SIZE];
    unsigned char workIndexBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_FACE_BUFFER_SIZE];
    unsigned vertexMask;
};

class TransvoxelMeshBuilder : public VoxelMeshBuilder
{

public:
    TransvoxelMeshBuilder(Context* context);
    virtual ~TransvoxelMeshBuilder() {}

    virtual unsigned VoxelDataCompatibilityMask() const;

    virtual bool BuildMesh(VoxelWorkload* workload);

    virtual bool ProcessMeshFragment(VoxelWorkload* workload);

    virtual bool ProcessMesh(VoxelBuildSlot* slot);

    virtual bool UploadGpuData(VoxelJob* job);

    virtual bool UpdateMaterialParameters(Material* material);

    virtual void AssignWork(VoxelBuildSlot* slot);

    virtual void FreeWork(VoxelBuildSlot* slot);

protected:
    bool ResizeIndexBuffer(unsigned numQuads);
    Vector<TransvoxelWorkBuffer> workBuffers_;
};

}
