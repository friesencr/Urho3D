#include "Voxel.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Generator.h"
#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"

#include "../DebugNew.h"

namespace Urho3D {

    static Vector<WeakPtr<VoxelMap> > voxelMapCache_;
	static void TouchCache(VoxelMap* voxelMap)
    {
		if (!voxelMap)
			return;

        for (unsigned i = 0; i < voxelMapCache_.Size(); ++i)
        {
			if (voxelMapCache_[i]->GetID() == voxelMap->GetID())
            {
				voxelMapCache_.Erase(i);
				break;
            }
        }
		voxelMapCache_.Insert(0, WeakPtr<VoxelMap>(voxelMap));
		while (voxelMapCache_.Size() > VoxelMap::MAX_LOADED_MAPS)
		{
			VoxelMap* unloadMap = voxelMapCache_[VoxelMap::MAX_LOADED_MAPS];
			if (unloadMap)
			{
				unloadMap->Unload();
				voxelMapCache_.Erase(VoxelMap::MAX_LOADED_MAPS);
			}
		}
    }

    unsigned char VoxelEncodeColor(Color c, bool tex1Enabled, bool tex2Enabled)
    {
	    return STBVOX_MAKE_COLOR(c.ToUInt(), tex1Enabled, tex2Enabled);
    }

    unsigned char VoxelEncodeVHeight(VoxelHeight southWest, VoxelHeight southEast, VoxelHeight northWest, VoxelHeight northEast)
    {
	    return STBVOX_MAKE_VHEIGHT(southWest, southEast, northWest, northEast);
    }

    unsigned char VoxelEncodeVHeight(VoxelHeight voxelHeight)
    {
	    return STBVOX_MAKE_VHEIGHT(voxelHeight, voxelHeight, voxelHeight, voxelHeight);
    }

    unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot, VoxelHeight height)
    {
	    return STBVOX_MAKE_GEOMETRY(geometry, rot, height);
    }

    unsigned char VoxelEncodeGeometry(VoxelGeometry geometry, VoxelRotation rot)
    {
	    return STBVOX_MAKE_GEOMETRY(geometry, rot, 0);
    }

    unsigned char VoxelEncodeGeometry(VoxelGeometry geometry)
    {
	    return STBVOX_MAKE_GEOMETRY(geometry, VOXEL_FACE_EAST,0);
    }

    /// Register object factory.
    void VoxelTextureMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelTextureMap>();
    }

    bool VoxelTextureMap::BeginLoad(Deserializer& source)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        ResourceRef diffuse1Ref = source.ReadResourceRef();
        ResourceRef diffuse2Ref = source.ReadResourceRef();
        this->defaultTexture2Map = source.ReadBuffer();
        SetDiffuse1Texture(cache->GetResource<Texture>(diffuse1Ref.name_));
        SetDiffuse2Texture(cache->GetResource<Texture>(diffuse2Ref.name_));
        return true;
    }

    void VoxelTextureMap::SetDiffuse1TextureArrayAttr(const ResourceRef& value)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Texture* texture = cache->GetResource<Texture>(value.name_);
        SetDiffuse2Texture(texture);
    }

    void VoxelTextureMap::SetDiffuse2TextureArrayAttr(const ResourceRef& value)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        SetDiffuse2Texture(cache->GetResource<Texture>(value.name_));
    }

    void VoxelTextureMap::SetDiffuse1Texture(Texture* texture)
    { 
        diffuse1Texture = texture;
    }

    void VoxelTextureMap::SetDiffuse2Texture(Texture* texture)
    { 
        diffuse2Texture = texture; 
    }

    /// Register object factory.
    void VoxelBlocktypeMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelBlocktypeMap>();
    }

    bool VoxelBlocktypeMap::BeginLoad(Deserializer& source)
    {
        if (source.ReadFileID() != "VOXB")
            return false;

        blockColor = source.ReadBuffer();
        blockGeometry = source.ReadBuffer();
        blockVHeight = source.ReadBuffer();
        blockTex1 = source.ReadBuffer();
        blockTex2 = source.ReadBuffer();
        blockSelector = source.ReadBuffer();
        blockSideTextureRotation = source.ReadBuffer();
        PODVector<unsigned char[6]>* faceData[3] = { &blockColorFace, &blockTex1Face, &blockTex2Face };
        for (unsigned set = 0; set < 3; ++set)
        {
            unsigned size = source.ReadVLE();
            faceData[set]->Resize(size);
            unsigned char* dataPtr = faceData[set]->Front();
            for (unsigned i = 0; i < size; ++i)
            {
                *dataPtr++ = source.ReadUByte();
                *dataPtr++ = source.ReadUByte();
                *dataPtr++ = source.ReadUByte();
                *dataPtr++ = source.ReadUByte();
                *dataPtr++ = source.ReadUByte();
                *dataPtr++ = source.ReadUByte();
            }
        }
        return true;
    }

    /// Saves voxel map information.
    bool VoxelBlocktypeMap::Save(Serializer& dest) 
    {
        if (!dest.WriteFileID("VOXB"))
            return false;

        dest.WriteBuffer(blockColor);
        dest.WriteBuffer(blockGeometry);
        dest.WriteBuffer(blockVHeight);
        dest.WriteBuffer(blockTex1);
        dest.WriteBuffer(blockTex2);
        dest.WriteBuffer(blockSelector);
        dest.WriteBuffer(blockSideTextureRotation);
        PODVector<unsigned char[6]>* faceData[3] = { &blockColorFace, &blockTex1Face, &blockTex2Face };
        for (unsigned set = 0; set < 3; ++set)
        {
            dest.WriteUInt(faceData[set]->Size());
            unsigned char* dataPtr = faceData[set]->Front();
            for (unsigned i = 0; i < faceData[set]->Size(); ++i)
            {
                dest.WriteUByte(*dataPtr++);
                dest.WriteUByte(*dataPtr++);
                dest.WriteUByte(*dataPtr++);
                dest.WriteUByte(*dataPtr++);
                dest.WriteUByte(*dataPtr++);
                dest.WriteUByte(*dataPtr++);
            }
        }
        return true;
    }

    /// Register object factory.
    void VoxelOverlayMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelOverlayMap>();
    }

    bool VoxelOverlayMap::BeginLoad(Deserializer& source)
    {
        return false;
    }

    /// Register object factory.
    void VoxelColorPalette::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelColorPalette>();
    }

    bool VoxelColorPalette::BeginLoad(Deserializer& source)
    {
        return false;
    }

    VoxelMap::VoxelMap(Context* context) 
        : Serializable(context),
        width_(0),
        height_(0),
        depth_(0),
        dataMask_(0),
		dataSourceType(0)
	{
		id_ = GetNextFreeID();
	}

    VoxelMap::~VoxelMap()
    {
    }

    /// Register object factory.
    void VoxelMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelMap>();
        ATTRIBUTE("Data Mask", unsigned, dataMask_, 0, AM_EDIT);
    }

    bool VoxelMap::Load(Deserializer& source)
    {
        if (!Serializable::Load(source))
            return false;

        if (!BeginLoad(source))
            return false;

		TouchCache(this);
		//LOGINFO(String(id_) + " Loaded");
        return true;
    }

    bool VoxelMap::BeginLoad(Deserializer& source)
    {
        width_ = source.ReadUInt();
        height_ = source.ReadUInt();
        depth_ = source.ReadUInt();
        dataMask_ = source.ReadUInt();
		zStride = height_ + 4;
		xStride = (height_ + 4) * (depth_ + 4);
		size_ = (width_ + 4)*(height_ + 4)*(depth_ + 4);

		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		GetBasicDataArrays(datas);
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
			if (dataMask_ & (1 << i))
				*datas[i] = source.ReadBuffer();
		}

        return true;
    }

    bool VoxelMap::IsLoaded()
    {
        if (width_ == 0 || height_ == 0 || depth_ == 0 || dataMask_ == 0)
            return false;

		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		GetBasicDataArrays(datas);
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
			if ((dataMask_ & (1 << i)) && datas[i]->Size() != size_)
				return false;
		}

        return true;
    }

    bool VoxelMap::Save(Serializer& dest)
    {
		dest.WriteUInt(width_);
		dest.WriteUInt(height_);
		dest.WriteUInt(depth_);
		dest.WriteUInt(dataMask_);

		if (!Reload())
			return false;

		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		GetBasicDataArrays(datas);
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
			if (dataMask_ & (1 << i))
				dest.WriteBuffer(*datas[i]);
		}
        return true;
    }

    void VoxelMap::Unload()
    {
		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		GetBasicDataArrays(datas);
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
			datas[i]->Clear();
			datas[i]->Compact();
		}
		//LOGINFO(String(id_) + " Unloaded");
    }

    bool VoxelMap::Reload(bool force)
    {
		if (!force && IsLoaded())
		{
			TouchCache(this);
			return true;
		}

		if (dataSourceType == 1)
		{
			Generator copy = generator_;
			return Load(copy);
		}
		else if (dataSourceType == 2)
		{
			ResourceCache* cache = GetSubsystem<ResourceCache>();
			SharedPtr<File> file = cache->GetFile(resourceRef_.name_);
			if (file && file->IsOpen())
				return Load(*file);
			else
				return false;
		}
        else
            return false;
    }

	void VoxelMap::SetSize(unsigned width, unsigned height, unsigned depth)
	{
		height_ = height;
		width_ = width;
		depth_ = depth;
		zStride = height + 4;
		xStride = (height + 4) * (depth + 4);
		size_ = (width_ + 4)*(height_ + 4)*(depth_ + 4);
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

    void VoxelMap::SetSource(Generator generator) 
    {
		dataSourceType = 1;
		generator_ = generator;
    }

	void VoxelMap::SetSource(ResourceRef resourceRef)
	{
		dataSourceType = 2;
		resourceRef_ = resourceRef;
	}
}
