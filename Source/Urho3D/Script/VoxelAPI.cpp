#include "../Voxel/Voxel.h"
#include "../Voxel/VoxelBuilder.h"
#include "../Voxel/VoxelSet.h"
#include "../Voxel/VoxelChunk.h"

/* namespace Urho3D */
/* { */
/*     static void RegisterVoxel(asIScriptEngine* engine) */
/*     { */
/*         RegisterDrawable<VoxelChunk>(engine, "VoxelChunk"); */
/*         RegisterComponent<VoxelSet>(engine, "VoxelSet"); */
/*         RegisterResource<VoxelBlocktypeMap>(engine, "VoxelBlocktypeMap"); */
/*         RegisterResource<VoxelColorPalette>(engine, "VoxelColorPallete"); */
/*         RegisterResource<VoxelOverlayMap>(engine, "VoxelOverlayMap"); */

/*         engine->RegisterEnum("VoxelGeometry"); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_EMPTY", VOXEL_GEOMETRY_EMPTY); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_KNOCKOUT", VOXEL_GEOMETRY_KNOCKOUT); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_SOLID", VOXEL_GEOMETRY_SOLID); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_TRANSP", VOXEL_GEOMETRY_TRANSP); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_SLAB_UPPER", VOXEL_GEOMETRY_SLAB_UPPER); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_SLAB_LOWER", VOXEL_GEOMETRY_SLAB_LOWER); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP", VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM", VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED", VOXEL_GEOMETRY_FLOOR_SLOPE_NORTH_IS_TOP_AS_WALL_UNIMPLEMENTED); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED", VOXEL_GEOMETRY_CEIL_SLOPE_NORTH_IS_BOTTOM_AS_WALL_UNIMPLEMENTED); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_CROSSED_PAIR", VOXEL_GEOMETRY_CROSSED_PAIR); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_FORCE", VOXEL_GEOMETRY_FORCE); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_FLOOR_VHEIGHT_03", VOXEL_GEOMETRY_FLOOR_VHEIGHT_03); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_FLOOR_VHEIGHT_12", VOXEL_GEOMETRY_FLOOR_VHEIGHT_12); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_CEIL_VHEIGHT_03", VOXEL_GEOMETRY_CEIL_VHEIGHT_03); */
/*         engine->RegisterEnumValue("VoxelGeometry", "VOXEL_GEOMETRY_CEIL_VHEIGHT_12", VOXEL_GEOMETRY_CEIL_VHEIGHT_12); */

/*         engine->RegisterEnum("VoxelRotation"); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_EAST", VOXEL_FACE_EAST); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_NORTH", VOXEL_FACE_NORTH); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_WEST", VOXEL_FACE_WEST); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_SOUTH", VOXEL_FACE_SOUTH); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_UP", VOXEL_FACE_UP); */
/*         engine->RegisterEnumValue("VoxelRotation", "VOXEL_FACE_DOWN", VOXEL_FACE_DOWN); */

/*         engine->RegisterEnum("VoxelHeight"); */
/*         engine->RegisterEnumValue("VoxelHeight", "VOXEL_HEIGHT_0", VOXEL_HEIGHT_0); */
/*         engine->RegisterEnumValue("VoxelHeight", "VOXEL_HEIGHT_HALF", VOXEL_HEIGHT_HALF); */
/*         engine->RegisterEnumValue("VoxelHeight", "VOXEL_HEIGHT_1", VOXEL_HEIGHT_1); */
/*         engine->RegisterEnumValue("VoxelHeight", "VOXEL_HEIGHT_ONE_AND_HALF", VOXEL_HEIGHT_ONE_AND_HALF); */

/*         engine->RegisterEnum("VoxelFaceRotation"); */
/*         engine->RegisterEnumValue("VoxelFaceRotation", "VOXEL_FACEROT_0", VOXEL_FACEROT_0); */
/*         engine->RegisterEnumValue("VoxelFaceRotation", "VOXEL_FACEROT_90", VOXEL_FACEROT_90); */
/*         engine->RegisterEnumValue("VoxelFaceRotation", "VOXEL_FACEROT_180", VOXEL_FACEROT_180); */
/*         engine->RegisterEnumValue("VoxelFaceRotation", "VOXEL_FACEROT_270", VOXEL_FACEROT_270); */

/*         engine->RegisterGlobalFunction("uint8 voxelEncodeColor(Color, bool, bool)", asFUNCTION(VoxelEncodeColor), asCALL_CDECL); */
/*         engine->RegisterGlobalFunction("uint8 voxelEncodeVHeight(VoxelHeight, VoxelHeight, VoxelHeight, VoxelHeight)", asFUNCTION(VoxelEncodeVHeight), asCALL_CDECL); */
/*         engine->RegisterGlobalFunction("uint8 voxelEncodeGeometry(VoxelGeometry, VoxelRotation, VoxelHeight)", asFUNCTION(VoxelEncodeGeometry), asCALL_CDECL); */
/*         //engine->RegisterGlobalFunction("unsigned char voxelEncodeFaceMask(bool, bool, bool, bool, bool, bool)", asFUNCTION(VoxelEncodeFaceMask), asCALL_CDECL); */
/*         //engine->RegisterGlobalFunction("unsigned char voxelEncodeSideTextureSwap(unsigned char, unsigned char, unsigned char)", asFUNCTION(VoxelEncodeSideTextureSwap), asCALL_CDECL); */

