#include "../Core/Context.h"
#include "../Resource/ResourceCache.h"

#include "VoxelTextureMap.h"

#include "../DebugNew.h"

namespace Urho3D
{

void VoxelTextureMap::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelTextureMap>("Voxel");
}

bool VoxelTextureMap::BeginLoad(Deserializer& source)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    ResourceRef diffuseRef = source.ReadResourceRef();
    this->defaultTextureMap = source.ReadBuffer();
    SetDiffuseTexture(cache->GetResource<Texture>(diffuseRef.name_));
    return true;
}

void VoxelTextureMap::SetDiffuse1TextureArrayAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture* texture = cache->GetResource<Texture>(value.name_);
    SetDiffuseTexture(texture);
}

void VoxelTextureMap::SetDiffuseTexture(Texture* texture)
{ 
    diffuseTexture = texture;
}

};
