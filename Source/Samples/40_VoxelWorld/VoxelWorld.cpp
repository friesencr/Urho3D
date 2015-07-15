//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho3D.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/VoxelSet.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/VoxelBuilder.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/IO/Generator.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>

#include "VoxelWorld.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include <Urho3D/DebugNew.h>

//#define SMOOTH_TERRAIN

DEFINE_APPLICATION_MAIN(VoxelWorld)

static const int h = 128;
static const int w = 64;
static const int d = 64;

// spoofing a VoxelMap load
// uint height, uint width, uint depth, unit datamask
static const unsigned chunkHeader[4] = { w, h, d, VOXEL_BLOCK_BLOCKTYPE };
static const unsigned headerSize = sizeof(chunkHeader);
static const unsigned dataSize = (h + 4) * (w + 4) * (d + 4);
static const unsigned podsize = dataSize + 3;
static const unsigned generatorSize = headerSize + podsize;

float noiseFactors[10] = {
    1.0, 1.0, 1.0, 0.3,
    0.3, 0.4, 0.7, 1.0,
    1.0, 1.0
};

static void AOVoxelLighting(VoxelChunk* chunk, VoxelMap* src, VoxelProcessorWriters writers)
{
    unsigned char* bt = 0;
    const int xStride = src->xStride;
    const int zStride = src->zStride;

    for (int x = -1; x < (int)src->width_+1; x++)
    {
        for (int z = -1; z < (int)src->depth_+1; z++)
        {
            for (int y = -1; y < (int)src->height_+1; y++)
            {
                int index = src->GetIndex(x, y, z);
                bt = &src->blocktype[index];

#ifdef SMOOTH_TERRAIN
                if (bt[0] > 0)
                {
                    if (bt[1] == 0)
                    {
                        VoxelHeight scores[4] = { VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0 };
                        int corners[4] = { -xStride - zStride, xStride - zStride, -xStride + zStride, xStride + zStride };
                        unsigned checks[4][6] = {
                            { zStride, zStride+1, 0, 1, xStride, xStride+1 },
                            { zStride, zStride+1, 0, 1, -xStride, -xStride+1 },
                            { -zStride, -zStride+1, 0, 1, xStride, xStride+1 },
                            { -zStride, -zStride+1, 0, 1, -xStride, -xStride+1 }
                        };
                        //int checks[4][3] = {
                        //    { zStride+1, 1, xStride+1 },
                        //    { zStride+1, 1, -xStride+1 },
                        //    { -zStride+1, 1, xStride+1 },
                        //    { -zStride+1, 1, -xStride+1 }
                        //};

                        unsigned score = 0;
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            for (unsigned j = 0; j < 6; ++j)
                            {
                                score += bt[corners[i] + checks[i][j]] > 0;
                            }
                            scores[i] = (VoxelHeight)(unsigned)(score/6);
                        }
                        writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_FLOOR_VHEIGHT_03);
                        writers.vHeight.buffer[index] = VoxelEncodeVHeight(scores[0], scores[1], scores[2], scores[3]);
                    }
                    //else if (bt[-1] == 0)
                    //{
                    //    //writers.geometry.buffer[index] = VoxelEncodeGeometry(x*z % 2 == 0 ? VOXEL_TYPE_CEIL_VHEIGHT_03 : VOXEL_TYPE_CEIL_VHEIGHT_12);
                    //    writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_CEIL_VHEIGHT_03);
                    //    writers.vHeight.buffer[index] = VoxelEncodeVHeight(
                    //        (bt[-xStride - zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,   // sw
                    //        (bt[xStride - zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,    // se
                    //        (bt[-xStride + zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0,   // nw
                    //        (bt[xStride + zStride - 1] > 0) ? VOXEL_HEIGHT_1 : VOXEL_HEIGHT_0    // ne
                    //    );
                    //}
                    else
                        writers.geometry.buffer[index] = VoxelEncodeGeometry(VOXEL_TYPE_SOLID);
                }
#endif
                int light = 
                    (bt[-xStride - zStride] == 0) +  // nw
                    (bt[-zStride] == 0) +            // n
                    (bt[xStride - zStride] == 0) +   // ne
                    (bt[-xStride] == 0) +            // w
                    (bt[0] == 0) +                   // origin
                    (bt[xStride] == 0) +             // e
                    (bt[-xStride + zStride] == 0) +  // sw
                    (bt[zStride] == 0) +             // s
                    (bt[xStride + zStride] == 0);    // se

                writers.lighting.buffer[index - 1] += light * 255 / 27;
                writers.lighting.buffer[index] += light * 255 / 27;
                writers.lighting.buffer[index + 1] += light * 255 / 27;
            }
        }
    }
}