/*         RegisterResource<VoxelMap>(engine, "VoxelMap"); */
/*         //engine->RegisterObjectMethod("VoxelMap", "SetSource(Generator generator)", asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "SetSource(ResourceRef resourceRef)", asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "SetDataMask(unsigned dataMask)", asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "SetProcessorDataMask(unsigned processorDataMask)", asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_height() const", asMETHOD(VoxelMap, GetHeight), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_width() const", asMETHOD(VoxelMap, GetWidth), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_depth() const", asMETHOD(VoxelMap, GetDepth), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_size() const", asMETHOD(VoxelMap, GetSize), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_dataMask() const", asMETHOD(VoxelMap, GetDataMask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint get_processorDataMask() const", asMETHOD(VoxelMap, GetProcessorDataMask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint GetIndex(int, int, int) const", asMETHOD(VoxelMap, GetIndex), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetSize(uint8, uint8, uint8)", asMETHOD(VoxelMap, SetSize), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeBlocktype(uint8)", asMETHOD(VoxelMap, InitializeBlocktype), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeVHeight(uint8)", asMETHOD(VoxelMap, InitializeVHeight), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeLighting(uint8)", asMETHOD(VoxelMap, InitializeLighting), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeColor(uint8)", asMETHOD(VoxelMap, InitializeColor), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeTex2(uint8)", asMETHOD(VoxelMap, InitializeTex2), asCALL_THISCALL); */
/*         //engine->RegisterObjectMethod("VoxelMap", "void InitializeGeometry(uint8)", asMETHOD(VoxelMap, InitializeGeometry), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetBlockType(int , int , int) const", asMETHOD(VoxelMap, GetBlocktype), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetOverlay(int, int, int) const", asMETHOD(VoxelMap, GetOverlay), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetColor(int, int, int) const", asMETHOD(VoxelMap, GetColor), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetColor2(int, int, int) const", asMETHOD(VoxelMap, GetColor2), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetColor2Facemask(int , int, int) const", asMETHOD(VoxelMap, GetColor2Facemask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetColor3(int , int, int) const", asMETHOD(VoxelMap, GetColor3), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetColor3Facemask(int, int, int) const", asMETHOD(VoxelMap, GetColor3Facemask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetExtendedColor(int, int, int) const", asMETHOD(VoxelMap, GetExtendedColor), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetEColor(int, int, int) const", asMETHOD(VoxelMap, GetEColor), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetEcolorFaceMask(int, int, int) const", asMETHOD(VoxelMap, GetEColorFaceMask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetGeometry(int, int, int) const", asMETHOD(VoxelMap, GetGeometry), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetVHeight(int, int, int) const", asMETHOD(VoxelMap, GetVHeight), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetLighting(int, int, int) const", asMETHOD(VoxelMap, GetLighting), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetRotate(int, int, int) const", asMETHOD(VoxelMap, GetRotate), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetTex2(int, int, int) const", asMETHOD(VoxelMap, GetTex2), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetTex2Replace(int, int, int) const", asMETHOD(VoxelMap, GetTex2Replace), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "uint8 GetTex2Facemask(int, int, int) const", asMETHOD(VoxelMap, GetTex2Facemask), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetColor(int, int, int, uint8)", asMETHOD(VoxelMap, SetColor), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetBlocktype(int, int, int, uint8)", asMETHOD(VoxelMap, SetBlocktype), asCALL_THISCALL); */
/*     //    engine->RegisterObjectMethod("VoxelMap", "void SetVHeight(int, int, int, VoxelHeight, VoxelHeight, VoxelHeight, VoxelHeight)", asMETHOD(VoxelMap, SetVHeight), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetLighting(int, int, int, uint8)", asMETHOD(VoxelMap, SetLighting), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetTex2(int, int, int, uint8)", asMETHOD(VoxelMap, SetTex2), asCALL_THISCALL); */
/*         engine->RegisterObjectMethod("VoxelMap", "void SetGeometry(int, int, int, VoxelGeometry)", asMETHOD(VoxelMap, SetGeometry), asCALL_THISCALL); */
/*     } */

/*     static VoxelBuilder* GetVoxelBuilder() */
/*     { */
/*         return GetScriptContext()->GetSubsystem<VoxelBuilder>(); */
/*     } */

/*     void RegisterVoxelAPI(asIScriptEngine* engine) */
/*     { */
/*         RegisterVoxel(engine); */
/*     } */
/* } */

