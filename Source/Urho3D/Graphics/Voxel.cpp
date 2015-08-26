#include "Voxel.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Generator.h"
#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../IO/Compression.h"
#include "../Scene/Component.h"
#include "../Core/Profiler.h"

#include "../DebugNew.h"

namespace Urho3D {

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

    /// Register object factory.
    void VoxelTextureMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelTextureMap>("Voxel");
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
        context->RegisterFactory<VoxelBlocktypeMap>("Voxel");
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
        context->RegisterFactory<VoxelOverlayMap>("Voxel");
    }

    bool VoxelOverlayMap::BeginLoad(Deserializer& source)
    {
        return false;
    }

    /// Register object factory.
    void VoxelColorPalette::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelColorPalette>("Voxel");
    }

    bool VoxelColorPalette::BeginLoad(Deserializer& source)
    {
        return false;
    }

    VoxelMap::VoxelMap(Context* context) 
        : Resource(context),
        width_(0),
        height_(0),
        depth_(0),
        dataMask_(0),
        blocktypeMap(0)
	{
	}

    VoxelMap::~VoxelMap()
    {
    }

    /// Register object factory.
    void VoxelMap::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelMap>("Voxel");
    }

    void VoxelMap::EncodeData(VoxelMap* voxelMap, Serializer& dest)
    {
		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		voxelMap->GetBasicDataArrays(datas);
        unsigned dataMask = voxelMap->GetDataMask();
        unsigned size = voxelMap->GetSize();
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
            if (dataMask & (1 << i))
            {
                unsigned char* data = &datas[i]->Front();
                if (!datas[i]->Size())
                    continue;

                unsigned char val = data[0];
                unsigned count = 1;
                unsigned position = 1;
                do
                {
                    if (position == size || val != data[position])
                    {
                        dest.WriteVLE(count);
                        dest.WriteUByte(val);
                        count = 0;
                        val = data[position];
                    }
                    count++;
                } while (position++ < size);
            }
		}
    }

    unsigned VoxelMap::DecodeData(VoxelMap* voxelMap, Deserializer& source)
    {
        unsigned numDatas = 0;
		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		voxelMap->GetBasicDataArrays(datas);
        unsigned dataMask = voxelMap->GetDataMask();
        unsigned size = voxelMap->GetSize();

		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
			if (dataMask & (1 << i))
			{
                numDatas++;
				datas[i]->Resize(size);
                unsigned char* dataPtr = &datas[i]->Front();
                unsigned position = 0;
                while (position < size)
                {
                    unsigned count = source.ReadVLE();
                    unsigned char val = source.ReadUByte();
                    memset(dataPtr, val, count);
                    dataPtr += count;
                    position += count;
                }
			}
		}
        return numDatas * size;
    }

    bool VoxelMap::BeginLoad(Deserializer& source)
    {
        PROFILE(VoxelMapLoad);
        if (source.ReadFileID() != "VOXM")
            return false;

        PODVector<unsigned char> buffer(source.GetSize());
        source.Read(&buffer.Front(), source.GetSize());
        MemoryBuffer memBuffer(buffer);

        SetSize(memBuffer.ReadUInt(), memBuffer.ReadUInt(), memBuffer.ReadUInt());
        dataMask_ = memBuffer.ReadUInt();
        
        ResourceRef ref = memBuffer.ReadResourceRef();
        loadVoxelBlocktypeMap_ = ref.name_;

        unsigned dataSize = DecodeData(this, memBuffer);
        SetMemoryUse(sizeof(VoxelMap) + dataSize);
        return true;
    }

    bool VoxelMap::EndLoad()
    {
        if (!loadVoxelBlocktypeMap_.Empty())
        {
            SetBlocktypeMapAttr(ResourceRef(VoxelBlocktypeMap::GetTypeStatic(), loadVoxelBlocktypeMap_));
            loadVoxelBlocktypeMap_ = String::EMPTY;
        }
        return true;
    }

    bool VoxelMap::Save(Serializer& dest)
    {
        if (!dest.WriteFileID("VOXM"))
        {
            LOGERROR("Failed to save voxel map");
            return false;
        }
		dest.WriteUInt(width_);
		dest.WriteUInt(height_);
		dest.WriteUInt(depth_);
		dest.WriteUInt(dataMask_);
        dest.WriteResourceRef(GetBlocktypeMapAttr());

        return true;
    }

	void VoxelMap::SetSize(unsigned width, unsigned height, unsigned depth)
	{
		height_ = height;
		width_ = width;
		depth_ = depth;
		strideZ = height + 4;
		strideX = (height + 4) * (depth + 4);
		size_ = (width_ + 4)*(height_ + 4)*(depth_ + 4);

		PODVector<unsigned char>* datas[NUM_BASIC_STREAMS];
		GetBasicDataArrays(datas);
		for (unsigned i = 0; i < NUM_BASIC_STREAMS; ++i)
		{
            if (dataMask_ & (1 << i))
                datas[i]->Resize(size_);
        }
	}

    void VoxelMap::SetBlocktypeMap(VoxelBlocktypeMap* voxelBlocktypeMap)
    {
        blocktypeMap = voxelBlocktypeMap;
    }

    void VoxelMap::SetBlocktypeMapAttr(const ResourceRef& value)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        VoxelBlocktypeMap* blocktypeMap = cache->GetResource<VoxelBlocktypeMap>(value.name_);
        SetBlocktypeMap(blocktypeMap);
    }

    ResourceRef VoxelMap::GetBlocktypeMapAttr() const
    {
        return GetResourceRef(blocktypeMap, VoxelBlocktypeMap::GetTypeStatic());
    }

    static const int MAP_NORTH = 0;
    static const int MAP_SOUTH = 1;
    static const int MAP_EAST = 2;
    static const int MAP_WEST = 3;

    void VoxelMap::TransferAdjacentData(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west)
    {
        TransferAdjacentDataInternal(north, MAP_NORTH);
        TransferAdjacentDataInternal(south, MAP_SOUTH);
        TransferAdjacentDataInternal(east, MAP_EAST);
        TransferAdjacentDataInternal(west, MAP_WEST);
    }

    void VoxelMap::TransferAdjacentNorthData(VoxelMap* source)
    {
        TransferAdjacentDataInternal(source, MAP_NORTH);
    }

    void VoxelMap::TransferAdjacentSouthData(VoxelMap* source)
    {
        TransferAdjacentDataInternal(source, MAP_SOUTH);
    }

    void VoxelMap::TransferAdjacentEastData(VoxelMap* source)
    {
        TransferAdjacentDataInternal(source, MAP_EAST);
    }

    void VoxelMap::TransferAdjacentWestData(VoxelMap* source)
    {
        TransferAdjacentDataInternal(source, MAP_WEST);
    }

    void VoxelMap::TransferAdjacentDataInternal(VoxelMap* source, int direction)
    {
        if (!source)
            return;

        // Copy adjacent data
        PODVector<unsigned char>* srcDatas[VoxelMap::NUM_BASIC_STREAMS];
        PODVector<unsigned char>* destDatas[VoxelMap::NUM_BASIC_STREAMS];
        source->GetBasicDataArrays(srcDatas);
        this->GetBasicDataArrays(destDatas);

        for (unsigned i = 0; i < VoxelMap::NUM_BASIC_STREAMS; ++i)
        {
            if (!this->dataMask_ & (1 << i))
                continue;

            if (!source->dataMask_ & (1 << i))
                continue;

            unsigned char* src = 0;
            unsigned char* dst = 0;

            if (srcDatas[i]->Size() == this->size_)
            {
                if (destDatas[i]->Size() == this->size_)
                {
                    src = &srcDatas[i]->Front();
                    dst = &destDatas[i]->Front();
                }
            }

            if (!(src && dst))
                continue;

            unsigned char* srcPtr = src;
            unsigned char* dstPtr = dst;

            if (direction == MAP_NORTH)
            {
                for (unsigned x = 0; x < this->width_; ++x)
                    for (unsigned y = 0; y < this->height_; ++y)
                    {
                        dstPtr[this->GetIndex(x, y, this->depth_)] = srcPtr[source->GetIndex(x, y, 0)];
                        dstPtr[this->GetIndex(x, y, this->depth_ + 1)] = srcPtr[source->GetIndex(x, y, 1)];
                    }
            }
            else if (direction == MAP_SOUTH)
            {
                for (unsigned x = 0; x < this->width_; ++x)
                    for (unsigned y = 0; y < this->height_; ++y)
                    {
                        dstPtr[this->GetIndex(x, y, -1)] = srcPtr[source->GetIndex(x, y, source->depth_ - 1)];
                        dstPtr[this->GetIndex(x, y, -2)] = srcPtr[source->GetIndex(x, y, source->depth_ - 2)];
                    }
            }
            else if (direction == MAP_EAST)
            {
                for (unsigned z = 0; z < this->depth_; ++z)
                    for (unsigned y = 0; y < this->height_; ++y)
                    {
                        dstPtr[this->GetIndex(this->width_, y, z)] = srcPtr[source->GetIndex(0, y, z)];
                        dstPtr[this->GetIndex(this->width_ + 1, y, z)] = srcPtr[source->GetIndex(1, y, z)];
                    }
            }
            else if (direction == MAP_WEST)
            {
                for (unsigned z = 0; z < this->depth_; ++z)
                    for (unsigned y = 0; y < this->height_; ++y)
                    {
                        dstPtr[this->GetIndex(-1, y, z)] = srcPtr[source->GetIndex(source->width_ - 1, y, z)];
                        dstPtr[this->GetIndex(-2, y, z)] = srcPtr[source->GetIndex(source->width_ - 2, y, z)];
                    }
            }
        }
    }
}
