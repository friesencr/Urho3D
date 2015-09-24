#if 0
#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE  1

//#include <STB/stb_voxel_render.h>

#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#include "VoxelDefs.h"
#include "VoxelBuilder.h"
#include "ClassicMeshBuilder.h"

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

static float URHO3D_default_palette[64][4];

static unsigned char URHO3D_default_palette_compact[64][3] =
{
	{ 255, 255, 255 }, { 238, 238, 238 }, { 221, 221, 221 }, { 204, 204, 204 },
	{ 187, 187, 187 }, { 170, 170, 170 }, { 153, 153, 153 }, { 136, 136, 136 },
	{ 119, 119, 119 }, { 102, 102, 102 }, { 85, 85, 85 }, { 68, 68, 68 },
	{ 51, 51, 51 }, { 34, 34, 34 }, { 17, 17, 17 }, { 0, 0, 0 },
	{ 255, 240, 240 }, { 255, 220, 220 }, { 255, 160, 160 }, { 255, 32, 32 },
	{ 200, 120, 160 }, { 200, 60, 150 }, { 220, 100, 130 }, { 255, 0, 128 },
	{ 240, 240, 255 }, { 220, 220, 255 }, { 160, 160, 255 }, { 32, 32, 255 },
	{ 120, 160, 200 }, { 60, 150, 200 }, { 100, 130, 220 }, { 0, 128, 255 },
	{ 240, 255, 240 }, { 220, 255, 220 }, { 160, 255, 160 }, { 32, 255, 32 },
	{ 160, 200, 120 }, { 150, 200, 60 }, { 130, 220, 100 }, { 128, 255, 0 },
	{ 255, 255, 240 }, { 255, 255, 220 }, { 220, 220, 180 }, { 255, 255, 32 },
	{ 200, 160, 120 }, { 200, 150, 60 }, { 220, 130, 100 }, { 255, 128, 0 },
	{ 255, 240, 255 }, { 255, 220, 255 }, { 220, 180, 220 }, { 255, 32, 255 },
	{ 160, 120, 200 }, { 150, 60, 200 }, { 130, 100, 220 }, { 128, 0, 255 },
	{ 240, 255, 255 }, { 220, 255, 255 }, { 180, 220, 220 }, { 32, 255, 255 },
	{ 120, 200, 160 }, { 60, 200, 150 }, { 100, 220, 130 }, { 0, 255, 128 },
};

ClassicMeshBuilder::ClassicMeshBuilder(Context* context) : VoxelMeshBuilder(context),
    sharedIndexBuffer_(0)
{
    transform_.Resize(3);
    normals_.Resize(32);
    ambientTable_.Resize(3);
    texscaleTable_.Resize(64);
    texgenTable_.Resize(64);
    defaultColorTable_.Resize(64);

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

    // color table
    for (int i = 0; i < 64; ++i) {
        URHO3D_default_palette[i][0] = URHO3D_default_palette_compact[i][0] / 255.0f;
        URHO3D_default_palette[i][1] = URHO3D_default_palette_compact[i][1] / 255.0f;
        URHO3D_default_palette[i][2] = URHO3D_default_palette_compact[i][2] / 255.0f;
        URHO3D_default_palette[i][3] = 1.0f;
    }

    for (unsigned i = 0; i < 64; ++i)
        defaultColorTable_[i] = Vector4(URHO3D_default_palette[i]);
}

unsigned ClassicMeshBuilder::VoxelDataCompatibilityMask() const {
    return 0xffffffff;
}

bool ClassicMeshBuilder::BuildMesh(VoxelWorkload* workload) 
{
    VoxelBuildSlot* slot = workload->slot;
    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    VoxelMap* voxelMap = job->voxelMap;
    VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;
    STBWorkBuffer* workBuffer = (STBWorkBuffer*)slot->workBuffer;

    stbvox_mesh_maker* mm = &workBuffer->meshMakers[workload->index];
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
    VoxelRangeFragment processRange = workload->range;
    if (voxelMap->GetVoxelProcessors().Size() > 0 && voxelMap->GetProcessorDataMask())
    {
        PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
        GetVoxelDataDatas(&slot->writer, writerDatas);
        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (((1 << i) & voxelMap->GetProcessorDataMask()))
                *stb_data[i] = &writerDatas[i]->At(zero);
        }
    }
