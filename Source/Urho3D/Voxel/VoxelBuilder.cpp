#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../Resource/ResourceCache.h"

#include "VoxelWriter.h"
#include "VoxelBuilder.h"

#include "../DebugNew.h"

namespace Urho3D
{

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context)
{

}

VoxelBuilder::~VoxelBuilder()
{
    for (unsigned i = 0; i < buildJobs_.Size(); ++i)
        delete buildJobs_[i];
}

void VoxelBuilder::AllocateWorkerBuffers()
{
    unsigned numSlots = 2;
    if (numSlots == slots_.Size())
        return;

    slots_.Resize(numSlots);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        FreeBuildSlot(&slots_[i]);
        slots_[i].index = i;
    }
}

void VoxelBuilder::RegisterProcessor(String name, VoxelProcessorFunc func)
{
    processors_[name] = func;
}

bool VoxelBuilder::UnregisterProcessor(String name)
{
    return processors_.Erase(name);
}

void VoxelBuilder::FreeWorkerBuffers()
{
    slots_.Clear();
}

void VoxelBuilder::CompleteWork(unsigned priority)
{
    //PROFILE(VoxelWork);
    while (RunJobs() || UploadCompletedWork() || IsBuilding())
        Time::Sleep(0);
}

void BuildWorkloadHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelWorkload* workload = (VoxelWorkload*)workItem->aux_;
    workload->builder->BuildWorkload(workload);
}

VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, VoxelMap* voxelMap, bool async)
{
    if (!chunk)
        return 0;

    if (!voxelMap)
        return 0;

    if (!slots_.Size())
        AllocateWorkerBuffers();

    VoxelJob* job = new VoxelJob();
    job->chunk = chunk;
    chunk->voxelJob_ = job;
    job->slot = 0;
    job->voxelMap = voxelMap;
    job->backend = STBMeshBuilder::GetBaseTypeStatic();
    buildJobs_.Push(job);

    RunJobs();
    if (!async)
        CompleteWork();

    return job;
}

bool VoxelBuilder::RunJobs()
{
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (buildJobs_.Size() > 0)
        {
            int slotId = GetFreeBuildSlot();
            if (slotId != -1)
            {
                unsigned count = buildJobs_.Size();
                VoxelJob* job = buildJobs_[0];
                VoxelBuildSlot* slot = &slots_[slotId];
                job->slot = slot;
                slot->job = job;
                slot->backend = (VoxelMeshBuilder*)GetSubsystem(job->backend);
                slot->backend->AssignWork(slot);
                buildJobs_.Erase(0);
                ProcessJob(job);
                break;
            }
        }
    }
    return buildJobs_.Size() > 0;
}

bool VoxelBuilder::UploadCompletedWork()
{
    MutexLock lock(uploadJobMutex_);
    for (;;)
    {
        if (!uploadJobs_.Size())
            return false;

        VoxelJob* job = uploadJobs_[0];
        VoxelMeshBuilder* backend = (VoxelMeshBuilder*)GetSubsystem(job->backend);
        uploadJobs_.Erase(0);
        if (!backend->UploadGpuData(job))
            LOGERROR("Failed to upload data to gpu");
        else
            job->chunk->OnVoxelChunkCreated();

        delete job;
    }
    return false;
}

void VoxelBuilder::ProcessJob(VoxelJob* job)
{
    PROFILE(BuildChunk);
    WorkQueue* workQueue = GetSubsystem<WorkQueue>();
    VoxelBuildSlot* slot = job->slot;
    VoxelMap* voxelMap = job->voxelMap;
    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);

#if 1
    if (voxelMap->GetVoxelProcessors().Size() > 0 && voxelMap->processorDataMask_)
    {
        PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
        voxelMap->GetBasicDataArrays(datas);

        PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
        slot->writer.GetBasicDataArrays(writerDatas);
        slot->writer.SetDataMask(voxelMap->GetProcessorDataMask());
        slot->writer.SetPadding(voxelMap->GetPadding());
        slot->writer.SetSize(voxelMap->width_, voxelMap->height_, voxelMap->depth_);

        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (!((1 << i) & voxelMap->processorDataMask_))
                continue;

            //unsigned char* source = &datas[i]->Front();
            unsigned char* dest = &writerDatas[i]->Front();
            memset(dest, 0, voxelMap->size_);
        }
    }
