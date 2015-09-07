#pragma once

#include "../Resource/Resource.h"

#include "VoxelUtils.h"
#include "VoxelData.h"
#include "VoxelWriter.h"
#include "VoxelMap.h"


//void ApplyVoxelBrushStroke(Urho3D::VoxelMap* voxelMap, const Urho3D::VoxelRangeFragment& range, Urho3D::VoxelProcessorWriters writers)
//{
//
//}

namespace Urho3D
{

class VoxelMap;
class VoxelBrush : public VoxelData, public Resource
{
    OBJECT(VoxelBrush);
public:
    VoxelBrush(Context* context) : Resource(context_) {}
    virtual unsigned GetPadding() const { return 0; }
};

void ApplyVoxelBrushStroke(VoxelMap& voxelMap, const VoxelBrush& brush, const VoxelRange& range);

}
