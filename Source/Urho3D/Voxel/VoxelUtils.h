#pragma once

#include "VoxelDefs.h"
#include "VoxelEvents.h"

namespace Urho3D {

unsigned char URHO3D_API VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled);
unsigned char URHO3D_API VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast);
unsigned char URHO3D_API VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot, VoxelHeight height);
//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot);
//
//unsigned char VoxelEncodeGeometry(VoxelGeometry geometry);

//unsigned char VoxelEncodeFaceMask(bool east, bool north, bool west, bool south, bool up, bool down);
//
//unsigned char VoxelEncodeSideTextureSwap(unsigned char block, unsigned char overlay, unsigned char ecolor);


struct URHO3D_API VoxelRGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct URHO3D_API VoxelRange
{
    int startX;
    int startY;
    int startZ;
    int endX;
    int endY;
    int endZ;
};

struct URHO3D_API VoxelRangeFragment : VoxelRange
{
    int indexX;
    int indexY;
    int indexZ;
    int lenX;
    int lenY;
    int lenZ;
};


}
