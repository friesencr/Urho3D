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
    unsigned numSlots = 12;
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
    for (;;)
    {
        bool runJobs = RunJobs();
        bool upload = UploadCompletedWork();
        if (!(runJobs || upload || IsBuilding()))
            return;

        Time::Sleep(0);
    }
}

void BuildWorkloadHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelBuildSlot* slot = (VoxelBuildSlot*)workItem->aux_;
    slot->builder->BuildWorkload(slot);
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
    job->backend = TransvoxelMeshBuilder::GetTypeStatic();
    //job->backend = STBMeshBuilder::GetTypeStatic();
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
        else
            break;
    }
    return buildJobs_.Size() > 0;
}

bool VoxelBuilder::UploadCompletedWork()
{
    for (;;)
    {
        VoxelBuildSlot* slot = 0;
        {
            MutexLock lock(slotMutex_);
            for (unsigned i = 0; i < slots_.Size(); ++i)
            {
                if (slots_[i].upload == true)
                {
                    slot = &slots_[i];
                    break;
                }
            }
            if (!slot)
                return false;
        }

        VoxelMeshBuilder* backend = (VoxelMeshBuilder*)slot->backend;
        if (!backend->UploadGpuData(slot))
            LOGERROR("Failed to upload data to gpu");
        else
            slot->job->chunk->OnVoxelChunkCreated();

        backend->FreeWork(slot);
        delete slot->job;
        FreeBuildSlot(slot);
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

#if 0
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

    slot->builder = this;

    SharedPtr<WorkItem> workItem(new WorkItem());
    workItem->priority_ = 0;
    workItem->aux_ = slot;
    workItem->workFunction_ = BuildWorkloadHandler;
    slot->workItem = workItem;
    workQueue->AddWorkItem(workItem);
}

bool VoxelBuilder::RunVoxelProcessor(VoxelBuildSlot* slot)
{
    return true;

#if 0
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
#endif
}

void VoxelBuilder::BuildWorkload(VoxelBuildSlot* slot)
{
    VoxelMeshBuilder* builder = slot->backend;
    if (!(RunVoxelProcessor(slot) && builder->BuildMesh(slot) && builder->ProcessMesh(slot)))
    {
        slot->failed = true;
        slot->backend->FreeWork(slot);
        FreeBuildSlot(slot);
    }
    else
    {
        MutexLock lock(slotMutex_);
        slot->upload = true;
    }
}

void VoxelBuilder::CancelJob(VoxelJob* job)
{
    if (buildJobs_.Remove(job))
        delete job;
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
    slot->free = true;
    slot->upload = false;
    slot->job = 0;
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
