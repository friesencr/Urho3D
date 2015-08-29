#pragma once

#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Resource/Resource.h"
#include "../Container/Vector.h"
#include "../IO/Serializer.h"
#include "../IO/Deserializer.h"

namespace Urho3D 
{

class URHO3D_API VoxelBlocktypeMap : public Resource {
    OBJECT(VoxelBlocktypeMap);
    
public:
    VoxelBlocktypeMap(Context* context);
    ~VoxelBlocktypeMap();

    /// Register object factory.
    static void RegisterObject(Context* context);

    PODVector<unsigned char> blockColor;
    PODVector<unsigned char> blockGeometry;
    PODVector<unsigned char> blockVHeight;
    PODVector<unsigned char> blockTex1;
    PODVector<unsigned char> blockTex2;
    PODVector<unsigned char> blockSelector;
    PODVector<unsigned char> blockSideTextureRotation;
    PODVector<unsigned char[6]> blockColorFace;
    PODVector<unsigned char[6]> blockTex1Face;
    PODVector<unsigned char[6]> blockTex2Face;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);
};

}
