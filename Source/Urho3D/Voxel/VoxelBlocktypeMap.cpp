#include "VoxelBlocktypeMap.h"

#include "../DebugNew.h"

namespace Urho3D {

VoxelBlocktypeMap::VoxelBlocktypeMap(Context* context) : Resource(context)
{

}

VoxelBlocktypeMap::~VoxelBlocktypeMap()
{

}

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

};
