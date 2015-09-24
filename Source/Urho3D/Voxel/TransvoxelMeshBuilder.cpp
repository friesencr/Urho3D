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

static const Vector3 EDGE_TABLE[8][8] = {
    {
        CORNER_TABLE[0] - CORNER_TABLE[0],
        CORNER_TABLE[0] - CORNER_TABLE[1],
        CORNER_TABLE[0] - CORNER_TABLE[2],
        CORNER_TABLE[0] - CORNER_TABLE[3],
        CORNER_TABLE[0] - CORNER_TABLE[4],
        CORNER_TABLE[0] - CORNER_TABLE[5],
        CORNER_TABLE[0] - CORNER_TABLE[6],
        CORNER_TABLE[0] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[1] - CORNER_TABLE[0],
        CORNER_TABLE[1] - CORNER_TABLE[1],
        CORNER_TABLE[1] - CORNER_TABLE[2],
        CORNER_TABLE[1] - CORNER_TABLE[3],
        CORNER_TABLE[1] - CORNER_TABLE[4],
        CORNER_TABLE[1] - CORNER_TABLE[5],
        CORNER_TABLE[1] - CORNER_TABLE[6],
        CORNER_TABLE[1] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[2] - CORNER_TABLE[0],
        CORNER_TABLE[2] - CORNER_TABLE[1],
        CORNER_TABLE[2] - CORNER_TABLE[2],
        CORNER_TABLE[2] - CORNER_TABLE[3],
        CORNER_TABLE[2] - CORNER_TABLE[4],
        CORNER_TABLE[2] - CORNER_TABLE[5],
        CORNER_TABLE[2] - CORNER_TABLE[6],
        CORNER_TABLE[2] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[3] - CORNER_TABLE[0],
        CORNER_TABLE[3] - CORNER_TABLE[1],
        CORNER_TABLE[3] - CORNER_TABLE[2],
        CORNER_TABLE[3] - CORNER_TABLE[3],
        CORNER_TABLE[3] - CORNER_TABLE[4],
        CORNER_TABLE[3] - CORNER_TABLE[5],
        CORNER_TABLE[3] - CORNER_TABLE[6],
        CORNER_TABLE[3] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[4] - CORNER_TABLE[0],
        CORNER_TABLE[4] - CORNER_TABLE[1],
        CORNER_TABLE[4] - CORNER_TABLE[2],
        CORNER_TABLE[4] - CORNER_TABLE[3],
        CORNER_TABLE[4] - CORNER_TABLE[4],
        CORNER_TABLE[4] - CORNER_TABLE[5],
        CORNER_TABLE[4] - CORNER_TABLE[6],
        CORNER_TABLE[4] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[5] - CORNER_TABLE[0],
        CORNER_TABLE[5] - CORNER_TABLE[1],
        CORNER_TABLE[5] - CORNER_TABLE[2],
        CORNER_TABLE[5] - CORNER_TABLE[3],
        CORNER_TABLE[5] - CORNER_TABLE[4],
        CORNER_TABLE[5] - CORNER_TABLE[5],
        CORNER_TABLE[5] - CORNER_TABLE[6],
        CORNER_TABLE[5] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[6] - CORNER_TABLE[0],
        CORNER_TABLE[6] - CORNER_TABLE[1],
        CORNER_TABLE[6] - CORNER_TABLE[2],
        CORNER_TABLE[6] - CORNER_TABLE[3],
        CORNER_TABLE[6] - CORNER_TABLE[4],
        CORNER_TABLE[6] - CORNER_TABLE[5],
        CORNER_TABLE[6] - CORNER_TABLE[6],
        CORNER_TABLE[6] - CORNER_TABLE[7],
    },
    {
        CORNER_TABLE[7] - CORNER_TABLE[0],
        CORNER_TABLE[7] - CORNER_TABLE[1],
        CORNER_TABLE[7] - CORNER_TABLE[2],
        CORNER_TABLE[7] - CORNER_TABLE[3],
        CORNER_TABLE[7] - CORNER_TABLE[4],
        CORNER_TABLE[7] - CORNER_TABLE[5],
        CORNER_TABLE[7] - CORNER_TABLE[6],
        CORNER_TABLE[7] - CORNER_TABLE[7],
    }
};


TransvoxelMeshBuilder::TransvoxelMeshBuilder(Context* context) : VoxelMeshBuilder(context) 
{

}

unsigned TransvoxelMeshBuilder::VoxelDataCompatibilityMask() const {
    return VOXEL_BLOCK_BLOCKTYPE;
}

