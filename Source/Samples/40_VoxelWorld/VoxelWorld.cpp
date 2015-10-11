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
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/IO/Generator.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Voxel/VoxelStreamer.h>

#include "VoxelWorld.h"

#include <Urho3D/DebugNew.h>

DEFINE_APPLICATION_MAIN(VoxelWorld)

VoxelWorld::VoxelWorld(Context* context) :
    Sample(context)
{
    ProcSky::RegisterObject(context);
    counter_ = 0;
}

void VoxelWorld::Start()
{
    //Graphics* graphics = GetSubsystem<Graphics>();
    //IntVector2 resolution = graphics->GetResolutions()[0];
    //graphics->SetMode(1920, 1080, true, false, false, false, false, 2);

    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    //procSky_->Initialize();

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
    Node* skyNode = scene_->CreateChild("Sky");
    Skybox* skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(900.0);

    // Set an initial position for the camera scene node above the plane
    //cameraNode_->SetPosition(Vector3(1024.0, 128.0, 1024.0));
    cameraNode_->SetPosition(Vector3(0.0, 50.0, 0.0));

    //Node* skyNode = scene_->CreateChild("SkyNode");
    //procSky_ = skyNode->CreateComponent<ProcSky>();

    //Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    //light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    //The light will use default settings (white light, no shadows)
    Node* lightNode = skyNode->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.8f, -0.2f, 0.8f)); // The direction vector does not need to be normalized

    //Light* light = lightNode->CreateComponent<Light>();
    //light->SetLightType(LIGHT_DIRECTIONAL);
    //light->SetCastShadows(false);
    //light->SetBrightness(0.0);
    //light->SetEnabled(false);
    //light->SetColor(Color(1.0, 1.0, 1.0));
    //   light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    //light->SetShadowCascade(CascadeParameters(10.0f, 150.0f, 400.0f, 0.0f, 0.8f));

    Zone* zone = skyNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(Vector3(-100000.0f, 0.0f, -100000.0f), Vector3(100000.0f, 128.0f, 100000.0f)));
    zone->SetAmbientColor(Color(0.7f, 0.7f, 0.7f));
    //zone->SetHeightFog(true);
    //zone->SetAmbientGradient(true);
    zone->SetFogColor(Color(0.3f, 0.3f, 0.3f));
    zone->SetFogStart(400.0f);
    zone->SetFogEnd(900.0f);


    voxelNode_ = scene_->CreateChild("VoxelNode");
    VoxelSet* voxelSet = voxelNode_->CreateComponent<VoxelSet>();
    worldBuilder_ = new WorldBuilder(context_);
    worldBuilder_->SetSize(128, 128);
    worldBuilder_->SetVoxelSet(voxelSet);
    worldBuilder_->ConfigureParameters();
    worldBuilder_->CreateWorld();
    //worldBuilder_->SaveWorld();
    //worldBuilder_->LoadWorld();
    //worldBuilder_->BuildWorld();
    VoxelStreamer* streamer = voxelNode_->CreateComponent<VoxelStreamer>();
    streamer->SetEnabled(true);

    //SharedPtr<Texture2DArray> texture(new Texture2DArray(context_));
    //Vector<SharedPtr<Image> > images;
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Beach_sand_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Bowling_grass_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Brown_dirt_pxr128.png")));
    //images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/stone/Gray_marble_pxr128.png")));
    //if (!texture->SetData(images))
    //  return;

    //voxelBlocktypeMap_->diffuse1Textures = texture;

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
    const float MOVE_SPEED = 1600.0f;
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
    //if (voxelSet)
    //    cameraPositionText_->SetText(String(voxelSet->GetNumberOfLoadedChunks()));

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

    //worldBuilder_->BuildWorld();

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void VoxelWorld::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();

    //PODVector<VoxelChunk*> voxelChunks;
    //scene_->GetComponents<VoxelChunk>(voxelChunks, true);
    //for (unsigned i = 0; i < voxelChunks.Size(); ++i)
    //  voxelChunks[i]->DrawDebugGeometry(debug, true);
    //scene_->GetComponent<Octree>()->DrawDebugGeometry(true);

    //if (drawDebug_)
    //    scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}


