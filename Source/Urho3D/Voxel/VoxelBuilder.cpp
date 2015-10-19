#include "../IO/Log.h"
#include "../Core/Profiler.h"
#include "../Resource/ResourceCache.h"
#include "../Core/CoreEvents.h"

#include "VoxelWriter.h"
#include "VoxelBuilder.h"

#include "../DebugNew.h"

#include <stdio.h>

namespace Urho3D
{
  
static const unsigned SLOTS_PER_THREAD = 1;

static void VoxelBuildWorkHandler(const WorkItem* workItem, unsigned threadIndex)
{
    VoxelJob* job = (VoxelJob*)workItem->aux_;
    job->builder->BuildWorkload(job, threadIndex + 1, true); // reserve slot 0 for non async workloads
}

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_BEGINFRAME, HANDLER(VoxelBuilder, HandleBeginFrame));
}

VoxelBuilder::~VoxelBuilder()
{

}


void VoxelBuilder::AllocateWorkerBuffers()
{
    unsigned numWorkers = GetNumLogicalCPUs();
    unsigned numSlots = numWorkers * SLOTS_PER_THREAD + 2;
    if (numSlots == slots_.Size())
        return;

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
    while (!workQueue_->IsCompleted(0) && UploadCompletedWork())
        Time::Sleep(0);
}

void VoxelBuilder::BuildVoxelChunk(VoxelChunk* chunk, bool async)
{
    if (!chunk)
    {
        LOGERROR("Cannot build null chunk.");
        return;
    }

    if (!slots_.Size())
        AllocateWorkerBuffers();

    chunk->SetNumberOfMeshes(1);
    VoxelJob* job = new VoxelJob();
    chunk->voxelJob_ = job;
    job->chunk = chunk;
    job->slot = 0;
    job->builder = this;
    job->voxelMap = chunk->GetVoxelMap(false);

    if (!job->voxelMap)
    {
        LOGERROR("Cannot build chunk.  Missing voxel map.");
        return;
    }

    //job->backend = TransvoxelMeshBuilder::GetTypeStatic();
    chunk->meshBuilder_ = (VoxelMeshBuilder*)GetSubsystem(STBMeshBuilder::GetTypeStatic());
    if (!chunk->meshBuilder_)
    {
        LOGERROR("Cannot build chunk.  Missing mesh builder.");
        return;
    }

    //async = false;
    if (async) {
        workQueue_->Resume();
        SharedPtr<WorkItem> workItem(new WorkItem());
        workItem->workFunction_ = VoxelBuildWorkHandler;
        workItem->aux_ = job;
        workQueue_->AddWorkItem(workItem);
    }
    else
        BuildWorkload(job, 0, false) && UploadJob(job);
}

void VoxelBuilder::FreeJob(VoxelJob* job)
{
    delete job;
}

bool VoxelBuilder::UploadJob(VoxelJob* job)
{
    VoxelMeshBuilder* backend = job->chunk->meshBuilder_;
    bool success = backend->UploadGpuData(job);
    if (success)
        job->chunk->OnVoxelChunkCreated();
    else
        LOGERROR("Failed to upload data to gpu");

    FreeJob(job);
    return success;
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
            UploadJob(job);
        }
        else
            break;
    }
    return true;
}

void VoxelBuilder::ProcessJob(VoxelJob* job)
{
    VoxelChunk* chunk = job->chunk;
    chunk->SetNumberOfMeshes(1);
}

bool VoxelBuilder::RunVoxelProcessor(VoxelBuildSlot* slot)
{
#if 1
    VoxelMap* voxelMap = slot->job->voxelMap;
    VoxelChunk* voxelChunk = slot->job->chunk;

    if (voxelChunk->GetVoxelProcessors().Size() > 0 && voxelChunk->processorDataMask_)
    {
        PODVector<unsigned char>* datas[VOXEL_DATA_NUM_BASIC_STREAMS];
        voxelMap->GetBasicDataArrays(datas);

        PODVector<unsigned char>* writerDatas[VOXEL_DATA_NUM_BASIC_STREAMS];
        slot->writer.GetBasicDataArrays(writerDatas);
        slot->writer.SetDataMask(voxelChunk->GetProcessorDataMask());
        slot->writer.SetPadding(voxelMap->GetPadding());
        slot->writer.SetSize(voxelMap->width_, voxelMap->height_, voxelMap->depth_);

        for (unsigned i = 0; i < VOXEL_DATA_NUM_BASIC_STREAMS; ++i)
        {
            if (!((1 << i) & voxelChunk->processorDataMask_))
                continue;

            //unsigned char* source = &datas[i]->Front();
            unsigned char* dest = &writerDatas[i]->Front();
            memset(dest, 0, voxelMap->size_);
        }

        // corrects the voxel processor to tell it to bleed outside the range
        VoxelRangeFragment processRange;
        processRange.startX = -1;
        processRange.startY = -1;
        processRange.startZ = -1;
        processRange.endX = VOXEL_CHUNK_SIZE_X + 1;
        processRange.endY = VOXEL_CHUNK_SIZE_Y + 1;
        processRange.endZ = VOXEL_CHUNK_SIZE_Z + 1;

        PODVector<StringHash> mapProcessors = voxelChunk->GetVoxelProcessors();
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

bool VoxelBuilder::BuildWorkload(VoxelJob* job, unsigned slotIndex, bool async)
{
    VoxelBuildSlot* slot = &slots_[slotIndex];
    slot->state = VOXEL_BUILD_STATE_STARTED;
    slot->job = job;
    job->slot = slot;

    VoxelMeshBuilder* backend = job->chunk->meshBuilder_;
    VoxelChunk* chunk = job->chunk;
    VoxelMap* voxelMap = job->voxelMap;

    bool success = voxelMap->IsFilled() || chunk->FillVoxelMap(voxelMap);
    if (success)
    {
        backend->AssignWork(slot);
        success = RunVoxelProcessor(slot) && backend->BuildMesh(slot) && backend->ProcessMesh(slot);
    }

    backend->FreeWork(slot);

    if (!success)
        FreeJob(job);
    else if (async)
    {
        completedWorkMutex_.Acquire();
        completedJobs_.Push(job);
        completedWorkMutex_.Release();
    }
    FreeBuildSlot(slot);
    return success;
}

void VoxelBuilder::FreeBuildSlot(VoxelBuildSlot* slot)
{
    slot->job = 0;
    slot->state = VOXEL_BUILD_STATE_FREE;
}

void VoxelBuilder::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    if (workQueue_)
    {
        UploadCompletedWork();
        if (workQueue_->IsCompleted(0))
            workQueue_->Pause();
    }
}

}
