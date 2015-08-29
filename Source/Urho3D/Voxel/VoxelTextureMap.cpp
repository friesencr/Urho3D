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

};