static void FillTerrainPerlin(Context* context, unsigned char* dataPtr, VariantMap& parameters, unsigned podIndex)
{
    VoxelWriter writer;
    writer.SetSize(w, h, d);
    writer.InitializeBuffer(dataPtr);
    writer.Clear(0);
    unsigned tileX = parameters["TileX"].GetUInt();
    unsigned tileZ = parameters["TileZ"].GetUInt();
    unsigned chunkX = parameters["TileX"].GetUInt() * w;
    unsigned chunkZ = parameters["TileZ"].GetUInt() * d;


    const int DEEP_WATER_BLOCK = 27;
    const int WATER_BLOCK = 28;
    const int DESERT_BLOCK = 44;
    const int DIRT_BLOCK = 44;
    const int GRASS_BLOCK = 36;
    const int SLATE_BLOCK = 29;
    const int FLOOR_BLOCK = 12;
    const int SNOW_BLOCK = 5;
    const int LIGHT_SNOW_BLOCK = 2;
    const int WHITE_SNOW_BLOCK = 0;

    const int FLOOR_HEIGHT = 10;
    const int SLATE_HEIGHT = 20;
    const int DIRT_HEIGHT = 30;
    const int DESERT_HEIGHT = 45;
    const int GRASS_HEIGHT = 70;
    const int SNOW_HEIGHT = 90;
    const int LIGHT_SNOW_HEIGHT = 105;
    const int WHITE_SNOW_HEIGHT = 128;
    const int WATER_HEIGHT = 30;

    int blocks[8] = {FLOOR_BLOCK, SLATE_BLOCK, DIRT_BLOCK, DESERT_BLOCK, GRASS_BLOCK, SNOW_BLOCK, LIGHT_SNOW_BLOCK, WHITE_SNOW_BLOCK };
    int heights[8] = {FLOOR_HEIGHT, SLATE_HEIGHT, DIRT_HEIGHT, DESERT_HEIGHT, GRASS_HEIGHT, SNOW_HEIGHT, LIGHT_SNOW_HEIGHT, WHITE_SNOW_HEIGHT };
    int numBlocks = 8;

    for (unsigned x = 0; x < w; ++x)
    {
        for (unsigned z = 0; z < d; ++z)
        {
            // detail noise
            float dt = 0.0;
            for (int o = 3; o < 5; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + chunkX) / scale, (z + chunkZ) / scale, (float)-o, 256, 256, 256);
                dt += Abs(ns);
            }

            // low frequency
            float ht = 0.0;
            for (int o = 3; o < 9; ++o)
            {
                float scale = (float)(1 << o);
                float ns = stb_perlin_noise3((x + chunkX) / scale, (z + chunkZ) / scale, (float)o, 256, 256, 256);
                ht += ns * noiseFactors[o];
            }

            // biome
            float biome = stb_perlin_noise3((x + chunkX) / 2048, (z + chunkZ) / 2048, 32.0, 256, 256, 256);

            int height = (int)((ht + 0.2) * 45.0) + 32;
            height = Clamp(height, 1, 128);

            for (unsigned i = 0; i < height; ++i)
            {
                int h = ((float)i * (dt/2.0 + 1.0));
                int b = 0;
                for (int bh = 0; bh < numBlocks - 1; ++bh)
                {
                    if (h < heights[bh])
                    {
                        b = bh;
                        break;
                    }
                }
                writer.SetBlocktype(x, i, z, blocks[b] + (dt > 0.5 ? 1 : 0));
            }

            if (height < WATER_HEIGHT)
            {
                for (unsigned i = 0; i < WATER_HEIGHT; ++i)
                {
                    if (height > WATER_HEIGHT - 5 && i > WATER_HEIGHT - 5)
                        writer.SetBlocktype(x, i, z, WATER_BLOCK);
                    else
                        writer.SetBlocktype(x, i, z, DEEP_WATER_BLOCK +(dt > 0.7 ? 1 : 0));
                }
            }
        }
    }
}

