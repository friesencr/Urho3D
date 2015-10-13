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
const int VOXEL_BUILD_STATE_COMPLETE = 4;
const int VOXEL_BUILD_STATE_FAIL = 5;

typedef void(*VoxelProcessorFunc)(VoxelData* source, VoxelData* dest, const VoxelRangeFragment& range);

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
    WeakPtr<VoxelMap> voxelMap;
    VoxelBuildSlot* slot;
    VoxelBuilder* builder;
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
    void BuildVoxelChunk(VoxelChunk* chunk, bool async = false);
    // needs to be public for work item work method
    void CompleteWork(unsigned = M_MAX_UNSIGNED);
    //void CancelJob(VoxelJob* job);
    void RegisterProcessor(String name, VoxelProcessorFunc function);
    bool UnregisterProcessor(String name);
    bool BuildWorkload(VoxelJob* job, unsigned slotIndex, bool async);

private:
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();
    void FreeJob(VoxelJob* job);
    bool RunVoxelProcessor(VoxelBuildSlot* slot);
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    void FreeBuildSlot(VoxelBuildSlot* slot);
    void ProcessJob(VoxelJob* job);
    bool UploadCompletedWork();
    bool UploadJob(VoxelJob* job);

    Vector<VoxelJob*> completedJobs_;
    Vector<VoxelBuildSlot> slots_;
    SharedPtr<WorkQueue> workQueue_;
    Mutex completedWorkMutex_;
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
