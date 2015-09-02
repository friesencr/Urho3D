#include "VoxelWriter.h"

#include "../DebugNew.h"

namespace Urho3D
{

VoxelWriter::VoxelWriter() : VoxelData(),
    padding_(0)
{

}

VoxelWriter::~VoxelWriter()
{

}

unsigned VoxelWriter::GetPadding() const {
    return padding_;
}

void VoxelWriter::SetPadding(const unsigned padding)
{
    padding_ = padding;
    SetSize(width_, height_, depth_);
}

}
