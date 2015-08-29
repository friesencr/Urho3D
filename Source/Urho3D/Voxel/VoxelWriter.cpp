#include "VoxelWriter.h"

#include "../DebugNew.h"

namespace Urho3D
{

VoxelWriter::VoxelWriter() :
    size_(0),
    strideX_(0),
    strideZ_(0),
    buffer_(0)
{

}

VoxelWriter::~VoxelWriter()
{

}

unsigned VoxelWriter::GetIndex(int x, int y, int z) { return (y + 2) + ((z + 2) * strideZ_) + ((x + 2) * strideX_); }

void VoxelWriter::SetSize(unsigned width, unsigned height, unsigned depth)
{
    strideZ_ = height + 4;
    strideX_ = (height + 4) * (depth + 4);
    size_ = (width + 4)*(height + 4)*(depth + 4);
}

void VoxelWriter::SetBuffer(unsigned char* buffer)
{
    buffer_ = buffer;
}

unsigned char* VoxelWriter::GetBuffer()
{
    return buffer_;
}

void VoxelWriter::Clear(unsigned char value)
{
    if (buffer_ && size_)
        memset(buffer_, value, sizeof(unsigned char) * size_);
}

inline void VoxelWriter::Set(int x, int y, int z, unsigned char val)
{
    buffer_[GetIndex(x, y, z)] = val;
}

inline void VoxelWriter::SetColor(int x, int y, int z, unsigned char val)
{
    buffer_[GetIndex(x, y, z)] = val;
}

inline void VoxelWriter::SetBlocktype(int x, int y, int z, unsigned char val)
{
    buffer_[GetIndex(x, y, z)] = val;
}

inline void VoxelWriter::SetVheight(int x, int y, int z, VoxelHeight sw, VoxelHeight se, VoxelHeight nw, VoxelHeight ne)
{
    buffer_[GetIndex(x, y, z)] = VoxelEncodeVHeight(sw, se, nw, ne);
}

inline void VoxelWriter::SetLighting(int x, int y, int z, unsigned char val)
{
    buffer_[GetIndex(x, y, z)] = val;
}

inline void VoxelWriter::SetTex2(int x, int y, int z, unsigned char val)
{
    buffer_[GetIndex(x, y, z)] = val;
}

void VoxelWriter::SetGeometry(int x, int y, int z, VoxelGeometry geometry, VoxelRotation rotation, VoxelHeight height)
{
    buffer_[GetIndex(x, y, z)] = VoxelEncodeGeometry(geometry, rotation, height);
}

}
