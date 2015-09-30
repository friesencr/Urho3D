#include <Transvoxel/Transvoxel.h>

#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#include "VoxelBuilder.h"
#include "TransvoxelMeshBuilder.h"

namespace Urho3D
{

static const Vector3 CORNER_TABLE[8] = {
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    { -0.5f,  0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f,  0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f }
};

TransvoxelMeshBuilder::TransvoxelMeshBuilder(Context* context) : VoxelMeshBuilder(context) 
{

}

unsigned TransvoxelMeshBuilder::VoxelDataCompatibilityMask() const {
    return VOXEL_BLOCK_BLOCKTYPE;
}

bool TransvoxelMeshBuilder::BuildMesh(VoxelBuildSlot* slot)
{
    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    VoxelMap* voxelMap = job->voxelMap;
    VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;
    TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)&workBuffers_[slot->index];
    unsigned char* vb = workBuffer->vertexBuffer;
    unsigned * ib = workBuffer->indexBuffer;
    unsigned xStride = voxelMap->GetStrideX();
    unsigned zStride = voxelMap->GetStrideZ();
    unsigned numVertices = 0;
    bool success = true;
    BoundingBox box;
    unsigned vertexMask = MASK_POSITION | MASK_COLOR; // | MASK_NORMAL;

    // voxel data index offests per direction
    const int lsw = 0;
    const int lse = xStride;
    const int lnw = zStride;
    const int lne = xStride  +  zStride;
    const int hsw = 1;
    const int hse = xStride + 1;
    const int hnw = zStride + 1;
    const int hne = xStride + zStride + 1;

    // holds the index of previous built indexes
    unsigned deckCache[32*32*2*4]; // 32 * 32 is max size, use 2 Y layers and swap between per Y, max 4 verts reuse per cell
    unsigned deckSize = 32 * 32;

    // offset to position in deckCache x,y,z | 1,2,4 bits -1 to position per coord respectivly
    const int reuseOffsets[8] = {
        0,
        -VOXEL_CHUNK_SIZE_Z,
        -(VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
        -VOXEL_CHUNK_SIZE_Z - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
        - 1,
        -VOXEL_CHUNK_SIZE_Z - 1,
        -1  - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
        -1 - VOXEL_CHUNK_SIZE_Z - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
    };
    
    unsigned char* blocktype = &voxelMap->GetBlocktypeData().Front();

    for (unsigned y = 0; y < VOXEL_CHUNK_SIZE_Y; ++y)
    { 
        for (unsigned x = 0; x < VOXEL_CHUNK_SIZE_X; ++x)
        {
            unsigned index = voxelMap->GetIndex(x, y, 0);
            unsigned char* bt = &blocktype[index];

            for (unsigned z = 0; z < VOXEL_CHUNK_SIZE_Z; ++z)
            {
                unsigned char bt_lsw = bt[lsw];
                unsigned char bt_lse = bt[lse];
                unsigned char bt_lnw = bt[lnw];
                unsigned char bt_lne = bt[lne];
                unsigned char bt_hsw = bt[hsw];
                unsigned char bt_hse = bt[hse];
                unsigned char bt_hnw = bt[hnw];
                unsigned char bt_hne = bt[hne];

                bt += zStride;

                unsigned char caseCode =
                    (bt_lsw > 0)
                    | ((bt_lse > 0) << 1)
                    | ((bt_lnw > 0) << 2)
                    | ((bt_lne > 0) << 3)
                    | ((bt_hsw > 0) << 4)
                    | ((bt_hse > 0) << 5)
                    | ((bt_hnw > 0) << 6)
                    | ((bt_hne > 0) << 7);


                if (caseCode == 0 || caseCode == 0xFF)
                    continue;

                const RegularCellData& regularData = regularCellData[regularCellClass[caseCode]];
                const unsigned short* vertexData = regularVertexData[caseCode];
                const unsigned triangleCount = regularData.GetTriangleCount();

                box.Merge(Vector3(x, y, z));

                for (unsigned i = 0; i < triangleCount; ++i)
                {
                    const unsigned char* triangleData = &regularData.vertexIndex[i * 3];
                    for (unsigned j = 3; j-- > 0; )
                    {
                        const unsigned short& edgeCode = vertexData[triangleData[j]];
                        unsigned short reusePos = edgeCode >> 12;
                        unsigned short reuseTri = (edgeCode >> 8) & 0x0F;
                        unsigned short v0 = (edgeCode >> 4) & 0x0F;
                        unsigned short v1 = edgeCode & 0x0F;
                        unsigned char t = 0x7F; // half way for now

                        if (reusePos == 8)
                        {
                            *ib++ = numVertices;
                            unsigned deckIndex = ((y % 2) * deckSize) + (x * 32) + z + reuseTri;
                            deckCache[deckIndex] = numVertices;
                            numVertices++;

                            if (vertexMask & MASK_POSITION)
                            {
                                const Vector3& p1 = CORNER_TABLE[v0];
                                const Vector3& p2 = CORNER_TABLE[v1];
                                Vector3 position = Vector3((float)x, (float)y, (float)z) + p1.Lerp(p2, 0.5);
                                *(Vector3*)vb = position;
                                vb += sizeof(Vector3);
                            }

                            if (vertexMask & MASK_COLOR)
                            {
                                *(unsigned*)vb = Color((float)y / 128.0, (float)y / 128.0, (float)y / 128.0, 1.0f).ToUInt();
                                vb += sizeof(unsigned);
                            }

                            if (vertexMask & MASK_NORMAL)
                            {
                                if (j == 0)
                                    *(Vector3*)vb = Vector3(1.0, 0.0, 0.0);
                                else if (j == 1)
                                    *(Vector3*)vb = Vector3(0.0, 1.0, 0.0);
                                else if (j == 2)
                                    *(Vector3*)vb = Vector3(0.0, 0.0, 1.0);

                                vb += sizeof(Vector3);
                            }
                        }
                        else
                        {
                            unsigned deckIndex = ((((reuseTri & 0x2) + y) % 2) * deckSize) + (x * 32) + z + reuseTri;
                            *ib++ = deckCache[deckIndex];
                        }
                    }
                }
            }
        }
    }
    workBuffer->box = box;
    workBuffer->numVerticies = numVertices;
    workBuffer->numIndicies = ib - workBuffer->indexBuffer;
    workBuffer->numTriangles = workBuffer->numIndicies / 3;
    return success;
}

bool TransvoxelMeshBuilder::ProcessMesh(VoxelBuildSlot* slot)
{
    if (slot->failed)
        return false;
    else
    {
        TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)&workBuffers_[slot->index];
        VoxelChunk* chunk = slot->job->chunk;
        VoxelMesh& mesh = chunk->GetVoxelMesh(0);

        mesh.numVertices_ = workBuffer->numVerticies;
        mesh.numTriangles_ = workBuffer->numTriangles;
        chunk->SetBoundingBox(workBuffer->box);
    }

