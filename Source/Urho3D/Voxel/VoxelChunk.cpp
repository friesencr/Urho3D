#include "../Scene/Node.h"
#include "../Graphics/Camera.h"

#include "VoxelChunk.h"

#include "../DebugNew.h"

namespace Urho3D {

VoxelChunk::VoxelChunk(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY),
    voxelJob_(0),
    voxelMap_(0),
    processorDataMask_(0)
{
    index_[0] = 0; index_[1] = 0; index_[2] = 0;
}

VoxelChunk::~VoxelChunk()
{
    //SetNumberOfMeshes(0);
    //if (voxelJob_)
    //{
    //    GetSubsystem<VoxelBuilder>()->CancelJob(voxelJob_);
    //    voxelJob_ = 0;
    //}
}

void VoxelChunk::RegisterObject(Context* context)
{
    context->RegisterFactory<VoxelChunk>("Voxel");
}

Geometry* VoxelChunk::GetGeometry(unsigned index) const
{
    return voxelMeshes_[index].geometry_;
}

UpdateGeometryType VoxelChunk::GetUpdateGeometryType()
{
    return UPDATE_MAIN_THREAD;
}

//bool VoxelChunk::Build(VoxelMap* voxelMap)
//{
//    if (!voxelMap)
//        return false;
//
//    VoxelBuilder* voxelBuilder = GetSubsystem<VoxelBuilder>();
//    if (voxelJob_)
//    {
//        voxelBuilder->CancelJob(voxelJob_);
//    }
//    voxelJob_ = voxelBuilder->BuildVoxelChunk(this, voxelMap, false);
//    return true;
//}
SharedPtr<VoxelMap> VoxelChunk::GetVoxelMap(bool filled)
{
    if (voxelMap_)
        return voxelMap_;
    else if (voxelSet_)
    {
        VoxelStore* voxelStore = voxelSet_->GetVoxelStore();
        if (voxelStore)
            return voxelStore->GetVoxelMap(index_[0], index_[1], index_[2], filled);
    }
    return SharedPtr<VoxelMap>();
}

bool VoxelChunk::FillVoxelMap(VoxelMap* voxelMap)
{
    if (voxelMap_)
    {
        voxelMap = voxelMap_;
        return voxelMap_->IsFilled();
    }
    else if (voxelSet_)
    {
        VoxelStore* voxelStore = voxelSet_->GetVoxelStore();
        if (voxelStore)
            return voxelStore->FillVoxelMap(voxelMap, index_[0], index_[1], index_[2]);
    }
    return false;
}

unsigned VoxelChunk::GetNumOccluderTriangles()
{
    unsigned numTriangles = 0;
    for (unsigned i = 0; i < batches_.Size(); ++i)
    {
        // Check that the material is suitable for occlusion (default material always is)
        Material* mat = batches_[i].material_;
        if (mat && !mat->GetOcclusion())
            continue;
        else
            numTriangles += batches_[i].geometry_->GetIndexCount() / 3;
    }
    return numTriangles;
}

void VoxelChunk::UpdateBatches(const FrameInfo& frame)
{
    for (unsigned i = 0; i < GetNumMeshes(); ++i)
    {
        const Matrix3x4& worldTransform = node_->GetWorldTransform();
        distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

        float scale = worldTransform.Scale().DotProduct(DOT_SCALE);
        lodDistance_ = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

        batches_[i].distance_ = distance_;
        batches_[i].worldTransform_ = &worldTransform;
    }
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

    //for (unsigned i = 0; i < GetNumMeshes(); ++i)
    //{
    //    if (updateMaterialParameters_[i])
    //        UpdateMaterialParameters(i);
    //}
}

void VoxelChunk::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void VoxelChunk::SetBoundingBox(const BoundingBox& box)
{
    boundingBox_ = box;
    OnMarkedDirty(node_);
}

void VoxelChunk::ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results)
{
//RayQueryLevel level = query.level_;
//
//switch (level)
//{
//case RAY_AABB:
//    Drawable::ProcessRayQuery(query, results);
//    break;
//    
//case RAY_OBB:
//case RAY_TRIANGLE:
//    Matrix3x4 inverse(node_->GetWorldTransform().Inverse());
//    Ray localRay = query.ray_.Transformed(inverse);
//    float distance = localRay.HitDistance(boundingBox_);
//    Vector3 normal = -query.ray_.direction_;
//    
//    if (level == RAY_TRIANGLE && distance < query.maxDistance_)
//    {
//        Vector3 geometryNormal;
//        distance = geometry_->GetHitDistance(localRay, &geometryNormal);
//        normal = (node_->GetWorldTransform() * Vector4(geometryNormal, 0.0f)).Normalized();
//    }
//    
//    if (distance < query.maxDistance_)
//    {
//        RayQueryResult result;
//        result.position_ = query.ray_.origin_ + distance * query.ray_.direction_;
//        result.normal_ = normal;
//        result.distance_ = distance;
//        result.drawable_ = this;
//        result.node_ = node_;
//        result.subObject_ = M_MAX_UNSIGNED;
//        results.Push(result);
//    }
//    break;
//}
}

//bool VoxelChunk::GetHasShaderParameters(unsigned index)
//{
//    return hasMaterialParameters_[index];
//}

