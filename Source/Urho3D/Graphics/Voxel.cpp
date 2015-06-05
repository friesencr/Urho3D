#include "Voxel.h"

namespace Urho3D {

	unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled)
	{
		return STBVOX_MAKE_COLOR(c.ToUInt(), tex1Enabled, tex2Enabled);
	}

	unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast)
	{
		return STBVOX_MAKE_VHEIGHT(southWest, southEast, northWest, northEast);
	}

	unsigned char VoxelEncodeGeometry(VoxelGeometryType type, VoxelRotation rot, VoxelHeight height)
	{
		return STBVOX_MAKE_GEOMETRY(type, rot, height);
	}

	unsigned char VoxelEncodeGeometry(VoxelGeometryType type, VoxelRotation rot)
	{
		return STBVOX_MAKE_GEOMETRY(type, rot, 0);
	}

	unsigned char VoxelEncodeGeometry(VoxelGeometryType type)
	{
		return STBVOX_MAKE_GEOMETRY(type, VOXEL_FACE_EAST,0);
	}

}
