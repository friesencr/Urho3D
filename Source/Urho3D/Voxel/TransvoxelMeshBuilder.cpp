#if 0
#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#include "TransvoxelMeshBuilder.h"

namespace Urho3D
{

TransvoxelMeshBuilder(Context* context) : VoxelMeshBuilder(context) 
{

}

bool TransvoxelMeshBuilder::BuildMesh(VoxelWorkload* workload) 
{
    return true;
}

bool TransvoxelMeshBuilder::ProcessMeshFragment(VoxelWorkload* workload)
{
    return true;
}

bool TransvoxelMeshBuilder::ProcessMesh(VoxelBuildSlot* slot)
{
    return true;
}

bool TransvoxelMeshBuilder::UploadGpuData(VoxelJob* job)
{
    return true;
}

bool TransvoxelMeshBuilder::UpdateMaterialParameters(Material* material)
{
    return true;
}

}
#endif
