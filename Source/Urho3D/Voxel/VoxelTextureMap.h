#pragma once

#include "../Core/Object.h"
#include "../Resource/Resource.h"
#include "../Graphics/Texture.h"

namespace Urho3D
{

class URHO3D_API VoxelTextureMap : public Resource {
    OBJECT(VoxelTextureMap);

public:
    VoxelTextureMap(Context* context) : Resource(context) { }
    ~VoxelTextureMap() {}

    /// Register object factory.
    static void RegisterObject(Context* context);

    Texture* GetDiffuse1Texture() const { return diffuse1Texture; }

    Texture* GetDiffuse2Texture() const { return diffuse2Texture; }

    void SetDiffuse1Texture(Texture* texture);

    void SetDiffuse2Texture(Texture* texture);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);

private:
    SharedPtr<Texture> diffuse1Texture;
    SharedPtr<Texture> diffuse2Texture;
    PODVector<unsigned char> defaultTexture2Map;
    void SetDiffuse1TextureArrayAttr(const ResourceRef& value);
    void SetDiffuse2TextureArrayAttr(const ResourceRef& value);
};

}
