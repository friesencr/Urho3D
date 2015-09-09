#include "STBMeshBuilder.h"

class ClassicMeshBuilder : STBMeshBuilder
{
    virtual bool UploadGpuData(VoxelJob* job)
    {
        //if (!vb->SetSize(totalVertices, MASK_POSITION | MASK_TEXCOORD1 | MASK_NORMAL, true))
        //    return false;
        return false;
    }
};
