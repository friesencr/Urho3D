#include "VoxelChunk.h"
#include "Drawable.h"
#include "Geometry.h"
#include "OctreeQuery.h"
#include "Scene/Node.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/Camera.h"
#include "DebugRenderer.h"
#include "../../../../../.clion10/system/cmake/generated/3940ee08/3940ee08/Debug/include/Urho3D/Graphics/GraphicsDefs.h"

namespace Urho3D {

extern const char* GEOMETRY_CATEGORY;

VoxelChunk::VoxelChunk(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY),
    geometry_(new Geometry(context)),
    vertexBuffer_(new VertexBuffer(context)),
    sizeX_(0),
    sizeY_(0),
    sizeZ_(0),
    indexX_(0),
    indexY_(0),
    indexZ_(0),
    lodLevel_(0)
{
    geometry_->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1);
    batches_.Resize(1);
    batches_[0].geometry_ = geometry_;
    batches_[0].geometryType_ = GEOM_STATIC_NOINSTANCING;
}

VoxelChunk::~VoxelChunk()
{

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

UpdateGeometryType VoxelChunk::GetUpdateGeometryType()
{
    return UPDATE_MAIN_THREAD;
}

unsigned VoxelChunk::GetNumOccluderTriangles()
{
    return 0;
    /* if (geometry_->GetIndexBuffer()) */
    /* { */
	/* geometry_->GetIndexBuffer()->GetIndexCount() / 3 : 0; */
    /* } */
}

void VoxelChunk::UpdateBatches(const FrameInfo& frame)
{
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());
    
    float scale = worldTransform.Scale().DotProduct(DOT_SCALE);
    lodDistance_ = frame.camera_->GetLodDistance(distance_, scale, lodBias_);
    
    batches_[0].distance_ = distance_;
    batches_[0].worldTransform_ = &worldTransform;
}

void VoxelChunk::UpdateGeometry(const FrameInfo& frame)
{
    /* if (vertexBuffer_->IsDataLost()) */
    /* { */
    /*     if (owner_) */
    /*         owner_->BuildChunk(this); */
    /*     else */
    /*         vertexBuffer_->ClearDataLost(); */
    /* } */
    
    /* if (owner_) */
    /*     owner_->UpdatePatchLod(this); */
}

void VoxelChunk::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void VoxelChunk::SetOwner(VoxelSet* voxelSet)
{
	owner_ = voxelSet;
}

void VoxelChunk::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
    OnMarkedDirty(node_);
}

void VoxelChunk::ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results)
{
    RayQueryLevel level = query.level_;
    
    switch (level)
    {
    case RAY_AABB:
        Drawable::ProcessRayQuery(query, results);
        break;
        
    case RAY_OBB:
    case RAY_TRIANGLE:
        Matrix3x4 inverse(node_->GetWorldTransform().Inverse());
        Ray localRay = query.ray_.Transformed(inverse);
        float distance = localRay.HitDistance(boundingBox_);
        Vector3 normal = -query.ray_.direction_;
        
        if (level == RAY_TRIANGLE && distance < query.maxDistance_)
        {
            Vector3 geometryNormal;
            distance = geometry_->GetHitDistance(localRay, &geometryNormal);
            normal = (node_->GetWorldTransform() * Vector4(geometryNormal, 0.0f)).Normalized();
        }
        
        if (distance < query.maxDistance_)
        {
            RayQueryResult result;
            result.position_ = query.ray_.origin_ + distance * query.ray_.direction_;
            result.normal_ = normal;
            result.distance_ = distance;
            result.drawable_ = this;
            result.node_ = node_;
            result.subObject_ = M_MAX_UNSIGNED;
            results.Push(result);
        }
        break;
    }
}


void VoxelChunk::SetMaterial(Material* material)
{
    batches_[0].material_ = material;
}

void VoxelChunk::SetIndex(int x, int y, int z)
{
	indexX_ = x;
	indexY_ = y;
	indexZ_ = z;
}

void VoxelChunk::SetSize(int x, int y, int z)
{
	sizeX_ = x;
	sizeY_ = y;
	sizeZ_ = z;
}

int VoxelChunk::GetSizeX() const { return sizeX_; }

int VoxelChunk::GetSizeY() const { return sizeY_; }

int VoxelChunk::GetSizeZ() const { return sizeZ_; }

int VoxelChunk::GetIndexX() const { return indexX_; }

int VoxelChunk::GetIndexY() const { return indexY_; }

int VoxelChunk::GetIndexZ() const { return indexZ_; }

unsigned VoxelChunk::GetLodLevel() const { return lodLevel_; }

bool VoxelChunk::DrawOcclusion(OcclusionBuffer* buffer)
{
    return true;
}

Geometry* VoxelChunk::GetLodGeometry(unsigned batchIndex, unsigned level)
{
    return geometry_;
}

}

