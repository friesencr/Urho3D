#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Voxel.h"
#include "VoxelChunk.h"

#include <STB/stb_voxel_render.h>

#define VOXEL_COMPATIBLE = 0
 
namespace Urho3D {

static const unsigned char VOXEL_WORKER_SIZE_X = 32;
static const unsigned char VOXEL_WORKER_SIZE_Y = 128;
static const unsigned char VOXEL_WORKER_SIZE_Z = 32;

// VOXEL CONFIG MODE 0 - is 2 uints per vertex
static const unsigned VOXEL_WORKER_MAX_QUADS = 400000;
static const unsigned VOXEL_WORKER_VERTEX_BUFFER_SIZE = VOXEL_WORKER_MAX_QUADS * 4 * 4;
static const unsigned VOXEL_WORKER_FACE_BUFFER_SIZE = VOXEL_WORKER_MAX_QUADS * 4;

static const unsigned char VOXEL_CHUNK_SIZE_X = 64;
static const unsigned char VOXEL_CHUNK_SIZE_Y = 128;
static const unsigned char VOXEL_CHUNK_SIZE_Z = 64;
static const unsigned VOXEL_CHUNK_SIZE = VOXEL_CHUNK_SIZE_X * VOXEL_CHUNK_SIZE_Y * VOXEL_CHUNK_SIZE_Z;

class VoxelBuilder;
class VoxelChunk;
struct VoxelWorkSlot;
struct VoxelWorkload;

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
	unsigned slot;
};

struct VoxelWorkSlot
{
    Vector<VoxelWorkload> workloads;
    VoxelJob* job;
    stbvox_mesh_maker meshMakers[4];
    unsigned char workVertexBuffers[4][VOXEL_WORKER_VERTEX_BUFFER_SIZE];
    unsigned char workFaceBuffers[4][VOXEL_WORKER_FACE_BUFFER_SIZE];
    int numQuads;
    bool failed;
    bool free;
	bool upload;
    int workCounter;
	int numWorkloads;
    BoundingBox box;
    Mutex workMutex;
    Mutex dataMutex;
};

struct VoxelWorkload 
{
    int slot;
    VoxelBuilder* builder;
    SharedPtr<WorkItem> workItem;
    //PODVector<unsigned char> gpuData;
    //PODVector<unsigned char> cpuData;
	PODVector<Vector3> shadowData;
    unsigned char index[3];
    unsigned char start[3];
    unsigned char end[3];
    unsigned workloadIndex;
    int numQuads;
    BoundingBox box;
};

class URHO3D_API VoxelBuilder : public Object {
    OBJECT(VoxelBuilder);

public:
    VoxelBuilder(Context* context);
    ~VoxelBuilder();
    VoxelJob* BuildVoxelChunk(SharedPtr<VoxelChunk> chunk, bool async = false);
    // needs to be public for work item work method
    void BuildWorkload(VoxelWorkload* workload);
    void CompleteWork(unsigned = M_MAX_UNSIGNED);
    void CancelJob(VoxelJob* job);
    bool compatibilityMode;

private:

    bool ResizeIndexBuffer(unsigned size);
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();

    //
    // work commands
    //
    bool BuildMesh(VoxelWorkload* workload);
    void DecodeWorkBuffer(VoxelWorkload* workload);
    bool UploadGpuData(VoxelWorkSlot* slot, bool append = false);
    bool UploadGpuDataCompatibilityMode(VoxelWorkSlot* slot, bool append = false);
	bool SetMaterialParameters(Material* material);
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
	unsigned SimplifyMesh(VoxelWorkSlot* slot, unsigned char* verticies, unsigned char* normals, unsigned char** newMesh);
	bool PendingWork();

    //
    // slot management
    //
    int GetFreeWorkSlot();
    void AbortSlot(VoxelWorkSlot* slot);
    unsigned DecrementWorkSlot(VoxelWorkSlot* slot);
    void FreeWorkSlot(VoxelWorkSlot* slot);
    void ProcessSlot(VoxelWorkSlot* slot);

    //
    // job management
    //
    void ProcessJob(VoxelJob* job);
    void RemoveJob(VoxelJob* job);
    void QueueJob(VoxelJob* job);
    VoxelJob* CreateJob(VoxelChunk* chunk);
    void PurgeAllJobs();
    bool RunJobs();

    // 
    // state
    //
    Vector<VoxelJob*> jobs_;
    SharedPtr<IndexBuffer> sharedIndexBuffer_;
    Vector<VoxelWorkSlot> slots_;
	Vector<Variant> transform_;
	Vector<Variant> normals_;
	Vector<Variant> ambientTable_;
	Vector<Variant> texscaleTable_;
	Vector<Variant> texgenTable_;
	Vector<Variant> colorTable_;
	Mutex slotMutex_;
	Timer frameTimer_;
	unsigned maxFrameTime_;
	bool frameReset_;
	bool completeAllWork_;
};

}
