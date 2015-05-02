#include "VoxelChunk.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY = "Geometry";

VoxelChunk::VoxelChunk(Context* context) :
    Drawable(context, DRAWABLE_ANY),
    geometry_(new Geometry(context)),
    vertexBuffer_(new VertexBuffer(context))
{
    geometry_->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1 );
	geometry_->SetIndexBuffer(sharedIndexBuffer);
}

void VoxelChunk::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelChunk>(GEOMETRY_CATEGORY);
}

Geometry* VoxelChunk::GetGeometry() const
{
    return geometry_;
}

VertexBuffer* VoxelChunk::GetVertexBuffer() const
{
    return vertexBuffer_;
}

void VoxelChunk::OnWorldBoundingBoxUpdate()
{
    //worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void VoxelChunk::SetSize(int x, int y, int z)
{
	sizeX_ = x;
	sizeY_ = y;
	sizeZ_ = z;
	InitializeBuffer();
}

void VoxelChunk::InitializeBuffer()
{

}

}

