#pragma once

#include "../Core/Object.h"
#include "../Resource/Resource.h"

namespace Urho3D 
{

class URHO3D_API VoxelColorPalette : public Resource {
    OBJECT(VoxelColorPalette);

public:
    VoxelColorPalette(Context* context) : Resource(context) { }
    ~VoxelColorPalette() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

private:
    /// Palette color data.
    VariantVector colors_;

public:
    /// Gets the colors.
    const VariantVector& GetColors() { return colors_; }

    /// Sets the colors.
    void SetColors(const VariantVector colors) { colors_ = colors; }

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);
};

}
