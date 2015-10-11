#include <SDL/SDL_atomic.h>

#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../Resource/ResourceCache.h"
#include "../Core/Atomic.h"

#include "VoxelWriter.h"
#include "VoxelBuilder.h"

#include "../DebugNew.h"

namespace Urho3D
{
  
static const unsigned SLOTS_PER_THREAD = 1;

static void VoxelBuildWorkHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelJob* job = (VoxelJob*)workItem->aux_;
    job->builder->BuildWorkload(job, threadIndex);
}

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context)
{

}

VoxelBuilder::~VoxelBuilder()
{

}


void VoxelBuilder::AllocateWorkerBuffers()
{
    unsigned numWorkers = 8;
    unsigned numSlots = numWorkers * SLOTS_PER_THREAD + 1;
    if (numSlots == slots_.Size())
        return;


    //workerSempaphore_ = SDL_CreateSemaphore(0);

    slots_.Resize(numSlots);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        FreeBuildSlot(&slots_[i]);
        slots_[i].index = i;
    }

    workQueue_ = new WorkQueue(context_);
    workQueue_->CreateThreads(numWorkers);
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
    while (!workQueue_->IsCompleted(0) && UploadCompletedWork());
}

VoxelJob* VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, bool async)
{
    if (!chunk)
        return 0;

    if (!slots_.Size())
        AllocateWorkerBuffers();

    VoxelJob* job = new VoxelJob();
    chunk->voxelJob_ = job;
    job->chunk = chunk;
    job->slot = 0;
    job->builder = this;
    //job->backend = TransvoxelMeshBuilder::GetTypeStatic();
    job->backend = STBMeshBuilder::GetTypeStatic();
    ProcessJob(job);

    SharedPtr<WorkItem> workItem(new WorkItem());
    workItem->workFunction_ = VoxelBuildWorkHandler;
    workItem->aux_ = job;
    workQueue_->AddWorkItem(workItem);

    if (!async)
        CompleteWork();

    return job;
}

bool VoxelBuilder::UploadCompletedWork()
{
    for (;;)
    {
        if (completedJobs_.Size())
        {
            VoxelJob* job = 0;
            job = completedJobs_[0];
            completedWorkMutex_.Acquire();
            completedJobs_.Erase(0);
            completedWorkMutex_.Release();

            VoxelMeshBuilder* backend = (VoxelMeshBuilder*)GetSubsystem(job->backend);
            if (!backend->UploadGpuData(job))
                LOGERROR("Failed to upload data to gpu");
            else
                job->chunk->OnVoxelChunkCreated();

            delete job;
        }
        else
            break;
    }

    return true;
}

void VoxelBuilder::ProcessJob(VoxelJob* job)
{
    PROFILE(BuildChunk);
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

void VoxelBuilder::BuildWorkload(VoxelJob* job, unsigned slotIndex)
{
    VoxelBuildSlot* slot = &slots_[slotIndex];
    slot->state = VOXEL_BUILD_STATE_STARTED;
    slot->job = job;
    job->slot = slot;

    VoxelMeshBuilder* backend = (VoxelMeshBuilder*)GetSubsystem(job->backend);
    bool success = false;
    if (backend)
    {
        SharedPtr<VoxelMap> voxelMap(job->chunk->GetVoxelMap());
        job->voxelMap = voxelMap;
        if (voxelMap)
        {
            backend->AssignWork(slot);
            success = RunVoxelProcessor(slot) && backend->BuildMesh(slot) && backend->ProcessMesh(slot);
        }
        backend->FreeWork(slot);
    }
    if (success)
    {
        completedWorkMutex_.Acquire();
        completedJobs_.Push(job);
        completedWorkMutex_.Release();
    }
    else
        delete job;

    FreeBuildSlot(slot);
}

//void VoxelBuilder::CancelJob(VoxelJob* job)
//{
//    for (unsigned i = 0; i < jobCount_; ++i)
//    {
//        if (buildJobs_[i] == job)
//        {
//            buildJobs_[i] = 0;
//            delete job;
//            return;
//        }
//    }
//}

void VoxelBuilder::FreeBuildSlot(VoxelBuildSlot* slot)
{
    slot->job = 0;
    slot->state = VOXEL_BUILD_STATE_FREE;
}

int VoxelBuilder::GetFreeBuildSlot()
{
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        if (slots_[i].state == VOXEL_BUILD_STATE_FREE)
        {
            slots_[i].state = VOXEL_BUILD_STATE_READY;
            return i;
        }
    }
    return -1;
}

void VoxelBuilder::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    CompleteWork();
}

}
