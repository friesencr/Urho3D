#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE  1

//#include <STB/stb_voxel_render.h>

#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#include "VoxelDefs.h"
#include "VoxelBuilder.h"
#include "STBMeshBuilder.h"

namespace Urho3D
{

static const float URHO3D_RSQRT2 = 0.7071067811865f;
static const float URHO3D_RSQRT3 = 0.5773502691896f;

static float URHO3D_default_normals[32][3] =
{
	{ 1, 0, 0 },  // east
	{ 0, 1, 0 },  // north
	{ -1, 0, 0 }, // west
	{ 0, -1, 0 }, // south
	{ 0, 0, 1 },  // up
	{ 0, 0, -1 }, // down
	{ URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // east & up
	{ URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // east & down

	{ URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // east & up
	{ 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
	{ -URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // west & up
	{ 0, -URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
	{ URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // ne & up
	{ URHO3D_RSQRT3, URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // ne & down
	{ 0, URHO3D_RSQRT2, URHO3D_RSQRT2 }, // north & up
	{ 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down

	{ URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // east & down
	{ 0, URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // north & down
	{ -URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // west & down
	{ 0, -URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NW & up
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // NW & down
	{ -URHO3D_RSQRT2, 0, URHO3D_RSQRT2 }, // west & up
	{ -URHO3D_RSQRT2, 0, -URHO3D_RSQRT2 }, // west & down

	{ URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NE & up crossed
	{ -URHO3D_RSQRT3, URHO3D_RSQRT3, URHO3D_RSQRT3 }, // NW & up crossed
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SW & up crossed
	{ URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SE & up crossed
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, URHO3D_RSQRT3 }, // SW & up
	{ -URHO3D_RSQRT3, -URHO3D_RSQRT3, -URHO3D_RSQRT3 }, // SW & up
	{ 0, -URHO3D_RSQRT2, URHO3D_RSQRT2 }, // south & up
	{ 0, -URHO3D_RSQRT2, -URHO3D_RSQRT2 }, // south & down
};

static float URHO3D_default_ambient[3][4] =
{
	{ 0.0f, 1.0f, 0.0f, 0.0f }, // reversed lighting direction
	{ 0.1f, 0.1f, 0.1f, 0.0f }, // directional color
	{ 0.0f, 0.0f, 0.0f, 0.0f }, // constant color
};

static float URHO3D_default_texscale[128][4] =
{
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
	{ 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 1, 1, 0, 0 },
};

static float URHO3D_default_texgen[64][3] =
{
	{ 0, 1, 0 }, { 0, 0, 1 }, { 0, -1, 0 }, { 0, 0, -1 },
	{ -1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 0, -1 },
	{ 0, -1, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 0, -1 },
	{ 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 0, -1 },

	{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 },
	{ -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 },
	{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 },
	{ -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 },

	{ 0, 0, -1 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, -1, 0 },
	{ 0, 0, -1 }, { -1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 },
	{ 0, 0, -1 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 1, 0 },
	{ 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 },

	{ 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
	{ 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 },
	{ 0, -1, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
	{ 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 }, { 1, 0, 0 },
};


STBMeshBuilder::STBMeshBuilder(Context* context) : VoxelMeshBuilder(context),
    sharedIndexBuffer_(0)
{
    transform_.Resize(3);
    normals_.Resize(32);
    ambientTable_.Resize(3);
    texscaleTable_.Resize(64);
    texgenTable_.Resize(64);

    // copy transforms
    transform_[0] = Vector3(1.0, 0.5f, 1.0);
    transform_[1] = Vector3(0.0, 0.0, 0.0);
    transform_[2] = Vector3((float)(0 & 255), (float)(0 & 255), (float)(0 & 255));

    // copy normal table
    for (unsigned i = 0; i < 32; ++i)
        normals_[i] = Vector3(URHO3D_default_normals[i]);

    // ambient color table
    for (unsigned i = 0; i < 3; ++i)
        ambientTable_[i] = Vector3(URHO3D_default_ambient[i]);

    // texscale table
    for (unsigned i = 0; i < 64; ++i)
        texscaleTable_[i] = Vector4(URHO3D_default_texscale[i]);

    // texgen table
    for (unsigned i = 0; i < 64; ++i)
        texgenTable_[i] = Vector3(URHO3D_default_texgen[i]);

    workBuffers_.Resize(16);
    for (unsigned i = 0; i < workBuffers_.Size(); ++i)
        stbvox_init_mesh_maker(&workBuffers_[i].meshMaker);
}

unsigned STBMeshBuilder::VoxelDataCompatibilityMask() const {
    return 0xffffffff;
}

bool STBMeshBuilder::BuildMesh(VoxelBuildSlot* slot) 
{
    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    SharedPtr<VoxelMap> voxelMap(job->voxelMap);
    VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;
    STBWorkBuffer* workBuffer = (STBWorkBuffer*)&workBuffers_[slot->index];

    stbvox_mesh_maker* mm = &workBuffer->meshMaker;
    stbvox_set_input_stride(mm, voxelMap->GetStrideX(), voxelMap->GetStrideZ());

    stbvox_input_description *stbvox_map;
    stbvox_map = stbvox_get_input_description(mm);

    if (voxelBlocktypeMap)
    {
        stbvox_map->block_tex1 = voxelBlocktypeMap->blockTex1.Empty() ? 0 : &voxelBlocktypeMap->blockTex1.Front();
        stbvox_map->block_tex1_face = voxelBlocktypeMap->blockTex1Face.Empty() ? 0 : &voxelBlocktypeMap->blockTex1Face.Front();
        stbvox_map->block_tex2 = voxelBlocktypeMap->blockTex2.Empty() ? 0 : &voxelBlocktypeMap->blockTex2.Front();
        stbvox_map->block_tex2_face = voxelBlocktypeMap->blockTex2Face.Empty() ? 0 : &voxelBlocktypeMap->blockTex2Face.Front();
        stbvox_map->block_geometry = voxelBlocktypeMap->blockGeometry.Empty() ? 0 : &voxelBlocktypeMap->blockGeometry.Front();
        stbvox_map->block_vheight = voxelBlocktypeMap->blockVHeight.Empty() ? 0 : &voxelBlocktypeMap->blockVHeight.Front();
        stbvox_map->block_color = voxelBlocktypeMap->blockColor.Empty() ? 0 : &voxelBlocktypeMap->blockColor.Front();
    }

    PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
    GetVoxelDataDatas(voxelMap, datas);

    unsigned char** stb_data[VOXEL_DATA_NUM_BASIC_STREAMS] = {
        &stbvox_map->blocktype,
        &stbvox_map->color2,
        &stbvox_map->color2_facemask,
        &stbvox_map->color3,
        &stbvox_map->color3_facemask,
        &stbvox_map->color,
        &stbvox_map->ecolor_color,
        &stbvox_map->ecolor_facemask,
        &stbvox_map->extended_color,
        &stbvox_map->geometry,
        &stbvox_map->lighting,
        &stbvox_map->overlay,
        &stbvox_map->rotate,
        &stbvox_map->tex2,
        &stbvox_map->tex2_facemask,
        &stbvox_map->tex2_replace,
        &stbvox_map->vheight
    };

    // Set voxel maps for stb voxel
    int zero = voxelMap->GetIndex(0, 0, 0);
    for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        *stb_data[i] = ((1 << i) & voxelMap->GetDataMask()) ? &datas[i]->At(zero) : 0;

#if 1
    if (chunk->GetVoxelProcessors().Size() > 0 && chunk->GetProcessorDataMask())
    {
        PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
        GetVoxelDataDatas(&slot->writer, writerDatas);
        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (((1 << i) & chunk->GetProcessorDataMask()))
                *stb_data[i] = &writerDatas[i]->At(zero);
        }
    }
#endif

    stbvox_reset_buffers(mm);
    stbvox_set_buffer(mm, 0, 0, workBuffer->workVertexBuffer, VOXEL_STB_WORKER_VERTEX_BUFFER_SIZE);
    stbvox_set_buffer(mm, 0, 1, workBuffer->workFaceBuffer, VOXEL_STB_WORKER_FACE_BUFFER_SIZE);

    stbvox_set_default_mesh(mm, 0);

    bool success = true;
    for (unsigned y = 0; y < VOXEL_CHUNK_SIZE_Y; y += 16)
    {
        stbvox_set_input_range(
            mm,
            0,
            0,
            y,
            VOXEL_CHUNK_SIZE_X,
            VOXEL_CHUNK_SIZE_Z,
            y + 16
        );

        if (stbvox_make_mesh(mm) == 0)
        {
            // TODO handle partial uploads
            LOGERROR("Voxel mesh builder ran out of memory.");
            success = false;
            break;
        }
    }
    if (success)
        workBuffer->fragmentQuads = stbvox_get_quad_count(mm, 0);
    return success;
}

bool STBMeshBuilder::ProcessMesh(VoxelBuildSlot* slot)
{
    STBWorkBuffer* workBuffer = (STBWorkBuffer*)&workBuffers_[slot->index];
    VoxelChunk* chunk = slot->job->chunk;
    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    BoundingBox box;
    unsigned numVertices = workBuffer->fragmentQuads * 4;
    unsigned* verticies = (unsigned*)workBuffer->workVertexBuffer;
    for (int i = 0; i < numVertices; ++i)
    {
        unsigned v1 = verticies[i];
        Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0f, (float)((v1 >> 7u) & 127u));
        box.Merge(position);
    }
    mesh.numTriangles_ = workBuffer->fragmentQuads * 2;
    chunk->SetBoundingBox(box);

    VoxelJob* job = slot->job;
    Geometry* geometry = chunk->GetGeometry(0);

    // sets 
    unsigned vertexBufferSize = workBuffer->fragmentQuads * 4 * sizeof(unsigned);
    job->vertexBuffer = new unsigned char[vertexBufferSize];
    memcpy(job->vertexBuffer.Get(), workBuffer->workVertexBuffer, vertexBufferSize);
    geometry->SetRawVertexData(job->vertexBuffer, numVertices, MASK_DATA);
    
    // face data not index buffer
    unsigned indexBufferSize = workBuffer->fragmentQuads * sizeof(unsigned);
    job->indexBuffer = new  unsigned char[workBuffer->fragmentQuads * sizeof(unsigned)];
    memcpy(job->indexBuffer.Get(), workBuffer->workFaceBuffer, indexBufferSize);

    return true;
}

bool STBMeshBuilder::UploadGpuData(VoxelJob* job)
{
    PROFILE(UploadGpuData);

    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    unsigned numQuads = mesh.numTriangles_ / 2;
    if (numQuads == 0)
        return true;

    VertexBuffer* vb = mesh.vertexBuffer_;
    IndexBuffer* faceData = mesh.indexBuffer_;

    if (!vb->SetSize(numQuads * 4, MASK_DATA, false))
    {
        LOGERROR("Error allocating voxel vertex data.");
        return false;
    }

    if (!faceData->SetSize(numQuads, true, false))
    {
        LOGERROR("Error allocating voxel face data.");
        return false;
    }

    if (!vb->SetData(job->vertexBuffer.Get()))
    {
        LOGERROR("Error uploading voxel vertex data.");
        return false;
    }

    if (!faceData->SetData(job->indexBuffer.Get()))
    {
        LOGERROR("Error uploading voxel face data.");
        return false;
    }

    // face data
    Material* material = mesh.material_;
    {
        TextureBuffer* faceDataTexture = new TextureBuffer(context_);
        if (!faceDataTexture->SetSize(0))
        {
            LOGERROR("Error initializing voxel texture buffer");
            return false;
        }
        if (!faceDataTexture->SetData(faceData))
        {
            LOGERROR("Error setting voxel texture buffer data");
            return false;
        }
        material->SetTexture(TU_CUSTOM1, faceDataTexture);
    }

    // set shared quad index buffer
    if (!ResizeIndexBuffer(numQuads))
    {
        LOGERROR("Error resizing shared voxel index buffer");
        return false;
    }

    Geometry* geo = mesh.geometry_;
    geo->SetIndexBuffer(sharedIndexBuffer_);
    geo->SetVertexBuffer(0, vb, MASK_DATA);
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, numQuads * 6, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }
    return true;
}

bool STBMeshBuilder::UpdateMaterialParameters(Material* material)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Technique* technique = cache->GetResource<Technique>("Techniques/VoxelDiff.xml");
    material->SetTechnique(0, technique);
    material->SetShaderParameter("Transform", transform_);
    material->SetShaderParameter("NormalTable", normals_);
    material->SetShaderParameter("AmbientTable", ambientTable_);
    material->SetShaderParameter("TexScale", texscaleTable_);
    material->SetShaderParameter("TexGen", texgenTable_);
    return true;
}

bool STBMeshBuilder::ResizeIndexBuffer(unsigned numQuads)
{
    if (!(sharedIndexBuffer_.Null() || sharedIndexBuffer_->GetIndexCount() / 6 < numQuads))
        return true;

    unsigned numIndices = numQuads * 6;
    unsigned numVertices = numQuads * 4;

    unsigned indexSize = sizeof(int); // numVertices > M_MAX_UNSIGNED_SHORT ? sizeof(int) : sizeof(short);
    //SharedArrayPtr<unsigned char> data(new unsigned char[indexSize * (unsigned)(numIndices * 1.1)]); // add padding to reduce number of resizes
    SharedArrayPtr<unsigned char> data(new unsigned char[indexSize * 1000000]); // add padding to reduce number of resizes
    unsigned char* dataPtr = data.Get();

    // triangulates the quads
    for (unsigned i = 0; i < numQuads; i++)
    {
        *((unsigned*)dataPtr) = i * 4;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 1;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 2;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 2;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4 + 3;
        dataPtr += indexSize;
        *((unsigned*)dataPtr) = i * 4;
        dataPtr += indexSize;
    }

    if (sharedIndexBuffer_.Null())
    {
        sharedIndexBuffer_ = new IndexBuffer(context_);
        sharedIndexBuffer_->SetShadowed(true);
    }

    if (!sharedIndexBuffer_->SetSize(numIndices, indexSize == sizeof(unsigned), false))
    {
        LOGERROR("Could not set size of shared voxel index buffer.");
        return false;
    }

    if (!sharedIndexBuffer_->SetData(data.Get()))
    {
        LOGERROR("Could not set data of shared voxel index buffer.");
        return false;
    }
    return true;
}

void STBMeshBuilder::AssignWork(VoxelBuildSlot* slot)
{
    STBWorkBuffer* workBuffer = &workBuffers_[slot->index];
    workBuffer->fragmentQuads = 0;
}

void STBMeshBuilder::FreeWork(VoxelBuildSlot* slot)
{

}

}