    return true;
}

bool TransvoxelMeshBuilder::UploadGpuData(VoxelBuildSlot* slot)
{
    PROFILE(UploadGpuData);

    TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)&workBuffers_[slot->index];
    VoxelChunk* chunk = slot->job->chunk;
    chunk->SetNumberOfMeshes(1);

    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    VertexBuffer* vb = new VertexBuffer(context_);
    vb->SetShadowed(false);
    IndexBuffer* ib = new IndexBuffer(context_);
    ib->SetShadowed(false);

    unsigned vertexSize = 0;
    unsigned vertexMask = MASK_POSITION | MASK_COLOR; // | MASK_NORMAL;
    for (unsigned i = 0; i < MAX_VERTEX_ELEMENTS; ++i)
    {
        if (vertexMask & (1 << i))
            vertexSize += VertexBuffer::elementSize[i];
    }

    if (!vb->SetSize(mesh.numVertices_, vertexMask, false))
    {
        LOGERROR("Error allocating voxel vertex buffer.");
        return false;
    }

    if (!ib->SetSize(mesh.numIndicies_, true, false))
    {
        LOGERROR("Error allocating voxel index buffer.");
        return false;
    }

    unsigned vertexCount = 0;
    if (!vb->SetData(&workBuffer->vertexBuffer))
    {
        LOGERROR("Error uploading voxel vertex data.");
        return false;
    }

    if (!ib->SetData(&workBuffer->indexBuffer))
    {
        LOGERROR("Error uploading voxel face data.");
        return false;
    }

    Geometry* geo = mesh.geometry_;
    geo->SetVertexBuffer(0, vb, vertexMask);
    geo->SetIndexBuffer(ib);
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, mesh.numIndicies_, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }

    return true;
}

bool TransvoxelMeshBuilder::UpdateMaterialParameters(Material* material)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Technique* technique = cache->GetResource<Technique>("Techniques/NoTextureUnlitVCol.xml");
    material->SetTechnique(0, technique);
    return true;
}

void TransvoxelMeshBuilder::AssignWork(VoxelBuildSlot* slot)
{
    unsigned oldSize = workBuffers_.Size();
    if (workBuffers_.Size() <= slot->index)
        workBuffers_.Resize(slot->index + 1);

    TransvoxelWorkBuffer* workBuffer = &workBuffers_[slot->index];
    workBuffer->numTriangles = 0;
    workBuffer->numIndicies = 0;
    workBuffer->numVerticies = 0;
    workBuffer->box = BoundingBox();
}

void TransvoxelMeshBuilder::FreeWork(VoxelBuildSlot* slot)
{

}

}
