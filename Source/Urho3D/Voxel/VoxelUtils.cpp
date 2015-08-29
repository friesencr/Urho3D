#include <STB/stb_voxel_render.h>

#include "VoxelUtils.h"

#include "../DebugNew.h"

namespace Urho3D 
{
unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled)
{
    return STBVOX_MAKE_COLOR(c.ToUInt(), tex1Enabled, tex2Enabled);
}

unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast)
{
    return STBVOX_MAKE_VHEIGHT(southWest, southEast, northWest, northEast);
}

//unsigned char VoxelEncodeVHeight(VoxelHeight voxelHeight)
//{
   // return STBVOX_MAKE_VHEIGHT(voxelHeight, voxelHeight, voxelHeight, voxelHeight);
//}

unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot, VoxelHeight height)
{
    return STBVOX_MAKE_GEOMETRY(geometry, rot, height);
}

//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot)
//{
   // return STBVOX_MAKE_GEOMETRY(geometry, rot, 0);
//}

//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry)
//{
   // return STBVOX_MAKE_GEOMETRY(geometry, VOXEL_FACE_EAST,0);
//}

}
