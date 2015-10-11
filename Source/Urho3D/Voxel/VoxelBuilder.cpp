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

/// Worker thread managed by the work queue.
/// Construct.
VoxelWorker::VoxelWorker(VoxelBuilder* voxelBuilder, unsigned index) :
        voxelBuilder_(voxelBuilder),
        index_(index)
{

}

void VoxelWorker::ThreadFunction()
{
    // Init FPU state first
    InitFPU();
    voxelBuilder_->BuildWorkload(index_);
}

VoxelBuilder::VoxelBuilder(Context* context)
    : Object(context),
    jobCount_(0),
    assignedJobCount_(0),
    builtJobCount_(0),
    uploadedJobCount_(0),
    shutdown_(false)
{

}

VoxelBuilder::~VoxelBuilder()
{
    shutdown_ = true;

    for (unsigned i = 0; i < workers_.Size(); ++i)
        workers_[i]->Stop();

    for (unsigned i = 0; i < buildJobs_.Size(); ++i)
        delete buildJobs_[i];
}

void VoxelBuilder::AllocateWorkerBuffers()
{
    unsigned numWorkers = 0;
    unsigned numSlots = numWorkers * SLOTS_PER_THREAD;
    if (numSlots == slots_.Size())
        return;


    //workerSempaphore_ = SDL_CreateSemaphore(0);

    slots_.Resize(numSlots);
    for (unsigned i = 0; i < slots_.Size(); ++i)
    {
        FreeBuildSlot(&slots_[i]);
        slots_[i].index = i;
    }

    workers_.Resize(numWorkers);
    for (unsigned i = 0; i < workers_.Size(); ++i)
    {
        workers_[i] = new VoxelWorker(this, i);
        workers_[i]->Run();
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
    while(jobCount_ != uploadedJobCount_)
    {
        UploadCompletedWork();
        Time::Sleep(0);
    }
    jobCount_ = 0;
    assignedJobCount_ = 0;
    builtJobCount_ = 0;
    uploadedJobCount_ = 0;
    buildJobs_.Clear();
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
    //job->backend = TransvoxelMeshBuilder::GetTypeStatic();
    job->backend = STBMeshBuilder::GetTypeStatic();
    BASE_MEMORYBARRIER_ACQUIRE();
    buildJobs_.Push(job);
    jobCount_++;
    //SDL_SemPost(workerSempaphore_);
    BASE_MEMORYBARRIER_RELEASE();
    if (!async)
        CompleteWork();

    return job;
}

VoxelJob* VoxelBuilder::GetJob()
{
    unsigned originalIndex = assignedJobCount_;
    if (jobCount_ > assignedJobCount_)
    {
        unsigned index = AtomicCompareAndSwap((volatile unsigned*)&assignedJobCount_, assignedJobCount_ + 1, originalIndex);
        if (index == originalIndex)
        {
            VoxelJob* job = buildJobs_[index];
            return job;
        }
    }
    return 0;
}

bool VoxelBuilder::RunJobs()
{
    while(buildJobs_.Size() > 0)
    {
        int slotId = GetFreeBuildSlot();
        if (slotId != -1)
        {
            BASE_MEMORYBARRIER_ACQUIRE();
            unsigned count = buildJobs_.Size();
            VoxelJob* job = buildJobs_[0];
            VoxelBuildSlot* slot = &slots_[slotId];
            job->slot = slot;
            slot->job = job;
            VoxelMeshBuilder* builder = (VoxelMeshBuilder*)GetSubsystem(job->backend);
            buildJobs_.Erase(0);
            if (builder)
            {
                builder->AssignWork(slot);
                ProcessJob(job);
            }
            BASE_MEMORYBARRIER_RELEASE();
        }
        else
            break;
    }
    return buildJobs_.Size() > 0;
}

bool VoxelBuilder::UploadCompletedWork()
{
    if (builtJobCount_ == uploadedJobCount_)
        return false;

    VoxelBuildSlot* slot = 0;
    {
        for (unsigned i = 0; i < slots_.Size(); ++i)
        {
            if (slots_[i].state == VOXEL_BUILD_STATE_UPLOAD)
            {
                slot = &slots_[i];
                break;
            }
        }
        if (!slot)
            return false;
    }

    VoxelMeshBuilder* backend = (VoxelMeshBuilder*)GetSubsystem(slot->job->backend);
    if (!backend->UploadGpuData(slot))
        LOGERROR("Failed to upload data to gpu");
    else
        slot->job->chunk->OnVoxelChunkCreated();

    backend->FreeWork(slot);
    delete slot->job;
    FreeBuildSlot(slot);
    AtomicAdd(&uploadedJobCount_, 1);

    return builtJobCount_ != uploadedJobCount_;
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

void VoxelBuilder::BuildWorkload(unsigned slotIndex)
{
    for (;;)
    {
        if (shutdown_)
            return;

        //SDL_SemWait(workerSempaphore_);
        if (jobCount_ != assignedJobCount_)
        {
            bool foundWork = false;
            for (unsigned i = 0; i < SLOTS_PER_THREAD; ++i)
            {
                VoxelBuildSlot* slot = &slots_[slotIndex * SLOTS_PER_THREAD + i];
                if (slot->state == VOXEL_BUILD_STATE_FREE)
                {
                    VoxelJob* job = GetJob();
                    if (job)
                    {
                        foundWork = true;
                        slot->state = VOXEL_BUILD_STATE_STARTED;
                        slot->job = job;
                        job->slot = slot;

                        VoxelMeshBuilder* builder = (VoxelMeshBuilder*)GetSubsystem(slot->job->backend);
                        SharedPtr<VoxelMap> voxelMap(slot->job->chunk->GetVoxelMap());
                        slot->job->voxelMap = voxelMap;
                        if (builder && voxelMap && RunVoxelProcessor(slot) && builder->BuildMesh(slot) && builder->ProcessMesh(slot))
                        {
                            slot->state = VOXEL_BUILD_STATE_UPLOAD;
                            AtomicAdd(&builtJobCount_, 1);
                        }
                        else
                        {
                            slot->state = VOXEL_BUILD_STATE_FAIL;
                            builder->FreeWork(slot);
                            FreeBuildSlot(slot);
                        }
                    }
                }
            }
            if (!foundWork)
                Time::Sleep(0);
        }
        else
            Time::Sleep(0);
    }
}

void VoxelBuilder::CancelJob(VoxelJob* job)
{
    for (unsigned i = 0; i < jobCount_; ++i)
    {
        if (buildJobs_[i] == job)
        {
            buildJobs_[i] = 0;
            delete job;
            return;
        }
    }
}

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
