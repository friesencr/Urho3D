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

#include "VoxelWorld.h"
#

#include <Urho3D/DebugNew.h>

DEFINE_APPLICATION_MAIN(VoxelWorld)

static const int h = 64;
static const int w = 64;
static const int d = 64;


VoxelWorld::VoxelWorld(Context* context) :
    Sample(context)
{
    counter_ = 0;
}

void VoxelWorld::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();
}

void VoxelWorld::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();
    DebugRenderer* debug = scene_->CreateComponent<DebugRenderer>();

    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000, 1000));
    zone->SetAmbientColor(Color(0.2, 0.2, 0.2));

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky");
	skyNode->SetScale(500.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(500.0);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));


     //Create a directional light to the world so that we can see something. The light scene node's orientation controls the
     //light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
     //The light will use default settings (white light, no shadows)
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized

    //Light* light = lightNode->CreateComponent<Light>();
    //light->SetLightType(LIGHT_DIRECTIONAL);
    //light->SetCastShadows(true);
    //light->SetBrightness(0.7);

    //Node* planeNode = scene_->CreateChild("Plane");
    //planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    //StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    //planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    //planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));


    voxelNode_ = scene_->CreateChild("VoxelNode");
    //VoxelSet* voxelSet = voxelNode_->CreateComponent<VoxelSet>();
    voxelDefinition_ = new VoxelDefinition(context_);
	unsigned char geoSolid = VoxelDefinition::EncodeGeometry(VOXEL_TYPE_SOLID);
	unsigned char geoSlant = VoxelDefinition::EncodeGeometry(VOXEL_TYPE_SOLID);

    //unsigned char heightNormal = VoxelDefinition::EncodeVHeight(VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
	voxelDefinition_->blockTex1Face.Resize(5);
    //unsigned char heightSlope = VoxelDefinition::EncodeBlockTypeVHeight(VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
    //unsigned char heightRoof = VoxelDefinition::EncodeBlockTypeVHeight(VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0);
    //voxelDefinition_->blockVHeight.Push(0);
    //voxelDefinition_->blockVHeight.Push(heightNormal);
    //voxelDefinition_->blockVHeight.Push(heightNormal);

	// empty
	voxelDefinition_->blockTex1Face[0][1] = 1;
	voxelDefinition_->blockTex1Face[0][2] = 1;
	voxelDefinition_->blockTex1Face[0][3] = 1;
	voxelDefinition_->blockTex1Face[0][4] = 1;
	voxelDefinition_->blockTex1Face[0][5] = 1;

	// grass
	voxelDefinition_->blockTex1Face[1][0] = 0;
	voxelDefinition_->blockTex1Face[1][1] = 0;
	voxelDefinition_->blockTex1Face[1][2] = 0;
	voxelDefinition_->blockTex1Face[1][3] = 0;
	voxelDefinition_->blockTex1Face[1][4] = 0;
	voxelDefinition_->blockTex1Face[1][5] = 0;

	// grass
	voxelDefinition_->blockTex1Face[3][0] = 0;
	voxelDefinition_->blockTex1Face[3][1] = 0;
	voxelDefinition_->blockTex1Face[3][2] = 0;
	voxelDefinition_->blockTex1Face[3][3] = 0;
	voxelDefinition_->blockTex1Face[3][4] = 0;
	voxelDefinition_->blockTex1Face[3][5] = 0;

	// dirt
	voxelDefinition_->blockTex1Face[2][0] = 1;
	voxelDefinition_->blockTex1Face[2][1] = 1;
	voxelDefinition_->blockTex1Face[2][2] = 1;
	voxelDefinition_->blockTex1Face[2][3] = 1;
	voxelDefinition_->blockTex1Face[2][4] = 1;
	voxelDefinition_->blockTex1Face[2][5] = 1;

	// dirt
	voxelDefinition_->blockTex1Face[4][0] = 1;
	voxelDefinition_->blockTex1Face[4][1] = 1;
	voxelDefinition_->blockTex1Face[4][2] = 1;
	voxelDefinition_->blockTex1Face[4][3] = 1;
	voxelDefinition_->blockTex1Face[4][4] = 1;
	voxelDefinition_->blockTex1Face[4][5] = 1;

	// voxelDefinition_->blockTex1Face.Push(empty);
	// voxelDefinition_->blockTex1Face.Push(dirt);
	// voxelDefinition_->blockTex1Face.Push(grass);
    voxelDefinition_->blockGeometry.Push(0);
    voxelDefinition_->blockGeometry.Push(geoSolid);
    voxelDefinition_->blockGeometry.Push(geoSolid);
    voxelDefinition_->blockGeometry.Push(geoSlant);
    voxelDefinition_->blockGeometry.Push(geoSlant);
    //voxelDefinition_->blocktype.Clear();

    voxelDefinition_->SetSize(w, h, d);

	SharedPtr<Texture2DArray> texture(new Texture2DArray(context_));
	Vector<SharedPtr<Image> > images;
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("Textures/grass.png")));
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("Textures/dirt.png")));
	if (!texture->SetData(images))
		return;

	voxelDefinition_->diffuse1Textures = texture;

    //Node* spotNode = cameraNode_->CreateChild("PointLight");
    //spotNode->SetPosition(Vector3(0.0, -5.0, -5.0));
    //Light* spotLight = spotNode->CreateComponent<Light>();
    //spotLight->SetLightType(LIGHT_POINT);
    //spotLight->SetCastShadows(true);
    //spotLight->SetRange(100.0);
}

