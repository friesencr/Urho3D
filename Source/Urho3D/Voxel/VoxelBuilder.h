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
#include "VoxelWriter.h"
#include "VoxelChunk.h"
#include "VoxelMeshBuilder.h"
#include "STBMeshBuilder.h"
#include "TransvoxelMeshBuilder.h"

namespace Urho3D {

class VoxelData;
class VoxelMap;
class VoxelChunk;
class VoxelBuilder;
struct VoxelBuildSlot;

typedef void(*VoxelProcessorFunc)(VoxelData* source, VoxelData* dest, const VoxelRangeFragment& range);

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
    SharedPtr<VoxelMap> voxelMap;
    VoxelBuildSlot* slot;
    StringHash backend;
};

struct VoxelWorkload 
{
    unsigned index;
    VoxelBuildSlot* slot;
    VoxelBuilder* builder;
    VoxelRangeFragment range;
    SharedPtr<WorkItem> workItem;
};

struct VoxelBuildSlot
{
    unsigned index;
    VoxelJob* job;
    bool failed;
    bool free;
    int workCounter;
    int numWorkloads;
    VoxelMeshBuilder* backend;
    VoxelWorkload workloads[4];
    Mutex slotMutex;
    void* workBuffer;
    VoxelWriter writer;
};

class URHO3D_API VoxelBuilder : public Object {
    OBJECT(VoxelBuilder);

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
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();
    bool RunVoxelProcessor(VoxelWorkload* workload);

    //
    // slot management
    //
    int GetFreeBuildSlot();
    unsigned DecrementBuildSlot(VoxelBuildSlot* slot);
    void FreeBuildSlot(VoxelBuildSlot* slot);

    //
    // job management
    //
    void ProcessJob(VoxelJob* job);
    bool RunJobs();
    bool IsBuilding();
    bool UploadCompletedWork();

    // 
    // state
    //
    Vector<VoxelJob*> buildJobs_;
    Vector<VoxelJob*> uploadJobs_;
    Vector<VoxelBuildSlot> slots_;

    // completed work 
    Mutex slotMutex_;
    Mutex uploadJobMutex_;
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
