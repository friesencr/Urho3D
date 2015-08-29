#pragma once

#include "../Core/Context.h"

#include "VoxelDefs.h"
#include "VoxelEvents.h"
#include "VoxelUtils.h"
#include "VoxelBlocktypeMap.h"
#include "VoxelColorPalette.h"
#include "VoxelOverlayMap.h"
#include "VoxelTextureMap.h"
#include "VoxelMap.h"
#include "VoxelWriter.h"
#include "VoxelChunk.h"
#include "VoxelBuilder.h"
#include "VoxelStore.h"
#include "VoxelSet.h"
#include "VoxelStreamer.h"

namespace Urho3D {

void URHO3D_API RegisterVoxelLibrary(Context* context)
{
    VoxelBlocktypeMap::RegisterObject(context);
    VoxelOverlayMap::RegisterObject(context);
    VoxelTextureMap::RegisterObject(context);
    VoxelColorPalette::RegisterObject(context);
    VoxelMap::RegisterObject(context);
    VoxelChunk::RegisterObject(context);
    VoxelMapPage::RegisterObject(context);
    VoxelStore::RegisterObject(context);
    VoxelSet::RegisterObject(context);
    VoxelStreamer::RegisterObject(context);
}

}