#endif

    stbvox_reset_buffers(mm);
    stbvox_set_buffer(mm, 0, 0, workBuffer->workVertexBuffers[workload->index], VOXEL_WORKER_VERTEX_BUFFER_SIZE);
    stbvox_set_buffer(mm, 0, 1, workBuffer->workFaceBuffers[workload->index], VOXEL_WORKER_FACE_BUFFER_SIZE);

    stbvox_set_default_mesh(mm, 0);

    bool success = true;
    for (unsigned y = 0; y < workload->range.endY; y += 16)
    {
        stbvox_set_input_range(
                mm,
                workload->range.startX,
                workload->range.startZ,
                workload->range.startY + y,
                workload->range.endX,
                workload->range.endZ,
                Min(y + 16, workload->range.endY)
                );

        if (stbvox_make_mesh(mm) == 0)
        {
            // TODO handle partial uploads
            LOGERROR("Voxel mesh builder ran out of memory.");
            success = false;
            break;
        }
    }
    workBuffer->fragmentQuads[workload->index] = stbvox_get_quad_count(mm, 0);
    return success;
}

bool ClassicMeshBuilder::ProcessMeshFragment(VoxelWorkload* workload)
{
    STBWorkBuffer* workBuffer = (STBWorkBuffer*)workload->slot->workBuffer;
    VoxelBuildSlot* slot = workload->slot;
    VoxelChunk* chunk = slot->job->chunk;
    VoxelMesh& mesh = chunk->GetVoxelMesh(0);

    int numQuads = workBuffer->fragmentQuads[workload->index];
    unsigned workloadIndex = workload->index;
    unsigned numVertices = numQuads * 4; // 4 verts in a quad


geometry_->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1 | MASK_TANGENT);

VoxelWorkSlot* slot = workload->slot;
		int threadIndex = workload->threadIndex;
		int gpuVertexSize = sizeof(float) * 8; // position(3) / normal(3) / uv1(2)
		int cpuVertexSize = sizeof(float) * 3; // position(3)
		int numVertices = numQuads * 4; // 4 verts in a quad

		workload->gpuData.Resize(numVertices * gpuVertexSize);
		workload->cpuData.Resize(numVertices * cpuVertexSize);
		float* gpu = (float*)&workload->gpuData.Front();
		float* cpu = (float*)&workload->cpuData.Front();
		unsigned int* workData = (unsigned int*)slot->workBuffers[threadIndex];

        BoundingBox box;
		for (int i = 0; i < numVertices; ++i)
		{
			unsigned int v1 = *workData++;
			unsigned int v2 = *workData++;

			Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0, (float)((v1 >> 7u) & 127u));
			//float amb_occ = (float)((v1 >> 23u) & 63u) / 63.0;
			unsigned char tex1 = v2 & 0xFF;
			unsigned char tex2 = (v2 >> 8) & 0xFF;
			unsigned char color = (v2 >> 16) & 0xFF;
			unsigned char face_info = (v2 >> 24) & 0xFF;
			unsigned char normal = face_info >> 2u;
			//unsigned char face_rot = face_info & 2u;
			Vector3 normalf(URHO3D_default_normals[normal]);

            box.Merge(position);

			//LOGINFO(position.ToString());

			*gpu++ = position.x_;
			*gpu++ = position.y_;
			*gpu++ = position.z_;
			*cpu++ = position.x_;
			*cpu++ = position.y_;
			*cpu++ = position.z_;

			*gpu++ = normalf.x_;
			*gpu++ = normalf.y_;
			*gpu++ = normalf.z_;

			if (i % 4 == 0)
			{
				*gpu++ = 0.0;
				*gpu++ = 0.0;
			}
			else if (i % 4 == 1)
			{
				*gpu++ = 1.0;
				*gpu++ = 0.0;
			}
			else if (i % 4 == 2)
			{
				*gpu++ = 1.0;
				*gpu++ = 1.0;
			}
			else if (i % 4 == 3)
			{
				*gpu++ = 0.0;
				*gpu++ = 1.0;
			}
		}

    {
        BoundingBox box;
        // vertex data
        unsigned* sourceVertexData = (unsigned int*)workBuffer->workVertexBuffers[workloadIndex];
        mesh.rawVertexData_[workloadIndex].Resize(numVertices * sizeof(unsigned));
        unsigned* chunkVertexData = (unsigned*)&mesh.rawVertexData_[workloadIndex].Front();
        for (int i = 0; i < numVertices; ++i)
        {
            unsigned v1 = sourceVertexData[i];
            chunkVertexData[i] = v1;
            Vector3 position((float)(v1 & 127u), (float)((v1 >> 14u) & 511u) / 2.0f, (float)((v1 >> 7u) & 127u));
            box.Merge(position);
        }
        workBuffer->box[workload->index] = box;
    }

    {
        // face data
        unsigned* sourceFaceData = (unsigned*)workBuffer->workFaceBuffers[workloadIndex];
        mesh.rawFaceData_[workloadIndex].Resize(numQuads * sizeof(unsigned));
        unsigned* chunkFaceData = (unsigned*)&mesh.rawFaceData_[workloadIndex].Front();
        for (int i = 0; i < numQuads; ++i)
            chunkFaceData[i] = sourceFaceData[i];
    }
    return true;
}

