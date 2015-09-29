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

struct VoxelBuildSlot
{
    unsigned index;
    VoxelJob* job;
    VoxelBuilder* builder;
    bool failed;
    bool upload;
    bool free;
    VoxelMeshBuilder* backend;
    SharedPtr<WorkItem> workItem;
    VoxelWriter writer;
};

class URHO3D_API VoxelBuilder : public Object {
    OBJECT(VoxelBuilder);

public:
    VoxelBuilder(Context* context);
    ~VoxelBuilder();
    VoxelJob* BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, bool async = false);
    // needs to be public for work item work method
    void BuildWorkload(VoxelBuildSlot* slot);
    void CompleteWork(unsigned = M_MAX_UNSIGNED);
    void CancelJob(VoxelJob* job);
    void RegisterProcessor(String name, VoxelProcessorFunc function);
    bool UnregisterProcessor(String name);

private:
    void AllocateWorkerBuffers();
    void FreeWorkerBuffers();
    bool RunVoxelProcessor(VoxelBuildSlot* slot);

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
    bool IsBuilding();
    bool UploadCompletedWork();

    // 
    // state
    //
    Vector<VoxelJob*> buildJobs_;
    Vector<VoxelBuildSlot> slots_;

    // completed work 
    Mutex slotMutex_;
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