static unsigned RandomTerrain(Context* context, void* dest, unsigned size, unsigned position, VariantMap& parameters)
{
    if (position < 4 * sizeof(unsigned))
    {
        unsigned* dataPtr = (unsigned*)dest;
        *dataPtr++ = chunkHeader[position/4];
        return 4;
    }
    else if ((position - headerSize) % podsize < 3)
    {
        unsigned char* dataPtr = (unsigned char*)dest;
        unsigned char vle[3];
        vle[0] = (unsigned char)(dataSize | 0x80);
        vle[1] = (unsigned char)((dataSize >> 7) | 0x80);
        vle[2] = dataSize >> 14;
        *((unsigned char*)dataPtr) = vle[(position - headerSize) % podsize];
        dataPtr += 1;
        return 1;
    }
    else
    {
        unsigned podIndex = (position - headerSize) / podsize;
        unsigned char* dataPtr = (unsigned char*)dest;
        FillTerrainPerlin(context, dataPtr, parameters, podIndex);
        return dataSize;
    }
    return 0;
}

VoxelWorld::VoxelWorld(Context* context) :
    Sample(context)
{
    ProcSky::RegisterObject(context);
    counter_ = 0;
}

void VoxelWorld::Start()
{
    Generator::RegisterGeneratorFunction("RandomTerrain", RandomTerrain);

    // Execute base class startup
    Sample::Start();

    Graphics* graphics = GetSubsystem<Graphics>();
    IntVector2 resolution = graphics->GetResolutions()[0];
    //graphics->SetMode(resolution.x_, resolution.y_, true, false, false, false, false, 4);
    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    procSky_->Initialize();

    // Hook up to the frame update events
    SubscribeToEvents();
}

void VoxelWorld::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->SetAutoReloadResources(true);
    Renderer* renderer = GetSubsystem<Renderer>();
    //renderer->SetMaxOccluderTriangles(10000);

    scene_ = new Scene(context_);

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();
    DebugRenderer* debug = scene_->CreateComponent<DebugRenderer>();
    scene_->CreateComponent<PhysicsWorld>();


    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    //Node* skyNode = scene_->CreateChild("Sky");
    //Skybox* skybox = skyNode->CreateComponent<Skybox>();
    //skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    //skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(900.0);

    // Set an initial position for the camera scene node above the plane
    //cameraNode_->SetPosition(Vector3(1024.0, 128.0, 1024.0));
    cameraNode_->SetPosition(Vector3(0.0, 50.0, 0.0));

    Node* skyNode = scene_->CreateChild("SkyNode");
    procSky_ = skyNode->CreateComponent<ProcSky>();

    //Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    //light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    //The light will use default settings (white light, no shadows)
    Node* lightNode = skyNode->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.8f, -0.2f, 0.8f)); // The direction vector does not need to be normalized

    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(false);
    light->SetBrightness(0.0);
    light->SetEnabled(false);
    //light->SetColor(Color(1.0, 1.0, 1.0));
    //   light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    //light->SetShadowCascade(CascadeParameters(10.0f, 150.0f, 400.0f, 0.0f, 0.8f));

    Zone* zone = skyNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(Vector3(-100000, 0, -100000), Vector3(100000, 128, 100000)));
    zone->SetAmbientColor(Color(0.2, 0.2, 0.2));
    zone->SetHeightFog(true);
    //zone->SetAmbientGradient(true);
    zone->SetFogColor(Color(0.3, 0.3, 0.3));
    zone->SetFogStart(700.0f);
    zone->SetFogEnd(900.0f);


    voxelNode_ = scene_->CreateChild("VoxelNode");
    voxelBlocktypeMap_ = new VoxelBlocktypeMap(context_);
    voxelBlocktypeMap_->blockColor.Push(0);
    for (unsigned i = 1; i < 64; ++i)
        voxelBlocktypeMap_->blockColor.Push(i);

    File file(context_);
    if (file.Open("BlocktypeMap.bin", FILE_WRITE))
        voxelBlocktypeMap_->Save(file);

    //SharedPtr<Texture2DArray> texture(new Texture2DArray(context_));
    //Vector<SharedPtr<Image> > images;
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Beach_sand_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Bowling_grass_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Brown_dirt_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/stone/Gray_marble_pxr128.png")));
    //if (!texture->SetData(images))
    //  return;

    //voxelBlocktypeMap_->diffuse1Textures = texture;

    unsigned numX = 256;
    unsigned numZ = 256;
    VoxelSet* voxelSet = voxelNode_->CreateComponent<VoxelSet>();
    voxelSet->SetNumberOfChunks(numX, 1, numZ);
    for (unsigned x = 0; x < numX; ++x)
    {
        for (unsigned z = 0; z < numZ; ++z)
        {
            VoxelMap* map = new VoxelMap(context_);
            map->SetDataMask(VOXEL_BLOCK_BLOCKTYPE);
            map->SetSize(w, h, d);
            map->blocktypeMap = voxelBlocktypeMap_;
            SharedPtr<Generator> terrainGenerator(new Generator(context_));
            terrainGenerator->SetSize(generatorSize);
            terrainGenerator->SetName("RandomTerrain");
            VariantMap params;
            params["TileX"] = x;
            params["TileZ"] = z;
            terrainGenerator->SetParameters(params);
            map->SetSource(terrainGenerator);
            map->AddVoxelProcessor(AOVoxelLighting);
#ifdef SMOOTH_TERRAIN
            map->SetProcessorDataMask(VOXEL_BLOCK_LIGHTING | VOXEL_BLOCK_GEOMETRY | VOXEL_BLOCK_VHEIGHT);
#else
            map->SetProcessorDataMask(VOXEL_BLOCK_LIGHTING);
#endif
            voxelSet->SetVoxelMap(x, 0, z, map);
        }
    }
    voxelSet->BuildAsync();

    //   Node* spotNode = cameraNode_->CreateChild("PointLight");
    //   spotNode->SetPosition(Vector3(0.0, -15.0, 0.0));
    //   Light* spotLight = spotNode->CreateComponent<Light>();
    //   spotLight->SetLightType(LIGHT_POINT);
    //   spotLight->SetCastShadows(true);
    //   spotLight->SetRange(100.0);
    //spotLight->SetBrightness(0.7);
}

