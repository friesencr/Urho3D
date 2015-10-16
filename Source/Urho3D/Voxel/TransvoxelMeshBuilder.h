#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"
#include "../Graphics/IndexBuffer.h"

#include "VoxelMeshBuilder.h"


namespace Urho3D
{

struct VoxelBuildSlot;

static const unsigned TRANSVOXEL_WORKER_VERTEX_BUFFER_SIZE = 100000 * (sizeof(Vector3) + sizeof(Vector3) + sizeof(unsigned));
static const unsigned TRANSVOXEL_WORKER_INDEX_BUFFER_SIZE = 100000;

struct TransvoxelWorkBuffer
{
    unsigned vertexMask;
    BoundingBox box;
    unsigned numTriangles;
    unsigned numVerticies;
    unsigned numIndicies;
    unsigned char vertexBuffer[TRANSVOXEL_WORKER_VERTEX_BUFFER_SIZE];
    unsigned indexBuffer[TRANSVOXEL_WORKER_INDEX_BUFFER_SIZE];
};

class TransvoxelMeshBuilder : public VoxelMeshBuilder
{
    OBJECT(TransvoxelMeshBuilder);
public:
    TransvoxelMeshBuilder(Context* context);
    virtual ~TransvoxelMeshBuilder() {}

    virtual unsigned VoxelDataCompatibilityMask() const;

    virtual bool BuildMesh(VoxelBuildSlot* workload);

    virtual bool ProcessMesh(VoxelBuildSlot* slot);

    virtual bool UploadGpuData(VoxelJob* job);

    virtual bool UpdateMaterialParameters(Material* material);

    virtual void AssignWork(VoxelBuildSlot* slot);

    virtual void FreeWork(VoxelBuildSlot* slot);

protected:
    Vector<TransvoxelWorkBuffer> workBuffers_;
};

}
