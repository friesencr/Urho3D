#include "VoxelMeshBuilder.h"

namespace Urho3D
{

void VoxelMeshBuilder::GetVoxelDataDatas(VoxelData* voxelData, PODVector<unsigned char>** datas)
{
    voxelData->GetBasicDataArrays(datas);
}

}
