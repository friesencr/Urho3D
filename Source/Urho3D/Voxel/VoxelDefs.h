#pragma once

namespace Urho3D 
{

static const unsigned VOXEL_BLOCK_BLOCKTYPE      = 0x00000001;
static const unsigned VOXEL_BLOCK_COLOR2         = 0x00000002;
static const unsigned VOXEL_BLOCK_COLOR2FACEMASK = 0x00000004;
static const unsigned VOXEL_BLOCK_COLOR3         = 0x00000008;
static const unsigned VOXEL_BLOCK_COLOR3FACEMASK = 0x00000010;
static const unsigned VOXEL_BLOCK_COLOR          = 0x00000020;
static const unsigned VOXEL_BLOCK_ECOLOR         = 0x00000040;
static const unsigned VOXEL_BLOCK_ECOLORFACEMASK = 0x00000080;
static const unsigned VOXEL_BLOCK_EXTENDEDCOLOR  = 0x00000100;
static const unsigned VOXEL_BLOCK_GEOMETRY       = 0x00000200;
static const unsigned VOXEL_BLOCK_LIGHTING       = 0x00000400;
static const unsigned VOXEL_BLOCK_OVERLAY        = 0x00000800;
static const unsigned VOXEL_BLOCK_ROTATE         = 0x00001000;
static const unsigned VOXEL_BLOCK_TEX2           = 0x00002000;
static const unsigned VOXEL_BLOCK_TEX2FACEMASK   = 0x00004000;
static const unsigned VOXEL_BLOCK_TEX2REPLACE    = 0x00008000;
static const unsigned VOXEL_BLOCK_VHEIGHT        = 0x00010000;

static const unsigned VOXEL_NEIGHBOR_NORTH = 0x1;
static const unsigned VOXEL_NEIGHBOR_SOUTH = 0x2;
static const unsigned VOXEL_NEIGHBOR_EAST  = 0x4;
static const unsigned VOXEL_NEIGHBOR_WEST  = 0x8;

static const unsigned VOXEL_WORKER_SIZE_X             = 32;
static const unsigned VOXEL_WORKER_SIZE_Y             = 32;
static const unsigned VOXEL_WORKER_SIZE_Z             = 32;
static const unsigned VOXEL_MAX_WORKERS               = 1; // chunk size / worker size

static const unsigned VOXEL_CHUNK_SIZE_X              = 32;
static const unsigned VOXEL_CHUNK_SIZE_Y              = 32;
static const unsigned VOXEL_CHUNK_SIZE_Z              = 32;
static const unsigned VOXEL_CHUNK_SIZE                = VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Y * VOXEL_CHUNK_SIZE_Z;

static const unsigned VOXEL_STORE_PAGE_SIZE_1D        = 8;
static const unsigned VOXEL_STORE_PAGE_MASK_1D        = 0x7;
static const unsigned VOXEL_STORE_PAGE_SIZE_2D        = 8 * 8;
static const unsigned VOXEL_STORE_PAGE_SIZE_3D        = 8 * 8 * 8;
static const unsigned VOXEL_STORE_PAGE_STRIDE_X       = 6; // bits
static const unsigned VOXEL_STORE_PAGE_STRIDE_Z       = 3; // bits

static const unsigned char VOXEL_COMPRESSION_NONE = 0x00;
static const unsigned char VOXEL_COMPRESSION_RLE  = 0x01;
static const unsigned char VOXEL_COMPRESSION_LZ4  = 0x02;

static const unsigned VOXEL_DATA_NUM_BASIC_STREAMS     = 17;

// Shapes of blocks that aren't always cubes
enum VoxelGeometry
{
    VOXEL_GEOMETRY_EMPTY,
    VOXEL_GEOMETRY_KNOCKOUT,  // CREATES A HOLE IN THE MESH
    VOXEL_GEOMETRY_SOLID,
    VOXEL_GEOMETRY_TRANSP,    // SOLID GEOMETRY, BUT TRANSPARENT CONTENTS SO NEIGHBORS GENERATE NORMALLY, UNLESS SAME BLOCKGEOMETRY

    // FO_GEOMETRYLLOWING 4 CAN BE REPRESENTED BY VHEIGHT AS WELL
    VOXEL_GEOMETRY_SLAB_UPPER,
    VOXEL_GEOMETRY_SLAB_LOWER,
    VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP,
    VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM,

    VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED,   // SAME AS FLOOR_SLOPE ABOVE, BUT USES WALL'S TEXTURE & TEXTURE PROJECTION
    VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED,
    VOXEL_GEOMETRY_CROSSED_PAIR,    // CORNER-TO-CORNER PAIRS, WITH NORMAL VECTOR BUMPED UPWARDS
    VOXEL_GEOMETRY_FORCE,           // LIKE GEOM_TRANSP, BUT FACES VISIBLE EVEN IF NEIGHBOR IS SAME GEOMETRY, E.G. MINECRAFT FANCY LEAVES

    // TH_GEOMETRYESE ACCESS VHEIGHT INPUT
    VOXEL_GEOMETRY_FLOOR_VHEIGHT_03 = 12,  // DIAGONAL IS SW-NE
    VOXEL_GEOMETRY_FLOOR_VHEIGHT_12,       // DIAGONAL IS SE-NW
    VOXEL_GEOMETRY_CEIL_VHEIGHT_03,
    VOXEL_GEOMETRY_CEIL_VHEIGHT_12,

    VOXEL_GEOMETRY_COUNT, // NUMBER OF GEOM CASES
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

enum VoxelHeight
{
    VOXEL_HEIGHT_0,
    VOXEL_HEIGHT_HALF,
    VOXEL_HEIGHT_1,
    VOXEL_HEIGHT_ONE_AND_HALF,
};

enum VoxelFaceRotation
{
    VOXEL_FACEROT_0,
    VOXEL_FACEROT_90,
    VOXEL_FACEROT_180,
    VOXEL_FACEROT_270
};

}
