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

struct VoxelBuildSlot;
struct VoxelJob;

const unsigned VOXEL_STB_WORKER_MAX_QUADS          = 100000;
const unsigned VOXEL_STB_WORKER_VERTEX_BUFFER_SIZE = VOXEL_STB_WORKER_MAX_QUADS * 4 * 4; // 4 uint vertices
const unsigned VOXEL_STB_WORKER_FACE_BUFFER_SIZE   = VOXEL_STB_WORKER_MAX_QUADS * 4;

struct STBWorkBuffer
{
    int fragmentQuads;
    stbvox_mesh_maker meshMaker;
    unsigned char workVertexBuffer[VOXEL_STB_WORKER_VERTEX_BUFFER_SIZE];
    unsigned char workFaceBuffer[VOXEL_STB_WORKER_FACE_BUFFER_SIZE];
};

class STBMeshBuilder : public VoxelMeshBuilder
{
    OBJECT(STBMeshBuilder);
public:
    STBMeshBuilder(Context* context);
    virtual ~STBMeshBuilder() {}

    virtual unsigned VoxelDataCompatibilityMask() const;

    virtual bool BuildMesh(VoxelBuildSlot* workload);

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
    Vector<STBWorkBuffer> workBuffers_;
};

}
