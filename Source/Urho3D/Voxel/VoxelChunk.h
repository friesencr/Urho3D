#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"
#include "../Container/Ptr.h"

namespace Urho3D {

class VoxelChunk : public Drawable
{
    OBJECT(VoxelChunk);

public:
    VoxelChunk(Context* context);
    ~VoxelChunk();

    void SetIndex(int x, int y, int z);
    void SetSize(int x, int y, int z);

    Geometry* GetGeometry() const;
    VertexBuffer* GetVertexBuffer() const;

    int GetIndexX() const;
    int GetIndexY() const;
    int GetIndexZ() const;
    int GetSizeX() const;
    int GetSizeY() const;
    int GetSizeZ() const;
private:
    // Voxel chunk geometry
    SharedPtr<Geometry> geometry_;
    // Vertex data
    SharedPtr<VertexBuffer> vertexBuffer_;
    // X Index
    int indexX_;
    // Y Index
    int indexY_;
    // Z Index
    int indexZ_;
    // X Index
    int sizeX_;
    // Y Index
    int sizeY_;
    // Z Index
    int sizeZ_;
};

}