//void VoxelChunk::SetHasShaderParameters(unsigned index, bool isRequired)
//{
//    hasMaterialParameters_[index] = isRequired;
//}

void VoxelChunk::SetNumberOfMeshes(unsigned count)
{
    if (count > 2)
        return;

    if (count == voxelMeshes_.Size())
        return;

    unsigned oldCount = voxelMeshes_.Size();
    voxelMeshes_.Resize(count);
    batches_.Resize(count);

    for (unsigned i = oldCount; i < count; ++i)
    {
        VoxelMesh& mesh = voxelMeshes_[i];

        mesh.dirtyShaderParameters_ = false;
        mesh.material_ = new Material(context_);
        mesh.geometry_ = new Geometry(context_);
        mesh.vertexBuffer_ = new VertexBuffer(context_);
        mesh.indexBuffer_ = new IndexBuffer(context_);

        batches_[i].material_ = mesh.material_;
        batches_[i].geometry_ = mesh.geometry_;
        batches_[i].geometryType_ = GEOM_STATIC_NOINSTANCING;
    }
}

void VoxelChunk::SetMaterial(unsigned slot, Material* material)
{
    voxelMeshes_[slot].material_ = material;
    batches_[slot].material_ = material;
}

Material* VoxelChunk::GetMaterial(unsigned selector) const
{
    if (batches_.Size() > selector)
        return batches_[selector].material_;
    else
        return 0;
}

void VoxelChunk::SetIndex(unsigned char x, unsigned char y, unsigned char z)
{
    index_[0] = x;
    index_[1] = y;
    index_[2] = z;
}

unsigned char VoxelChunk::GetIndexX() { return index_[0]; }
unsigned char VoxelChunk::GetIndexY() { return index_[1]; }
unsigned char VoxelChunk::GetIndexZ() { return index_[2]; }

unsigned VoxelChunk::GetTotalTriangles() const
{
    unsigned total = 0;
    for (unsigned i = 0; i < GetNumMeshes(); ++i)
        total += GetNumTriangles(i);
    return total;
}

bool VoxelChunk::DrawOcclusion(OcclusionBuffer* buffer)
{
#if 0
    Matrix4 viewProj = buffer->GetProjection() * buffer->GetView();
    Matrix4 modelViewProj = viewProj * GetNode()->GetWorldTransform();

    for (unsigned i = 0; i < batches_.Size(); ++i)
    {
        Material* material = batches_[i].material_;
        if (material)
        {
            if (!material->GetOcclusion())
                return true;
            buffer->SetCullMode(material->GetCullMode());
        }
        else
            buffer->SetCullMode(CULL_CCW);


        Geometry* geometry = geometries_[i];

        const unsigned char* vertexData;
        unsigned vertexSize;
        const unsigned char* indexData;
        unsigned indexSize;
        unsigned elementMask;
        geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elementMask);

        // Check for valid geometry data
        if (!vertexData || !indexData)
            continue;

        unsigned indexStart = geometry->GetIndexStart();
        unsigned indexCount = reducedQuadCount_[i] * 6;

        // Draw and check for running out of triangles
        if (!buffer->Draw(node_->GetWorldTransform(), vertexData, vertexSize, indexData, indexSize, indexStart, indexCount))
            return false;
    }
#endif
    return true;
}

Geometry* VoxelChunk::GetLodGeometry(unsigned batchIndex, unsigned level)
{
    return batches_[batchIndex].geometry_;
}

void VoxelChunk::UpdateMaterialParameters(unsigned slot)
{
    Material* material = GetMaterial(slot);
    if (!material)
        return;

    if (meshBuilder_)
        meshBuilder_->UpdateMaterialParameters(material);

    if (textureMap_)
    {
        Texture* diffuse = textureMap_->GetDiffuseTexture();
        if (diffuse)
            material->SetTexture(TU_DIFFUSE, diffuse);
    }

    if (colorPalette_)
        material->SetShaderParameter("ColorTable", colorPalette_->GetColors());

    voxelMeshes_[slot].dirtyShaderParameters_ = false;
}

void VoxelChunk::OnVoxelChunkCreated()
{
    if (!voxelJob_)
        return;

    UpdateMaterialParameters(0);
    voxelJob_ = 0;
    voxelMeshes_[0].dirtyShaderParameters_ = true;
    if (!node_)
        return;

    if (!voxelMeshes_[0].numTriangles_)
        node_->SetEnabled(false);

    using namespace VoxelChunkCreated;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_NODE] = node_;
    node_->SendEvent(E_VOXELCHUNKCREATED, eventData);
}

const PODVector<StringHash>& VoxelChunk::GetVoxelProcessors()
{
    return voxelProcessors_;
}

void VoxelChunk::SetVoxelProcessors(PODVector<StringHash>& voxelProcessors)
{
    voxelProcessors_ = voxelProcessors;
}

void VoxelChunk::AddVoxelProcessor(StringHash voxelProcessorHash)
{
    voxelProcessors_.Push(voxelProcessorHash);
}

void VoxelChunk::RemoveVoxelProcessor(const StringHash& voxelProcessorHash)
{
    voxelProcessors_.Remove(voxelProcessorHash);
}

}
