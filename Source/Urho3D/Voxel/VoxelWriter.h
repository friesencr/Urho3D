#pragma once

#include "VoxelBuilder.h"
#include "VoxelUtils.h"
#include "VoxelMap.h"

namespace Urho3D 
{

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
