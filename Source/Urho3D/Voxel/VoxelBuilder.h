#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Core/WorkQueue.h"
#include "../Container/Ptr.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Material.h"

#include "VoxelDefs.h"
#include "VoxelUtils.h"
#include "VoxelMap.h"
#include "VoxelChunk.h"

namespace Urho3D {

class VoxelMap;
class VoxelChunk;
class VoxelBuilder;

typedef void(*VoxelProcessorFunc)(VoxelData* source, VoxelData* dest, const VoxelRangeFragment& range);

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
    SharedPtr<VoxelMap> voxelMap;
    unsigned slot;
};

struct VoxelCompletedWorkload
{
    PODVector<unsigned> vertexData;
    PODVector<unsigned> faceData;
    void* job;
    bool completed;
    BoundingBox box;
    unsigned numQuads;
};

struct VoxelWorkSlot;

struct VoxelWorkload 
{
    int slot;
    VoxelBuilder* builder;
    SharedPtr<WorkItem> workItem;
    VoxelRangeFragment range;
    unsigned workloadIndex;
    int numQuads;
    BoundingBox box;
};

class URHO3D_API VoxelBuilder : public Object {
    OBJECT(VoxelBuilder);
    friend class VoxelChunk;

public:
    VoxelBuilder(Context* context);
    ~VoxelBuilder();
    VoxelJob* BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, bool async = false);
    // needs to be public for work item work method
    void BuildWorkload(VoxelWorkload* workload);
    void CompleteWork(unsigned = M_MAX_UNSIGNED);
    void CancelJob(VoxelJob* job);
    void RegisterProcessor(String name, VoxelProcessorFunc function);
    bool UnregisterProcessor(String name);

private:

    bool ResizeIndexBuffer(unsigned size);
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();

    //
    // work commands
    //
    bool BuildMesh(VoxelWorkload* workload);
    void DecodeWorkBuffer(VoxelWorkload* workload);
    bool UploadGpuData(VoxelCompletedWorkload* workload);
    bool UploadGpuDataCompatibilityMode(VoxelWorkSlot* slot, bool append = false);
    bool UpdateMaterialParameters(Material* material, bool setColor);

    //
    // slot management
    //
    int GetFreeWorkSlot();
    unsigned DecrementWorkSlot(VoxelWorkSlot* slot);
    void TransferCompletedWork(VoxelWorkSlot* slot);
    void FreeWorkSlot(VoxelWorkSlot* slot);

    //
    // job management
    //
    void ProcessJob(VoxelJob* job);
    VoxelJob* CreateJob(VoxelChunk* chunk);
    void PurgeAllJobs();
    bool RunJobs();
    bool UploadCompletedWork();

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
    Vector<Variant> default_colorTable_;

    // completed work 
    unsigned maxReservedCompletedBuffers_;
    Vector<VoxelCompletedWorkload> completedChunks_;
    unsigned completedJobCount_;
    Mutex slotMutex_;
    Mutex completedWorkMutex_;
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