void VoxelWorld::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    cameraPositionText_ = instructionText;
    instructionText->SetText(cameraNode_->GetPosition().ToString());
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);

}

void VoxelWorld::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void VoxelWorld::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 100.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    Node* lightNode = scene_->GetChild("DirectionalLight");
    if (lightNode)
    {
        //lightNode->Rotate(Quaternion(0.0, 30.0 * timeStep, 0.0));
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // "Shoot" a physics object with left mousebutton
    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        SpawnObject();

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown('W'))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('S'))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('A'))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('D'))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    VoxelSet* voxelSet = voxelNode_->GetComponent<VoxelSet>();
    if (voxelSet)
        cameraPositionText_->SetText(String(voxelSet->GetNumberOfLoadedChunks()));

    // Toggle physics debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

void VoxelWorld::SpawnObject()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Create a smaller box at camera position
    Node* boxNode = scene_->CreateChild("SmallBox");
    boxNode->SetPosition(cameraNode_->GetPosition());
    boxNode->SetRotation(cameraNode_->GetRotation());
    boxNode->SetScale(0.25f);
    StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
    boxObject->SetCastShadows(true);

    // Create physics components, use a smaller mass also
    RigidBody* body = boxNode->CreateComponent<RigidBody>();
    body->SetMass(0.25f);
    body->SetFriction(0.75f);
    CollisionShape* shape = boxNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    const float OBJECT_VELOCITY = 10.0f;

    // Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
    // to overcome gravity better
    body->SetLinearVelocity(cameraNode_->GetRotation() * Vector3(0.0f, 0.25f, 1.0f) * OBJECT_VELOCITY);
}

void VoxelWorld::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, HANDLER(VoxelWorld, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, HANDLER(VoxelWorld, HandlePostRenderUpdate));
}

void VoxelWorld::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void VoxelWorld::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    //DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();

    //// If draw debug mode is enabled, draw navigation mesh debug geometry
    //PODVector<VoxelChunk*> voxelChunks;
    //scene_->GetComponents<VoxelChunk>(voxelChunks, true);
    //for (unsigned i = 0; i < voxelChunks.Size(); ++i)
    //  voxelChunks[i]->DrawDebugGeometry(debug, true);
    //scene_->GetComponent<Octree>()->DrawDebugGeometry(true);

    if (drawDebug_)
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}