bool TransvoxelMeshBuilder::BuildMesh(VoxelWorkload* workload)
{
    VoxelBuildSlot* slot = workload->slot;
    VoxelJob* job = slot->job;
    VoxelChunk* chunk = job->chunk;
    VoxelMap* voxelMap = job->voxelMap;
    VoxelBlocktypeMap* voxelBlocktypeMap = voxelMap->blocktypeMap;
    TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)slot->workBuffer;
    VoxelRangeFragment range = workload->range;

    unsigned xStride = voxelMap->GetStrideX();
    unsigned zStride = voxelMap->GetStrideZ();

    bool success = true;
    unsigned numVertices = 0;

    unsigned char* blocktype = &voxelMap->GetBlocktypeData().Front();

    const int lsw = -xStride - zStride - 1;
    const int lse = xStride - zStride - 1;
    const int lnw = -xStride + zStride - 1;
    const int lne = xStride + zStride - 1;
    const int hsw = -xStride - zStride + 1;
    const int hse = xStride - zStride + 1;
    const int hnw = -xStride + zStride + 1;
    const int hne = xStride + zStride + 1;

    static const int relativeCorners[8] = {
        0,
        -xStride,
        -1,
        -xStride - 1,
        -zStride,
        -xStride - zStride,
        -1 - zStride,
        -xStride - 1 - zStride
    };

    unsigned char* vb = workBuffer->workVertexBuffers[workload->index];
    unsigned * ib = (unsigned*)workBuffer->workIndexBuffers[workload->index];
    BoundingBox box;

    unsigned vertexMask = MASK_POSITION | MASK_COLOR;

    for (unsigned x = range.startX; x < range.endX; ++x)
    {
        for (unsigned z = range.startZ; z < range.endZ; ++z)
        {
            unsigned index = voxelMap->GetIndex(x, range.startY, z);
            unsigned char* bt = &blocktype[index];
            for (unsigned y = range.startY; y < range.endY; ++y)
            {
                unsigned char caseCode =
                      ((bt[lsw] > 0) << 7)
                    | ((bt[lse] > 0) << 6)
                    | ((bt[lnw] > 0) << 5)
                    | ((bt[lne] > 0) << 4)
                    | ((bt[hsw] > 0) << 3)
                    | ((bt[hse] > 0) << 2)
                    | ((bt[hnw] > 0) << 1)
                    | ( bt[hne] > 0);

                bt++;

                if (caseCode == 0 || caseCode == 0xFF)
                    continue;

                const RegularCellData& regularData = regularCellData[regularCellClass[caseCode]];
                const unsigned short* vertexData = regularVertexData[caseCode];
                const unsigned triangleCount = regularData.GetTriangleCount();

                for (unsigned i = 0; i < triangleCount; ++i)
                {
                    const unsigned char* triangleData = &regularData.vertexIndex[i * 3];
                    for (unsigned j = 0; j < 3; ++j)
                    {
                        const unsigned short& edgeCode = vertexData[triangleData[j]];
                        unsigned short v0 = (edgeCode >> 4) & 0x0F;
                        unsigned short v1 = edgeCode & 0x0F;
                        unsigned char t = 0x7F; // half way for now

                        if (vertexMask & MASK_POSITION)
                        {
                            Vector3 p1 = CORNER_TABLE[v0];
                            Vector3 p2 = CORNER_TABLE[v1];
                            Vector3 position = Vector3((float)x, (float)y, (float)z) + (p2); // *((float)t / 255.0f));
                            *(Vector3*)vb = position;
                            vb += sizeof(Vector3);
                            ib[i] = i;
                            numVertices++;
                            box.Merge(position);
                        }

                        if (vertexMask & MASK_COLOR)
                        {
                            *(unsigned*)vb = Color((float)y / 128.0, (float)y / 128.0, (float)y / 128.0, 1.0f).ToUInt();
                            vb += sizeof(unsigned);
                        }
                    }
                }
            }
        }
    }
    workBuffer->box[workload->index] = box;
    workBuffer->fragmentTriangles[workload->index] = numVertices / 3;
    workBuffer->fragmentVertices[workload->index] = numVertices;
    return success;
}

bool TransvoxelMeshBuilder::ProcessMeshFragment(VoxelWorkload* workload)
{
    VoxelBuildSlot* slot = workload->slot;
    VoxelChunk* chunk = slot->job->chunk;
    TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)slot->workBuffer;
    VoxelMesh& voxelMesh = chunk->GetVoxelMesh(0);

    unsigned vertexMask = MASK_POSITION | MASK_COLOR;
    unsigned vertexSize = 0;
    for (unsigned i = 0; i < MAX_VERTEX_ELEMENTS; ++i)
    {
        if (vertexMask & (1 << i))
            vertexSize += VertexBuffer::elementSize[i];
    }

    {
        unsigned count = workBuffer->fragmentVertices[workload->index];
        voxelMesh.rawIndexData_[workload->index].Resize(count * sizeof(unsigned));
        void* destIb = &voxelMesh.rawIndexData_[workload->index].Front();
        void* srcIb = &workBuffer->workIndexBuffers[workload->index];
        memcpy(destIb, srcIb, count * sizeof(unsigned));
    }

    {
        unsigned count = workBuffer->fragmentVertices[workload->index];
        voxelMesh.rawVertexData_[workload->index].Resize(count * vertexSize);
        void* destVb = &voxelMesh.rawVertexData_[workload->index].Front();
        void* srcVb = &workBuffer->workVertexBuffers[workload->index];
        memcpy(destVb, srcVb, count * vertexSize);
    }
    
    return true;
}

