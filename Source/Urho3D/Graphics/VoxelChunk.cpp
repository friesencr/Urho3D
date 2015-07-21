#include "VoxelChunk.h"
#include "Drawable.h"
#include "Geometry.h"
#include "OctreeQuery.h"
#include "Scene/Node.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/Camera.h"
#include "DebugRenderer.h"
#include "VoxelBuilder.h"
#include "OcclusionBuffer.h"
#include "DrawableEvents.h"

#include "../DebugNew.h"

namespace Urho3D {

    extern const char* GEOMETRY_CATEGORY;

    VoxelChunk::VoxelChunk(Context* context) :
        Drawable(context, DRAWABLE_GEOMETRY),
        numMeshes_(0),
        buildPrioirty_(0.0),
        buildVisible_(false)
    {
        size_[0] = 0; size_[1] = 0; size_[2] = 0;
        index_[0] = 0; index_[1] = 0; index_[2] = 0;
        SetNumberOfMeshes(numMeshes_);
    }

    VoxelChunk::~VoxelChunk()
    {

    }

    void VoxelChunk::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelChunk>(GEOMETRY_CATEGORY);
    }

    Geometry* VoxelChunk::GetGeometry(unsigned index) const
    {
        return geometries_[index];
    }

    VertexBuffer* VoxelChunk::GetVertexBuffer(unsigned index) const
    {
        return vertexData_[index];
    }

    IndexBuffer* VoxelChunk::GetFaceData(unsigned index) const
    {
        return faceData_[index];
    }

    TextureBuffer* VoxelChunk::GetFaceBuffer(unsigned index) const
    {
        return faceBuffer_[index];
    }

    UpdateGeometryType VoxelChunk::GetUpdateGeometryType()
    {
        return UPDATE_MAIN_THREAD;
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

        for (unsigned i = 0; i < GetNumMeshes(); ++i)
        {
            if (updateMaterialParameters_[i])
                UpdateMaterialParameters(i);
        }
    }

    void VoxelChunk::OnWorldBoundingBoxUpdate()
    {
        worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
    }

    void VoxelChunk::SetVoxelSet(VoxelSet* voxelSet)
    {
        voxelSet_ = voxelSet;
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
        if (count > 64)
            return;

        if (count == numMeshes_)
            return;

        numMeshes_ = count;

        batches_.Resize(count);
        vertexData_.Resize(count);
        faceData_.Resize(count);
        faceBuffer_.Resize(count);
        geometries_.Resize(count);
        materials_.Resize(count);
        batches_.Resize(count);
        updateMaterialParameters_.Resize(count);
        numQuads_.Resize(count);
        reducedQuadCount_.Resize(count);

        for (unsigned i = 0; i < count; ++i)
        {
            updateMaterialParameters_[i] = false;
            vertexData_[i] = new VertexBuffer(context_);
            vertexData_[i]->SetShadowed(true);
            faceData_[i] = new IndexBuffer(context_);
            faceData_[i]->SetShadowed(true);
            faceBuffer_[i] = new TextureBuffer(context_);
            materials_[i] = new Material(context_);
            geometries_[i] = new Geometry(context_);
            geometries_[i]->SetVertexBuffer(0, vertexData_[i], MASK_DATA);
            batches_[i].material_ = materials_[i];
            batches_[i].geometry_ = geometries_[i];
            batches_[i].geometryType_ = GEOM_STATIC_NOINSTANCING;
            numQuads_[i] = 0;
        }
    }

    void VoxelChunk::SetMaterial(unsigned selector, Material* material)
    {
        batches_[selector].material_ = material;
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

    void VoxelChunk::SetSize(unsigned char x, unsigned char y, unsigned char z)
    {
        size_[0] = x;
        size_[1] = y;
        size_[2] = z;
    }

    unsigned char VoxelChunk::GetSizeX() { return size_[0]; }
    unsigned char VoxelChunk::GetSizeY() { return size_[1]; }
    unsigned char VoxelChunk::GetSizeZ() { return size_[2]; }
    unsigned char VoxelChunk::GetIndexX() { return index_[0]; }
    unsigned char VoxelChunk::GetIndexY() { return index_[1]; }
    unsigned char VoxelChunk::GetIndexZ() { return index_[2]; }

    unsigned VoxelChunk::GetTotalQuads() const
    {
        unsigned total = 0;
        for (unsigned i = 0; i < GetNumMeshes(); ++i)
            total += GetNumQuads(i);
        return total;
    }

    void VoxelChunk::SetNeighbors(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west)
    {
        neighborNorth_ = north;
        neighborSouth_ = south;
        neighborEast_ = east;
        neighborWest_ = west;
    }

    VoxelMap* VoxelChunk::GetNeighborNorth() const
    {
        return neighborNorth_;
    }

    VoxelMap* VoxelChunk::GetNeighborSouth() const
    {
        return neighborSouth_;
    }

    VoxelMap* VoxelChunk::GetNeighborEast() const
    {
        return neighborEast_;
    }

    VoxelMap* VoxelChunk::GetNeighborWest() const
    {
        return neighborWest_;
    }

    bool VoxelChunk::DrawOcclusion(OcclusionBuffer* buffer)
    {
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
        return true;
    }

    void VoxelChunk::UnloadMesh()
    {
        if (buildJob_)
        {
            GetSubsystem<VoxelBuilder>()->CancelJob(buildJob_);
            buildJob_ = 0;
        }
        SetNumberOfMeshes(0);
    }

    void VoxelChunk::UnloadMap()
    {
        if (buildJob_)
        {
            GetSubsystem<VoxelBuilder>()->CancelJob(buildJob_);
            buildJob_ = 0;
        }
        if (voxelMap_)
            voxelMap_->Unload();
    }

    void VoxelChunk::Unload()
    {
        if (buildJob_)
        {
            GetSubsystem<VoxelBuilder>()->CancelJob(buildJob_);
            buildJob_ = 0;
        }
        UnloadMap();
        UnloadMesh();
    }

    bool VoxelChunk::Reload(bool async)
    {
        //GetNode()->SetEnabled(true);
        //SetEnabled(true);
        return BuildInternal(async);
    }

    bool VoxelChunk::Build()
    {
        return BuildInternal(false);
    }

    bool VoxelChunk::BuildAsync()
    {
        return BuildInternal(true);
    }

    bool VoxelChunk::BuildInternal(bool async)
    {
        VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();
        SharedPtr<VoxelMap> voxelMap(GetVoxelMap());
        if (buildJob_)
            builder->CancelJob(buildJob_);

        if (voxelMap.Null())
            return false;

        if (!voxelMap->IsLoaded() && !voxelMap->Reload())
            return false;

        buildJob_ = builder->BuildVoxelChunk(SharedPtr<VoxelChunk>(this), async);
        return true;
    }

    Geometry* VoxelChunk::GetLodGeometry(unsigned batchIndex, unsigned level)
    {
        return batches_[batchIndex].geometry_;
    }

    VoxelMap* VoxelChunk::GetVoxelMap() const
    {
        return voxelMap_;
    }

    void VoxelChunk::SetVoxelMap(VoxelMap* voxelMap)
    {
        voxelMap_ = voxelMap;
    }

    void VoxelChunk::UpdateMaterialParameters(unsigned slot)
    {
        Material* material = GetMaterial(slot);
        if (!material)
            return;

        bool setColor = true;

        if (voxelMap_)
        {
            VoxelTextureMap* textureMap = voxelMap_->textureMap;
            if (textureMap)
            {
                Texture* diffuse1 = textureMap->GetDiffuse1Texture();
                if (diffuse1)
                    material->SetTexture(TU_DIFFUSE, diffuse1);

                Texture* diffuse2 = textureMap->GetDiffuse2Texture();
                if (diffuse2)
                    material->SetTexture(TU_NORMAL, diffuse2);

            }

            VoxelColorPalette* colorPalette = voxelMap_->colorPalette;
            if (colorPalette)
            {
                setColor = false;
                material->SetShaderParameter("ColorTable", colorPalette->GetColors());
            }
        }
        VoxelBuilder* voxelBuilder = GetSubsystem<VoxelBuilder>();
        voxelBuilder->UpdateMaterialParameters(material, true);

        updateMaterialParameters_[slot] = false;
    }

    void VoxelChunk::OnVoxelChunkCreated()
    {
        buildJob_ = 0;
        updateMaterialParameters_[0] = true;
        if (!node_)
            return;

		if (!keepVoxelData_ && !voxelSet_)
			voxelMap_->Unload();

        using namespace VoxelChunkCreated;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_NODE] = node_;
        node_->SendEvent(E_VOXELCHUNKCREATED, eventData);
    }

}
