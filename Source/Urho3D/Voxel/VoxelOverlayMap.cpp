#include "../Core/Context.h"

#include "VoxelOverlayMap.h"

#include "../DebugNew.h"

namespace Urho3D 
{

void VoxelOverlayMap::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelOverlayMap>("Voxel");
}

bool VoxelOverlayMap::BeginLoad(Deserializer& source)
{
    return false;
}

}
