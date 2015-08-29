#pragma once

#include <Urho3D/Graphics/Voxel.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Voxel/Voxel.h>

using namespace Urho3D;

class WorldBuilder : public Object
{
    OBJECT(WorldBuilder);

    int height_;
    int width_;
    int depth_;
    SharedPtr<VoxelSet> voxelSet_;
    SharedPtr<VoxelBlocktypeMap> voxelBlocktypeMap_;
    SharedPtr<VoxelStore> voxelStore_;

public:

    WorldBuilder(Context*);
    ~WorldBuilder();
    void SetVoxelSet(VoxelSet* voxelSet) { voxelSet_ = voxelSet; }
    void ConfigureParameters();
    void CreateWorld();
    void LoadWorld();
    void SaveWorld();
    void BuildWorld();
    void SetSize(unsigned width, unsigned depth) { height_ = 1; width_ = width; depth_ = depth; }
};
