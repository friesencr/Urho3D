#include "../Core/Context.h"

#include "VoxelColorPalette.h"

#include "../DebugNew.h"

namespace Urho3D 
{

    void VoxelColorPalette::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelColorPalette>("Voxel");
    }

    bool VoxelColorPalette::BeginLoad(Deserializer& source)
    {
        return false;
    }

}
