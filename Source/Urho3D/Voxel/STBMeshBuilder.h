#pragma once

#include <STB/stb_voxel_render.h>

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

struct STBWorkBuffer
{
    int fragmentQuads[VOXEL_MAX_WORKERS];
    BoundingBox box[VOXEL_MAX_WORKERS];
    stbvox_mesh_maker meshMakers[VOXEL_MAX_WORKERS];
    unsigned char workVertexBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_VERTEX_BUFFER_SIZE];
    unsigned char workFaceBuffers[VOXEL_MAX_WORKERS][VOXEL_WORKER_FACE_BUFFER_SIZE];
};

class STBMeshBuilder : public VoxelMeshBuilder
{

public:
    STBMeshBuilder(Context* context);
    virtual ~STBMeshBuilder() {}

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
    SharedPtr<IndexBuffer> sharedIndexBuffer_;
    Vector<Variant> transform_;
    Vector<Variant> normals_;
    Vector<Variant> ambientTable_;
    Vector<Variant> texscaleTable_;
    Vector<Variant> texgenTable_;
    Vector<Variant> defaultColorTable_;
    Vector<STBWorkBuffer> workBuffers_;
};

}