void VoxelWorld::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
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

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

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

    //if (counter_ != 0)
    //    return;
	if (counter_ == 0)
	{
		for (unsigned x = 0; x < 2; ++x)
		{
			for (unsigned y = 0; y < 2; ++y)
			{
				Node* node = voxelNode_->CreateChild("VoxelChunk_" + String(x) + "_" + String(y));
				node->SetPosition(Vector3(x * 64, 0, y * 64));
				VoxelChunk* chunk = node->CreateComponent<VoxelChunk>();
				Material* material = chunk->GetMaterial(0);
				material->SetTexture(TU_DIFFUSE, voxelDefinition_->diffuse1Textures);
			}
		}
	}

    counter_++;
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();

    float offset = (float)h / 2.0 + ((counter_ % 10) - 5);
    float sphereSize = 25.0; // +((counter_ % 10) - 5);
    int counter = counter_ % h;
	//voxelDefinition_->SetSize(w, h, d);
	voxelDefinition_->InitializeBlocktype();
    for (unsigned x = 0; x < w; ++x)
    {
    	for (unsigned z = 0; z < d; ++z)
    	{
    		for (unsigned y = 0; y < h; ++y)
    		{
    			//voxelDefinition_->SetBlocktype(x, y, z, Rand() % 100 ? 0 : 1);
    			Vector3 v(x, y, z);
    			v = v - Vector3(w/2.0, h/2.0, offset);
    			voxelDefinition_->SetBlocktype(x,y,z,v.Length() < sphereSize && v.Length() > sphereSize - 5.0 ? 1 : 0);
    		}
    		counter++;
    	}
    }

    for (unsigned x = 0; x < 2; ++x)
    {
    	for (unsigned y = 0; y < 2; ++y)
    	{
			Node* node = voxelNode_->GetChild("VoxelChunk_" + String(x) + "_" + String(y));
			VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
    		builder->BuildVoxelChunk(chunk, voxelDefinition_);
    	}
    }

  //  Image* heightMap = cache->GetResource<Image>("Textures/HeightMap.png");
  //  SharedPtr<VoxelDefinition> definition(new VoxelDefinition(context_));
  //  for (unsigned a = 0; a < heightMap->GetWidth() / 64; ++a)
  //  {
		//unsigned chunkX = a * 64;
  //      for (unsigned b = 0; b < heightMap->GetHeight() / 64; ++b)
  //      {
		//	unsigned chunkZ = b * 64;
		//	Node* node = voxelNode_->CreateChild();
		//	node->SetPosition(Vector3(a * 64, 0, b * 64));
		//	VoxelChunk* chunk = node->CreateComponent<VoxelChunk>();
		//	definition->SetSize(64, 128, 64);
		//	definition->InitializeBlocktype();
		//	definition->InitializeVHeight();
		//	definition->blockGeometry = voxelDefinition_->blockGeometry;
		//	definition->blockTex1Face = voxelDefinition_->blockTex1Face;
		//	for (unsigned x = 0; x < w; ++x)
		//	{
		//		for (unsigned z = 0; z < d; ++z)
		//		{
		//			Color c = heightMap->GetPixel(a * 64 + x, b * 64 + z);
		//			int y =  (255 - ((heightMap->GetPixelInt(chunkX + x    , chunkZ + z    ) & 0x0000FF00) >> 8));
		//			int nw = (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z - 1) & 0x0000FF00) >> 8));
		//			//unsigned n =  (255 - ((heightMap->GetPixelInt(chunkX + x    , chunkZ + z - 1) & 0x0000FF00) >> 8));
		//			int ne = (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z - 1) & 0x0000FF00) >> 8));
		//			//unsigned w =  (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z    ) & 0x0000FF00) >> 8));
		//			//unsigned e =  (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z    ) & 0x0000FF00) >> 8));
		//			int sw = (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z + 1) & 0x0000FF00) >> 8));
		//			//unsigned s =  (255 - ((heightMap->GetPixelInt(chunkX + x    , chunkZ + z + 1) & 0x0000FF00) >> 8));
		//			int se = (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z + 1) & 0x0000FF00) >> 8));
		//			int height = y / 4;

		//			for (unsigned i = height - 4; i < height; ++i)
		//			{
		//				definition->SetBlocktype(x, i, z, c.Average() > 0.5 ? 1 : 2);
		//				definition->SetVheight(x, i, z, VoxelHeight::VOXEL_HEIGHT_1, VoxelHeight::VOXEL_HEIGHT_1, VoxelHeight::VOXEL_HEIGHT_1, VoxelHeight::VOXEL_HEIGHT_1);
		//			}

		//			// gather nearby values to get height
		//			VoxelHeight heights[4];
		//			float heightValues[4] = {
		//				(float)(y - sw) / 2.0,
		//				(float)(y - se) / 2.0,
		//				(float)(y - nw) / 2.0,
		//				(float)(y - ne) / 2.0,
		//			};
		//			for (unsigned j = 0; j < 4; ++j)
		//			{
		//				float hv = heightValues[j];
		//				if (hv < -0.5)
		//					heights[j] = VoxelHeight::VOXEL_HEIGHT_0;
		//				else if (hv < 0.0)
		//					heights[j] = VoxelHeight::VOXEL_HEIGHT_HALF;
		//				else if (hv < 0.5)
		//					heights[j] = VoxelHeight::VOXEL_HEIGHT_1;
		//				else
		//					heights[j] = VoxelHeight::VOXEL_HEIGHT_1;
		//			}

		//			definition->SetBlocktype(x, height, z, c.Average() > 0.5 ? 3 : 4);
		//			definition->SetVheight(x, height, z, heights[0], heights[1], heights[2], heights[3]);
		//		}
		//	}
		//	builder->BuildVoxelChunk(chunk, definition);
  //      }
  //  }
}

void VoxelWorld::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    //DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();

    //// If draw debug mode is enabled, draw navigation mesh debug geometry
    //PODVector<VoxelChunk*> voxelChunks;
    //scene_->GetComponents<VoxelChunk>(voxelChunks, true);
    //for (unsigned i = 0; i < voxelChunks.Size(); ++i)
    //	voxelChunks[i]->DrawDebugGeometry(debug, true);
    //scene_->GetComponent<Octree>()->DrawDebugGeometry(true);
}


