#pragma once

#include "../Core/Object.h"
#include "../Resource/Resource.h"

namespace Urho3D 
{

class URHO3D_API VoxelOverlayMap : public Resource {
    OBJECT(VoxelOverlayMap);

public:
    VoxelOverlayMap(Context* context) : Resource(context) { }
    ~VoxelOverlayMap() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

    PODVector<unsigned char[6]> overlayTex1;
    PODVector<unsigned char[6]> overlayTex2;
    PODVector<unsigned char[6]> overlayColor;
    PODVector<unsigned char> overlaySideTextureRotation;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);
};

}
