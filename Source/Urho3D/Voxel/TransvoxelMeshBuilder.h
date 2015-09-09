#pragma once

#if 0
#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"

#include "VoxelMeshBuilder.h"

namespace Urho3D
{


class TransvoxelMeshBuilder : public VoxelMeshBuilder
{
public:
    TransvoxelMeshBuilder(Context* context);
    virtual ~TransvoxelMeshBuilder();

    virtual unsigned VoxelDataCompatibilityMask() const {
        return 0xffffffff;
    }

    virtual bool BuildMesh(VoxelWorkload* workload);

    virtual bool ProcessMeshFragment(VoxelWorkload* workload);

    virtual bool ProcessMesh(VoxelBuildSlot* slot);

    virtual bool UploadGpuData(VoxelJob* job);

    virtual bool UpdateMaterialParameters(Material* material);

protected:
    Vector<Variant> transform_;
    Vector<Variant> normals_;
    Vector<Variant> ambientTable_;
    Vector<Variant> texscaleTable_;
    Vector<Variant> texgenTable_;
    Vector<Variant> defaultColorTable_;
};

}
#endif
