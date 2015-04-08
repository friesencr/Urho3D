#pragma once

#include "../Math/Vector3.h"
#include "../../ThirdParty/STB/stb_voxel_render.h"

// Shapes of blocks that aren't always cubes
enum VoxelGeometryType
{
   VOXEL_TYPE_EMPTY,
   VOXEL_TYPE_KNOCKOUT,  // CREATES A HOLE IN THE MESH
   VOXEL_TYPE_SOLID,
   VOXEL_TYPE_TRANSP,    // SOLID GEOMETRY, BUT TRANSPARENT CONTENTS SO NEIGHBORS GENERATE NORMALLY, UNLESS SAME BLOCKTYPE

   // FO_TYPELLOWING 4 CAN BE REPRESENTED BY VHEIGHT AS WELL
   VOXEL_TYPE_SLAB_UPPER,
   VOXEL_TYPE_SLAB_LOWER,
   VOXEL_TYPE_FLOOR_SLOPE_NORTH_IS_TOP,
   VOXEL_TYPE_CEIL_SLOPE_NORTH_IS_BOTTOM,

   VOXEL_TYPE_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED,   // SAME AS FLOOR_SLOPE ABOVE, BUT USES WALL'S TEXTURE & TEXTURE PROJECTION
   VOXEL_TYPE_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED,
   VOXEL_TYPE_CROSSED_PAIR,    // CORNER-TO-CORNER PAIRS, WITH NORMAL VECTOR BUMPED UPWARDS
   VOXEL_TYPE_FORCE,           // LIKE GEOM_TRANSP, BUT FACES VISIBLE EVEN IF NEIGHBOR IS SAME TYPE, E.G. MINECRAFT FANCY LEAVES

   // TH_TYPEESE ACCESS VHEIGHT INPUT
   VOXEL_TYPE_FLOOR_VHEIGHT_03 = 12,  // DIAGONAL IS SW-NE
   VOXEL_TYPE_FLOOR_VHEIGHT_12,       // DIAGONAL IS SE-NW
   VOXEL_TYPE_CEIL_VHEIGHT_03,
   VOXEL_TYPE_CEIL_VHEIGHT_12,

   VOXEL_TYPE_COUNT, // NUMBER OF GEOM CASES
};

enum VoxelRotation
{
   VOXEL_FACE_EAST,
   VOXEL_FACE_NORTH,
   VOXEL_FACE_WEST,
   VOXEL_FACE_SOUTH,
   VOXEL_FACE_UP,
   VOXEL_FACE_DOWN,

   VOXEL_FACE_COUNT,
};

struct VoxelTextureBlock {
    unsigned char east;
    unsigned char north;
    unsigned char west;
    unsigned char south;
    unsigned char up;
    unsigned char down;
};

union VoxelTextureMapping {
   VoxelTextureBlock textureBlock;
   char characterMap[6];
};

struct VoxelDefinition {
   VoxelGeometryType type;
   VoxelRotation rotation;
   VoxelTextureBlock textureBlock;
};

class URHO3D_API Voxel {
   char voxelDefinition_;
};