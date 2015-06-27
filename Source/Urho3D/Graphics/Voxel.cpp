#include "Voxel.h"
#include "../IO/Deserializer.h"
#include "../IO/Generator.h"

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

    VoxelMap::VoxelMap(Context* context) 
        : Resource(context),
        width_(0),
        height_(0),
        depth_(0),
        dataMask_(0)
	{

	}

    VoxelMap::~VoxelMap()
    {
    }

    bool VoxelMap::BeginLoad(Deserializer& source)
    {
        width_ = source.ReadUInt();
        height_ = source.ReadUInt();
        depth_ = source.ReadUInt();
        dataMask_ = source.ReadUInt();
		zStride = height_ + 2;
		xStride = (height_ + 2) * (depth_ + 2);
		size_ = (width_ + 2)*(height_ + 2)*(depth_ + 2);

        if (dataMask_ & VOXEL_BLOCK_BLOCKTYPE)
            blocktype = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_COLOR)
            color = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_GEOMETRY)
            geometry = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_VHEIGHT)
            vHeight = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_LIGHTING)
            lighting = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_ROTATE)
            rotate = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        if (dataMask_ & VOXEL_BLOCK_TEX2)
            tex2 = PODVector<unsigned char>(&source.ReadBuffer().Front(), size_);

        return true;
    }

    bool VoxelMap::IsLoaded()
    {
        if (width_ == 0 || height_ == 0 || depth_ == 0 || dataMask_ == 0)
            return false;

        if (dataMask_ & VOXEL_BLOCK_BLOCKTYPE && blocktype.Size() != size_)
            return false;

        if (dataMask_ & VOXEL_BLOCK_COLOR && color.Size() != size_)
            return false;

        if (dataMask_ & VOXEL_BLOCK_GEOMETRY && geometry.Size() != size_)
            return false;

        if (dataMask_ & VOXEL_BLOCK_VHEIGHT && vHeight.Size() != size_)
            return false;

        if (dataMask_ & VOXEL_BLOCK_ROTATE && rotate.Size() != size_)
            return false;

        if (dataMask_ & VOXEL_BLOCK_TEX2 && tex2.Size() != size_)
            return false;

        return true;
    }

    bool VoxelMap::Save(Serializer& dest)
    {
        return true;
    }

    void VoxelMap::Unload()
    {
        blocktype.Clear();
        blocktype.Compact();
        color.Clear();
        color.Compact();
        geometry.Clear();
        geometry.Compact();
        vHeight.Clear();
        vHeight.Compact();
        lighting.Clear();
        lighting.Compact();
        rotate.Clear();
        rotate.Compact();
        tex2.Clear();
        tex2.Compact();
    }

    bool VoxelMap::Reload()
    {
        if (source_.NotNull())
        {
            Generator* sourceGenerator = (Generator*)source_.Get();
            Generator copy(context_);
            copy.SetName(sourceGenerator->GetName());
            copy.SetParameters(sourceGenerator->GetParameters());
            return Load(copy);
        }
        else
            return false;
    }

	void VoxelMap::SetSize(unsigned width, unsigned height, unsigned depth)
	{
		height_ = height;
		width_ = width;
		depth_ = depth;
		zStride = height + 2;
		xStride = (height + 2) * (depth + 2);
		size_ = (width_ + 2)*(height_ + 2)*(depth_ + 2);
	}

	void VoxelMap::InitializeBlocktype(unsigned char initialValue)
	{
		blocktype.Resize(size_);
		memset(&blocktype.Front(), initialValue, sizeof(char) * size_);
	}

	void VoxelMap::InitializeVHeight(unsigned char initialValue)
	{
		vHeight.Resize(size_);
		memset(&vHeight.Front(), initialValue, sizeof(char) * size_);
	}

	void VoxelMap::InitializeLighting(unsigned char initialValue)
	{
		lighting.Resize(size_);
		memset(&lighting.Front(), initialValue, sizeof(char) * size_);
	}

	void VoxelMap::InitializeColor(unsigned char initialValue)
	{
		color.Resize(size_);
		memset(&color.Front(), initialValue, sizeof(char) * size_);
	}

	void VoxelMap::InitializeTex2(unsigned char initialValue)
	{
		tex2.Resize(size_);
		memset(&tex2.Front(), initialValue, sizeof(char) * size_);
	}

	void VoxelMap::InitializeGeometry(unsigned char initialValue)
	{
		geometry.Resize(size_);
		memset(&geometry.Front(), initialValue, sizeof(char) * size_);
	}

    void VoxelMap::SetSource(Object* deserializer) 
    {
        source_ = deserializer;
    }
}
