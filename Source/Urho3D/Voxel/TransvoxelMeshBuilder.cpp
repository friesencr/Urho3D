#include <Transvoxel/Transvoxel.h>

#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#include "VoxelBuilder.h"
#include "TransvoxelMeshBuilder.h"

namespace Urho3D
{

static const Vector3 CORNER_TABLE[8] = {
    Vector3( -0.5f, -0.5f, -0.5f ),
    Vector3(  0.5f, -0.5f, -0.5f ),
    Vector3( -0.5f, -0.5f,  0.5f ),
    Vector3(  0.5f, -0.5f,  0.5f ),
    Vector3( -0.5f,  0.5f, -0.5f ),
    Vector3(  0.5f,  0.5f, -0.5f ),
    Vector3( -0.5f,  0.5f,  0.5f ),
    Vector3(  0.5f,  0.5f,  0.5f )
};

TransvoxelMeshBuilder::TransvoxelMeshBuilder(Context* context) : VoxelMeshBuilder(context) 
{
    workBuffers_.Resize(16);
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
    unsigned vertexSize = sizeof(Vector3) + sizeof(unsigned);
    bool success = true;
    BoundingBox box;
    unsigned vertexMask = MASK_POSITION | MASK_COLOR; // | MASK_NORMAL;

    // voxel data index offests per direction
    // TODO offset by -1
    const int lsw = 0;
    const int lse = xStride;
    const int lnw = zStride;
    const int lne = xStride  +  zStride;
    const int hsw = 1;
    const int hse = xStride + 1;
    const int hnw = zStride + 1;
    const int hne = xStride + zStride + 1;

    // holds the index of previous built indexes
    const unsigned deckSize = VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z;
    const unsigned deckCacheSize = deckSize * 2 * 4; // 32 * 32 is max size, use 2 Y layers and swap between per Y, max 4 verts reuse per cell
    unsigned deckCache[deckCacheSize]; 
    

    // offset to position in deckCache x,y,z | 1,2,4 bits -1 to position per coord respectivly
//     const int reuseOffsets[8] = {
//         0,
//         -VOXEL_CHUNK_SIZE_Z,
//         -(VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
//         -VOXEL_CHUNK_SIZE_Z - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
//         - 1,
//         -VOXEL_CHUNK_SIZE_Z - 1,
//         -1  - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
//         -1 - VOXEL_CHUNK_SIZE_Z - (VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Z),
//     };
    
    const int reuseOffsets[8] = {
        0,
        -1,
        -(VOXEL_CHUNK_SIZE_X),
        -1 - (VOXEL_CHUNK_SIZE_X),
        0,
        -1,
        -(VOXEL_CHUNK_SIZE_X),
        -1 - (VOXEL_CHUNK_SIZE_X),
    };
    
    unsigned char* blocktype = &voxelMap->GetBlocktypeData().Front();

    for (unsigned y = 0; y < VOXEL_CHUNK_SIZE_Y; ++y)
    { 
        for (unsigned z = 0; z < VOXEL_CHUNK_SIZE_Z; ++z)
        {
            unsigned index = voxelMap->GetIndex(0, y, z);
            unsigned char* bt = &blocktype[index];
            for (unsigned x = 0; x < VOXEL_CHUNK_SIZE_X; ++x)
            {
                unsigned char bt_lsw = bt[lsw];
                unsigned char bt_lse = bt[lse];
                unsigned char bt_lnw = bt[lnw];
                unsigned char bt_lne = bt[lne];
                unsigned char bt_hsw = bt[hsw];
                unsigned char bt_hse = bt[hse];
                unsigned char bt_hnw = bt[hnw];
                unsigned char bt_hne = bt[hne];

                bt += xStride;

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

                        if (reusePos == 8 || y == 0 || x == 0 || z == 0)
                        {
                            *ib++ = numVertices;
                            int deckIndex = ((
                                ((y % 2) * deckSize) + 
                                x + 
                                (z * VOXEL_CHUNK_SIZE_X)) * 4
                            ) + reuseTri;
                            deckCache[deckIndex] = numVertices;

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
                                
                            /* LOGINFO(String("NEW  - ") + */ 
                            /*     String(x) */
                            /*     + " - " + String(y) */
                            /*     + " - " + String(z) */
                            /*     + " - " + String(reuseTri) */ 
                            /*     + " - " + String(deckIndex) */
                            /*     + " - " + String(numVertices) */
                            /*     //+ " - " + (*(Vector3*)workBuffer->vertexBuffer[numVertices * vertexSize]).ToString() */
                            /* ); */
                            
                            numVertices++;
                        }
                        else
                        {
                            assert(reuseTri < 5);
                            int offset = reuseOffsets[reusePos];
                            int deckIndex = 
                                (((
                                    (((y + ((reusePos & 4) ? 1 : 0)) % 2) * deckSize) +  // y
                                    x + // x
                                    (z * VOXEL_CHUNK_SIZE_X) // z
                                   ) + offset) * 4) +
                                reuseTri;  // tri
                                
                            assert(deckIndex >= 0 && deckIndex < deckCacheSize);
                            unsigned vertexId = deckCache[deckIndex];
                            /* LOGINFO(String("USED - ") + */ 
                            /*     String(x) */
                            /*     + " - " + String(y) */
                            /*     + " - " + String(z) */
                            /*     + " - " + String(reuseTri) */
                            /*     + " - " + String(deckIndex) */
                            /*     + " - " + String(vertexId) */
                            /*     + " - " + String(reusePos) */
                            /*     + " - " + String(offset) */
                            /*     //+ " - " + (*(Vector3*)workBuffer->vertexBuffer[vertexId * vertexSize]).ToString() */
                            /* ); */
                            *ib++ = vertexId;
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
    TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)&workBuffers_[slot->index];
    VoxelChunk* chunk = slot->job->chunk;
    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    VoxelJob* job = slot->job;

    mesh.numVertices_ = workBuffer->numVerticies;
    mesh.numTriangles_ = workBuffer->numTriangles;
    mesh.numIndicies_ = workBuffer->numIndicies;
    chunk->SetBoundingBox(workBuffer->box);

    Geometry* geometry = chunk->GetGeometry(0);

    unsigned vertexSize = 0;
    unsigned vertexMask = MASK_POSITION | MASK_COLOR; // | MASK_NORMAL;
    for (unsigned i = 0; i < MAX_VERTEX_ELEMENTS; ++i)
    {
        if (vertexMask & (1 << i))
            vertexSize += VertexBuffer::elementSize[i];
    }

    // sets 
    unsigned vertexBufferSize = workBuffer->numVerticies * vertexSize;
    job->vertexBuffer = new unsigned char[vertexBufferSize];
    memcpy(job->vertexBuffer.Get(), workBuffer->vertexBuffer, vertexBufferSize);
    geometry->SetRawVertexData(job->vertexBuffer, workBuffer->numVerticies, MASK_DATA);
    
    // index buffer
    unsigned indexBufferSize = workBuffer->numIndicies * sizeof(unsigned);
    job->indexBuffer = new unsigned char[indexBufferSize];
    memcpy(job->indexBuffer.Get(), workBuffer->indexBuffer, indexBufferSize);
    geometry->SetRawIndexData(job->indexBuffer, sizeof(unsigned));

    return true;
}

bool TransvoxelMeshBuilder::UploadGpuData(VoxelJob* job)
{
    PROFILE(UploadGpuData);

    VoxelChunk* chunk = job->chunk;
    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    unsigned numTriangles = mesh.numTriangles_;
    if (numTriangles == 0)
        return true;

    VertexBuffer* vb = mesh.vertexBuffer_;
    IndexBuffer* ib = mesh.indexBuffer_;

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

    if (!vb->SetData(job->vertexBuffer.Get()))
    {
        LOGERROR("Error uploading voxel vertex data.");
        return false;
    }

    if (!ib->SetData(job->indexBuffer.Get()))
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
