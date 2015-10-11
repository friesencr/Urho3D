#pragma once

#include "../Core/Context.h"
#include "../Core/Variant.h"
#include "../Container/Ptr.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Material.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Thread.h"
#include "../Core/WorkQueue.h"

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

const int VOXEL_BUILD_STATE_FREE = 0;
const int VOXEL_BUILD_STATE_READY = 1;
const int VOXEL_BUILD_STATE_STARTED = 2;
const int VOXEL_BUILD_STATE_UPLOAD = 3;
const int VOXEL_BUILD_STATE_COMPLETE = 4;
const int VOXEL_BUILD_STATE_FAIL = 5;

typedef void(*VoxelProcessorFunc)(VoxelData* source, VoxelData* dest, const VoxelRangeFragment& range);

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
    WeakPtr<VoxelMap> voxelMap;
    VoxelBuildSlot* slot;
    VoxelBuilder* builder;
    StringHash backend;
    SharedArrayPtr<unsigned char> vertexBuffer;
    SharedArrayPtr<unsigned char> indexBuffer;
    void* extra;
};

struct VoxelBuildSlot
{
    unsigned index;
    int state;
    VoxelJob* job;
    VoxelWriter writer;
};

class URHO3D_API VoxelBuilder : public Object {
    OBJECT(VoxelBuilder);

public:
    VoxelBuilder(Context* context);
    ~VoxelBuilder();
    VoxelJob* BuildVoxelChunk(VoxelChunk* chunk, bool async = false);
    // needs to be public for work item work method
    void CompleteWork(unsigned = M_MAX_UNSIGNED);
    //void CancelJob(VoxelJob* job);
    void RegisterProcessor(String name, VoxelProcessorFunc function);
    bool UnregisterProcessor(String name);
    void BuildWorkload(VoxelJob* job, unsigned slotIndex);

private:
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();
    bool RunVoxelProcessor(VoxelBuildSlot* slot);
    
    /// Handle frame start event. Purge completed work from the main thread queue, and perform work if no threads at all.
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

    //
    // slot management
    //
    int GetFreeBuildSlot();
    void FreeBuildSlot(VoxelBuildSlot* slot);

    //
    // job management
    //
    void ProcessJob(VoxelJob* job);
    bool RunJobs();
    bool UploadCompletedWork();
    VoxelJob* GetJob();

    // 
    // state
    //
    Vector<VoxelJob*> completedJobs_;
    Vector<VoxelBuildSlot> slots_;
    SharedPtr<WorkQueue> workQueue_;
    Mutex completedWorkMutex_;

    // completed work 
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
