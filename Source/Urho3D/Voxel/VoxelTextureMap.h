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

    Texture* GetDiffuseTexture() const { return diffuseTexture; }

    void SetDiffuseTexture(Texture* texture);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    /// Saves voxel map information.
    //virtual bool Save(Serializer& dest);

private:
    SharedPtr<Texture> diffuseTexture;
    PODVector<unsigned char> defaultTextureMap;
    void SetDiffuse1TextureArrayAttr(const ResourceRef& value);
    void SetDiffuse2TextureArrayAttr(const ResourceRef& value);
};

}
