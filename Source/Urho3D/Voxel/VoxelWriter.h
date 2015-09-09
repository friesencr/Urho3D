#pragma once

#include "VoxelData.h"
#include "VoxelUtils.h"

namespace Urho3D 
{
class VoxelBuilder;
class VoxelWriter : public VoxelData
{
friend class VoxelBuilder;
public:
    VoxelWriter();
    ~VoxelWriter();
    void SetPadding(const unsigned padding);
    virtual unsigned GetPadding() const;

private:
    unsigned padding_;

};

}