#endif

    unsigned index = 0;
    slot->numWorkloads = 4;
    slot->workCounter = slot->numWorkloads;
    for (int x = 0; x < 2; ++x)
    {
        for (int z = 0; z < 2; ++z)
        {
            VoxelWorkload* workload = &slot->workloads[index];
            workload->builder = this;
            workload->slot = job->slot;
            workload->range.positionX = 0;
            workload->range.positionY = 0;
            workload->range.positionZ = 0;
            workload->range.lengthX = VOXEL_WORKER_SIZE_X;
            workload->range.lengthY = VOXEL_WORKER_SIZE_Y;
            workload->range.lengthZ = VOXEL_WORKER_SIZE_Z;
            workload->range.startX = x * VOXEL_WORKER_SIZE_X;
            workload->range.startY = 0;
            workload->range.startZ = z * VOXEL_WORKER_SIZE_Z;
            workload->range.endX = VOXEL_WORKER_SIZE_X * (x + 1);
            workload->range.endY = VOXEL_WORKER_SIZE_Y;
            workload->range.endZ = VOXEL_WORKER_SIZE_Z * (z + 1);
            workload->index = index++;

            SharedPtr<WorkItem> workItem(new WorkItem());
            workItem->priority_ = 0;
            workItem->aux_ = workload;
            workItem->start_ = slot;
            workItem->workFunction_ = BuildWorkloadHandler;
            workload->workItem = workItem;
            workQueue->AddWorkItem(workItem);
        }
    }
}

bool VoxelBuilder::RunVoxelProcessor(VoxelWorkload* workload)
{
    VoxelBuildSlot* slot = workload->slot;
    VoxelMap* voxelMap = slot->job->voxelMap;
    VoxelRangeFragment processRange = workload->range;
    if (voxelMap->GetVoxelProcessors().Size() > 0 && voxelMap->GetProcessorDataMask())
    {
        PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
        slot->writer.GetBasicDataArrays(writerDatas);

        // corrects the voxel processor to tell it to bleed outside the range
        processRange.startX--;
        processRange.startY--;
        processRange.startZ--;
        processRange.endX++;
        processRange.endY++;
        processRange.endZ++;

        PODVector<StringHash> mapProcessors = voxelMap->GetVoxelProcessors();
        for (unsigned p = 0; p < mapProcessors.Size(); ++p)
        {
            if (processors_.Contains(mapProcessors[p]))
            {
                VoxelProcessorFunc func = processors_[mapProcessors[p]];
                func(voxelMap, &slot->writer, processRange);
            }
        }
    }
    return true;
}

void VoxelBuilder::BuildWorkload(VoxelWorkload* workload)
{
    VoxelBuildSlot* slot = workload->slot;
    VoxelMeshBuilder* builder = slot->backend;

    if (!(builder->BuildMesh(workload) && RunVoxelProcessor(workload) && builder->ProcessMeshFragment(workload)))
    {
        MutexLock lock(slot->slotMutex);
        slot->failed = true;
    }

    if (DecrementBuildSlot(slot) == 0)
    {
        slot->failed = builder->ProcessMesh(slot);
        {
            MutexLock lock(uploadJobMutex_);
            uploadJobs_.Push(slot->job);
        }
        slot->backend->FreeWork(slot);
        FreeBuildSlot(slot);
    }
}

void VoxelBuilder::CancelJob(VoxelJob* job)
{
    if (buildJobs_.Remove(job))
        delete job;
}

unsigned VoxelBuilder::DecrementBuildSlot(VoxelBuildSlot* slot)
{
    MutexLock lock(slot->slotMutex);
    slot->workCounter--;
    return slot->workCounter;
}

bool VoxelBuilder::IsBuilding()
{
    MutexLock lock(slotMutex_);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (!slots_[i].free)
            return true;
    }
    return false;
}

void VoxelBuilder::FreeBuildSlot(VoxelBuildSlot* slot)
{
    MutexLock lock(slotMutex_);
    slot->failed = false;
    slot->workCounter = 0;
    slot->numWorkloads = 0;
    slot->free = true;
    slot->job = 0;
    slot->workBuffer = 0;
    slot->backend = 0;
}

int VoxelBuilder::GetFreeBuildSlot()
{
    MutexLock lock(slotMutex_);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (slots_[i].free)
        {
            slots_[i].free = false;
            return i;
        }
    }
    return -1;
}

}
