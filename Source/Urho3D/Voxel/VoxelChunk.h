#pragma once

#include "VoxelSet.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"
#include "../Container/Ptr.h"
#include "../Core//Mutex.h"

namespace Urho3D {

class URHO3D_API VoxelChunk : public Drawable
{
    OBJECT(VoxelChunk);
    friend class VoxelSet;

public:
    VoxelChunk(Context* context);
    ~VoxelChunk();

    static void RegisterObject(Context* context);

    Geometry* GetGeometry() const;
    VertexBuffer* GetVertexBuffer() const;

    int GetIndexX() const;
    int GetIndexY() const;
    int GetIndexZ() const;
    int GetSizeX() const;
    int GetSizeY() const;
    int GetSizeZ() const;

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();

private:
    void SetIndex(int x, int y, int z);
    void SetSize(int x, int y, int z);
	int DecrementWorker();
	void InitializeBuffer();
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
	float transform_[3][3];
	float bounds_[2][3];
	float priority_;
	int numQuads_;
	static const SharedPtr<IndexBuffer> sharedIndexBuffer;
};

}
