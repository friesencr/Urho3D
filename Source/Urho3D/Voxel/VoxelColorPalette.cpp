#include "../Core/Context.h"

#include "VoxelColorPalette.h"

#include "../DebugNew.h"

namespace Urho3D 
{
    static Vector<Variant> defaultColorTable_;
    static float URHO3D_default_palette[64][4];
    static unsigned char URHO3D_default_palette_compact[64][3] =
    {
        { 255, 255, 255 }, { 238, 238, 238 }, { 221, 221, 221 }, { 204, 204, 204 },
        { 187, 187, 187 }, { 170, 170, 170 }, { 153, 153, 153 }, { 136, 136, 136 },
        { 119, 119, 119 }, { 102, 102, 102 }, { 85, 85, 85 }, { 68, 68, 68 },
        { 51, 51, 51 }, { 34, 34, 34 }, { 17, 17, 17 }, { 0, 0, 0 },
        { 255, 240, 240 }, { 255, 220, 220 }, { 255, 160, 160 }, { 255, 32, 32 },
        { 200, 120, 160 }, { 200, 60, 150 }, { 220, 100, 130 }, { 255, 0, 128 },
        { 240, 240, 255 }, { 220, 220, 255 }, { 160, 160, 255 }, { 32, 32, 255 },
        { 120, 160, 200 }, { 60, 150, 200 }, { 100, 130, 220 }, { 0, 128, 255 },
        { 240, 255, 240 }, { 220, 255, 220 }, { 160, 255, 160 }, { 32, 255, 32 },
        { 160, 200, 120 }, { 150, 200, 60 }, { 130, 220, 100 }, { 128, 255, 0 },
        { 255, 255, 240 }, { 255, 255, 220 }, { 220, 220, 180 }, { 255, 255, 32 },
        { 200, 160, 120 }, { 200, 150, 60 }, { 220, 130, 100 }, { 255, 128, 0 },
        { 255, 240, 255 }, { 255, 220, 255 }, { 220, 180, 220 }, { 255, 32, 255 },
        { 160, 120, 200 }, { 150, 60, 200 }, { 130, 100, 220 }, { 128, 0, 255 },
        { 240, 255, 255 }, { 220, 255, 255 }, { 180, 220, 220 }, { 32, 255, 255 },
        { 120, 200, 160 }, { 60, 200, 150 }, { 100, 220, 130 }, { 0, 255, 128 },
    };

    void VoxelColorPalette::RegisterObject(Context* context)
    {
        context->RegisterFactory<VoxelColorPalette>("Voxel");

        // color table
        for (int i = 0; i < 64; ++i) {
            URHO3D_default_palette[i][0] = URHO3D_default_palette_compact[i][0] / 255.0f;
            URHO3D_default_palette[i][1] = URHO3D_default_palette_compact[i][1] / 255.0f;
            URHO3D_default_palette[i][2] = URHO3D_default_palette_compact[i][2] / 255.0f;
            URHO3D_default_palette[i][3] = 1.0f;
        }

        defaultColorTable_.Resize(64);
        for (unsigned i = 0; i < 64; ++i)
            defaultColorTable_[i] = Vector4(URHO3D_default_palette[i]);
    }

    bool VoxelColorPalette::BeginLoad(Deserializer& source)
    {
        return false;
    }

    const VariantVector& VoxelColorPalette::GetColors()
    {
        if (colors_.Size())
            return colors_;
        else
            return defaultColorTable_;
    }
}
