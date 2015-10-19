#pragma once

#include "../Core/Object.h"
#include "../Resource/Resource.h"

#include "VoxelData.h"
#include "VoxelBlocktypeMap.h"
#include "VoxelTextureMap.h"
#include "VoxelOverlayMap.h"
#include "VoxelColorPalette.h"
#include "VoxelBuilder.h"

namespace Urho3D 
{
class URHO3D_API VoxelMap : public Resource, public VoxelData
{
    OBJECT(VoxelMap);
friend class VoxelBuilder;
public:
    SharedPtr<VoxelBlocktypeMap> blocktypeMap;

    unsigned char dataSourceType;

    /// Construct empty.
    VoxelMap(Context* context);

    /// Destruct.
    virtual ~VoxelMap();

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);

    virtual bool EndLoad();

    /// Saves voxel map information.
    virtual bool Save(Serializer& dest);

    /// Sets the blocktype map attribute
    void SetBlocktypeMap(VoxelBlocktypeMap*);
    
    /// Sets the blocktype map attribute
    void SetBlocktypeMapAttr(const ResourceRef&);

    /// Gets the blocktype map attribute
    ResourceRef GetBlocktypeMapAttr() const;

    virtual inline unsigned GetPadding() const { return 2; }

    void TransferAdjacentData(VoxelMap* north, VoxelMap* south, VoxelMap* east, VoxelMap* west);
    void TransferAdjacentNorthData(VoxelMap* source);
    void TransferAdjacentSouthData(VoxelMap* source);
    void TransferAdjacentEastData(VoxelMap* source);
    void TransferAdjacentWestData(VoxelMap* source);
    void TransferAdjacentDataDirection(VoxelMap* source, int direction);

    bool IsFilled() const { return size_ > 0; }

private:
    String loadVoxelBlocktypeMap_;
};

}