bool ClassicMeshBuilder::ProcessMesh(VoxelBuildSlot* slot)
{
    if (slot->failed)
        return false;
    else
    {
        STBWorkBuffer* workBuffer = (STBWorkBuffer*)slot->workBuffer;
        VoxelChunk* chunk = slot->job->chunk;
        VoxelMesh& mesh = chunk->GetVoxelMesh(0);

        unsigned totalQuads = 0;
        BoundingBox box;
        for (unsigned i = 0; i < slot->numWorkloads; ++i)
        {
            totalQuads += workBuffer->fragmentQuads[i];
            box.Merge(workBuffer->box[i]);
        }
        mesh.numQuads_ = totalQuads;
        chunk->SetBoundingBox(box);
    }

    return true;
}

bool ClassicMeshBuilder::UploadGpuData(VoxelJob* job)
{
    PROFILE(UploadGpuData);

    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    unsigned numQuads = mesh.numQuads_;
    if (numQuads == 0)
        return true;

    VertexBuffer* chunkVertexData = mesh.vertexData_;
    IndexBuffer* chunkFaceData = mesh.faceData_;

    if (!chunkVertexData->SetSize(numQuads * 4, MASK_DATA, false))
    {
        LOGERROR("Error allocating voxel vertex data.");
        return false;
    }

    if (!chunkFaceData->SetSize(numQuads, true, false))
    {
        LOGERROR("Error allocating voxel face data.");
        return false;
    }

    unsigned quadPosition = 0;
    for (unsigned i = 0; i < VOXEL_MAX_WORKERS; ++i)
    {
        unsigned quadCount = mesh.rawFaceData_[i].Size() / sizeof(unsigned);
        if (!chunkVertexData->SetDataRange(&mesh.rawVertexData_[i].Front(), quadPosition * 4, quadCount * 4))
        {
            LOGERROR("Error uploading voxel vertex data.");
            return false;
        }

        if (!chunkFaceData->SetDataRange(&mesh.rawFaceData_[i].Front(), quadPosition, quadCount))
        {
            LOGERROR("Error uploading voxel face data.");
            return false;
        }
        quadPosition += quadCount;
    }

    // face data
    Material* material = mesh.material_;
    {
        TextureBuffer* faceDataTexture = mesh.faceBuffer_;
        if (!faceDataTexture->SetSize(0))
        {
            LOGERROR("Error initializing voxel texture buffer");
            return false;
        }
        if (!faceDataTexture->SetData(chunkFaceData))
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
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, numQuads * 6, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }
    return true;
}

bool ClassicMeshBuilder::UpdateMaterialParameters(Material* material)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Technique* technique = cache->GetResource<Technique>("Techniques/VoxelDiff.xml");
    material->SetTechnique(0, technique);
    material->SetShaderParameter("Transform", transform_);
    material->SetShaderParameter("NormalTable", normals_);
    material->SetShaderParameter("AmbientTable", ambientTable_);
    material->SetShaderParameter("TexScale", texscaleTable_);
    material->SetShaderParameter("TexGen", texgenTable_);
    material->SetShaderParameter("ColorTable", defaultColorTable_);
    return true;
}

bool ClassicMeshBuilder::ResizeIndexBuffer(unsigned numQuads)
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

void ClassicMeshBuilder::AssignWork(VoxelBuildSlot* slot)
{
    unsigned oldSize = workBuffers_.Size();
    if (workBuffers_.Size() <= slot->index)
        workBuffers_.Resize(slot->index + 1);

    for (unsigned i = oldSize; i < workBuffers_.Size(); ++i)
    {
        for (unsigned x = 0; x < VOXEL_MAX_WORKERS; ++x)
            stbvox_init_mesh_maker(&workBuffers_[i].meshMakers[x]);
    }

    STBWorkBuffer* workBuffer = &workBuffers_[slot->index];
    for (unsigned x = 0; x < VOXEL_MAX_WORKERS; ++x)
    {
        workBuffer->fragmentQuads[x] = 0;
        workBuffer->box[x] = BoundingBox();
    }
    slot->workBuffer = workBuffer;
}

void ClassicMeshBuilder::FreeWork(VoxelBuildSlot* slot)
{

}

}
#endif
