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
struct VoxelBuildSlot;

typedef void(*VoxelProcessorFunc)(VoxelData* source, VoxelData* dest, const VoxelRangeFragment& range);

struct VoxelJob {
    SharedPtr<VoxelChunk> chunk;
    SharedPtr<VoxelMap> voxelMap;
    unsigned slot;
};

struct VoxelWorkload 
{
    int slot;
    VoxelBuilder* builder;
    SharedPtr<WorkItem> workItem;
    VoxelRangeFragment range;
    unsigned workloadIndex;
    int numQuads;
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
    bool UploadGpuData(VoxelJob* job);
    bool UploadGpuDataCompatibilityMode(VoxelBuildSlot* slot, bool append = false);
    bool UpdateMaterialParameters(Material* material, bool setColor);

    //
    // slot management
    //
    int GetFreeBuildSlot();
    unsigned DecrementBuildSlot(VoxelBuildSlot* slot);
    void PrepareCompletedWork(VoxelBuildSlot* slot);
    void FreeBuildSlot(VoxelBuildSlot* slot);

    //
    // job management
    //
    void ProcessJob(VoxelJob* job);
    VoxelJob* CreateJob(VoxelChunk* chunk);
    bool RunJobs();
    bool IsBuilding();
    bool UploadCompletedWork();

    // 
    // state
    //
    Vector<VoxelJob*> buildJobs_;
    Vector<VoxelJob*> uploadJobs_;
    SharedPtr<IndexBuffer> sharedIndexBuffer_;
    Vector<VoxelBuildSlot> slots_;
    Vector<Variant> transform_;
    Vector<Variant> normals_;
    Vector<Variant> ambientTable_;
    Vector<Variant> texscaleTable_;
    Vector<Variant> texgenTable_;
    Vector<Variant> defaultColorTable_;

    // completed work 
    Mutex slotMutex_;
    Mutex uploadJobMutex_;
    HashMap<StringHash, VoxelProcessorFunc> processors_;
};

}
