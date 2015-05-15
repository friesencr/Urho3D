#include "VoxelSet.h"
#include "Drawable.h"
#include "../Core/Object.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Core/WorkQueue.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY;

VoxelSet::VoxelSet(Context* context) :
Component(context)
{
}

VoxelSet::~VoxelSet()
{

}

void VoxelSet::RegisterObject(Context* context)
{
	context->RegisterFactory<VoxelSet>(GEOMETRY_CATEGORY);
}

void VoxelSet::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
	Serializable::OnSetAttribute(attr, src);

	// Change of any non-accessor attribute requires recreation of the terrain
	//if (!attr.accessor_)
	//    recreateTerrain_ = true;
}

void VoxelSet::ApplyAttributes()
{
	//if (recreateTerrain_)
	//    CreateGeometry();
}

void VoxelSet::OnSetEnabled()
{
	using namespace SceneUpdate;
	Scene* scene = GetScene();
	if (IsEnabled())
		SubscribeToEvent(scene, E_SCENEUPDATE, HANDLER(VoxelSet, HandleSceneUpdate));
	else
		UnsubscribeFromEvent(scene, E_SCENEUPDATE);
}

void VoxelSet::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{

}


void VoxelSet::SetDimensions(unsigned width, unsigned height, unsigned depth)
{
	height_ = height;
	width_ = width;
	depth_ = depth;
}

}