bool TransvoxelMeshBuilder::ProcessMesh(VoxelBuildSlot* slot)
{
    if (slot->failed)
        return false;
    else
    {
        TransvoxelWorkBuffer* workBuffer = (TransvoxelWorkBuffer*)slot->workBuffer;
        VoxelChunk* chunk = slot->job->chunk;
        VoxelMesh& mesh = chunk->GetVoxelMesh(0);

        unsigned numVerts = 0;
        BoundingBox box;
        unsigned indexCount = 0;
        unsigned vertexCount = 0;
        for (unsigned i = 0; i < slot->numWorkloads; ++i)
        {
            if (i > 0)
            {
                unsigned* ib = (unsigned*)&mesh.rawIndexData_[i].Front();
                for (unsigned x = 0; x < workBuffer->fragmentVertices[i]; ++x)
                    ib[x] += indexCount;
            }
            indexCount += workBuffer->fragmentVertices[i];
            numVerts += workBuffer->fragmentVertices[i];
            box.Merge(workBuffer->box[i]);
        }
        mesh.numVertices_ = numVerts;
        mesh.numTris_ = numVerts / 3;
        chunk->SetBoundingBox(box);
    }

    return true;
}

bool TransvoxelMeshBuilder::UploadGpuData(VoxelJob* job)
{
    PROFILE(UploadGpuData);

    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

    VoxelMesh& mesh = chunk->GetVoxelMesh(0);
    VoxelMap* voxelMap = job->voxelMap;

    VertexBuffer* vb = new VertexBuffer(context_);
    //IndexBuffer* ib = new IndexBuffer(context_);
    mesh.vertexBuffer_ = vb;
    //mesh.indexBuffer_ = ib;

    unsigned vertexSize = 0;
    unsigned vertexMask = MASK_POSITION | MASK_COLOR;
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

    //if (!ib->SetSize(mesh.numVertices_, true, false))
    //{
    //    LOGERROR("Error allocating voxel index buffer.");
    //    return false;
    //}

    unsigned vertexCount = 0;
    for (unsigned i = 0; i < VOXEL_MAX_WORKERS; ++i)
    {
        if (!vb->SetDataRange(&mesh.rawVertexData_[i].Front(), vertexCount, mesh.rawVertexData_[i].Size() / vertexSize))
        {
            LOGERROR("Error uploading voxel vertex data.");
            return false;
        }
        vertexCount += mesh.rawVertexData_[i].Size() / vertexSize;
    }

    //unsigned indexCount = 0;
    //for (unsigned i = 0; i < VOXEL_MAX_WORKERS; ++i)
    //{
    //    if (!ib->SetDataRange(&mesh.rawIndexData_[i].Front(), indexCount, mesh.rawIndexData_[i].Size() / sizeof(unsigned)))
    //    {
    //        LOGERROR("Error uploading voxel face data.");
    //        return false;
    //    }
    //    indexCount += mesh.rawIndexData_[i].Size() / sizeof(unsigned);
    //}

    Geometry* geo = mesh.geometry_;
    geo->SetVertexBuffer(0, vb, vertexMask);
    //geo->SetIndexBuffer(ib);
    if (!geo->SetDrawRange(TRIANGLE_LIST, 0, 0, 0, mesh.numVertices_, false))
    {
        LOGERROR("Error setting voxel draw range");
        return false;
    }
    return true;
}

bool TransvoxelMeshBuilder::UpdateMaterialParameters(Material* material)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Technique* technique = cache->GetResource<Technique>("Techniques/NoTextureVCol.xml");
    material->SetTechnique(0, technique);
    return true;
}

void TransvoxelMeshBuilder::AssignWork(VoxelBuildSlot* slot)
{
    unsigned oldSize = workBuffers_.Size();
    if (workBuffers_.Size() <= slot->index)
        workBuffers_.Resize(slot->index + 1);

    TransvoxelWorkBuffer* workBuffer = &workBuffers_[slot->index];
    for (unsigned x = 0; x < VOXEL_MAX_WORKERS; ++x)
    {
        workBuffer->fragmentTriangles[x] = 0;
        workBuffer->box[x] = BoundingBox();
    }
    slot->workBuffer = workBuffer;
}

void TransvoxelMeshBuilder::FreeWork(VoxelBuildSlot* slot)
{

}

}
