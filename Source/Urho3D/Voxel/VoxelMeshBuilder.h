#pragma once

#include "../Core/Context.h"
#include "../Core/Object.h"
#include "../Container/Vector.h"
#include "../Graphics/Material.h"

#include "VoxelData.h"

namespace Urho3D
{
class VoxelData;
struct VoxelJob;
struct VoxelWorkload;
struct VoxelBuildSlot;

class VoxelMeshBuilder : public Object
{
    OBJECT(VoxelMeshBuilder);
    BASEOBJECT(VoxelMeshBuilder);

public:
    VoxelMeshBuilder(Context* context) : Object(context)
    {

    }

    virtual ~VoxelMeshBuilder() {

    }

    virtual unsigned VoxelDataCompatibilityMask() const = 0;
    virtual bool BuildMesh(VoxelWorkload* workload) = 0;
    virtual bool ProcessMeshFragment(VoxelWorkload* workload) = 0;
    virtual bool ProcessMesh(VoxelBuildSlot* slot) = 0;
    virtual bool UploadGpuData(VoxelJob* job) = 0;
    virtual bool UpdateMaterialParameters(Material* material) = 0;
    virtual void AssignWork(VoxelBuildSlot* slot) = 0;
    virtual void FreeWork(VoxelBuildSlot* slot) = 0;

protected:
    void GetVoxelDataDatas(VoxelData* voxelData, PODVector<unsigned char>** datas);
};

};
