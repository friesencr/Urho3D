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

#include "VoxelWorld.h"

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
	cache->SetAutoReloadResources(true);
	Renderer* renderer = GetSubsystem<Renderer>();
	renderer->SetMaxOccluderTriangles(10000);

    scene_ = new Scene(context_);

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();
    DebugRenderer* debug = scene_->CreateComponent<DebugRenderer>();
    scene_->CreateComponent<PhysicsWorld>();

    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000, 1000));
    zone->SetAmbientColor(Color(0.5, 0.5, 0.5));
	//zone->SetAmbientGradient(true);
	zone->SetFogColor(Color(0.9f, 1.0f, 1.0f));
	zone->SetFogStart(500.0f);
	zone->SetFogEnd(750.0f);


	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky");
	skyNode->SetScale(1000.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>();
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(1000.0);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));


     //Create a directional light to the world so that we can see something. The light scene node's orientation controls the
     //light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
     //The light will use default settings (white light, no shadows)
 //   Node* lightNode = scene_->CreateChild("DirectionalLight");
 //   lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized

 //   Light* light = lightNode->CreateComponent<Light>();
 //   light->SetLightType(LIGHT_DIRECTIONAL);
 //   light->SetCastShadows(true);
 //   light->SetBrightness(0.7);
	//light->SetColor(Color(0.7, 1.0, 0.7));

    //Node* planeNode = scene_->CreateChild("Plane");
    //planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    //StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    //planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    //planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));


    voxelNode_ = scene_->CreateChild("VoxelNode");
    //VoxelSet* voxelSet = voxelNode_->CreateComponent<VoxelSet>();
    voxelBlocktypeMap_ = new VoxelBlocktypeMap(context_);
	unsigned char geoSolid = VoxelEncodeGeometry(VOXEL_TYPE_SOLID);
	unsigned char geoSlant = VoxelEncodeGeometry(VOXEL_TYPE_SLAB_LOWER);

    //unsigned char heightNormal = VoxelDefinition::EncodeVHeight(VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
	voxelBlocktypeMap_->blockTex1Face.Resize(5);
    //unsigned char heightSlope = VoxelDefinition::EncodeBlockTypeVHeight(VOXEL_HEIGHT_0, VOXEL_HEIGHT_0, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
    //unsigned char heightRoof = VoxelDefinition::EncodeBlockTypeVHeight(VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_0, VOXEL_HEIGHT_0);
    //voxelBlocktypeMap_->blockVHeight.Push(0);
    //voxelBlocktypeMap_->blockVHeight.Push(heightNormal);
    //voxelBlocktypeMap_->blockVHeight.Push(heightNormal);

	// empty
	voxelBlocktypeMap_->blockTex1Face[0][1] = 0;
	voxelBlocktypeMap_->blockTex1Face[0][2] = 0;
	voxelBlocktypeMap_->blockTex1Face[0][3] = 0;
	voxelBlocktypeMap_->blockTex1Face[0][4] = 0;
	voxelBlocktypeMap_->blockTex1Face[0][5] = 0;

	// grass
	voxelBlocktypeMap_->blockTex1Face[1][0] = 0;
	voxelBlocktypeMap_->blockTex1Face[1][1] = 0;
	voxelBlocktypeMap_->blockTex1Face[1][2] = 0;
	voxelBlocktypeMap_->blockTex1Face[1][3] = 0;
	voxelBlocktypeMap_->blockTex1Face[1][4] = 0;
	voxelBlocktypeMap_->blockTex1Face[1][5] = 0;

	// dirt
	voxelBlocktypeMap_->blockTex1Face[2][0] = 1;
	voxelBlocktypeMap_->blockTex1Face[2][1] = 1;
	voxelBlocktypeMap_->blockTex1Face[2][2] = 1;
	voxelBlocktypeMap_->blockTex1Face[2][3] = 1;
	voxelBlocktypeMap_->blockTex1Face[2][4] = 1;
	voxelBlocktypeMap_->blockTex1Face[2][5] = 1;

	// brick
	voxelBlocktypeMap_->blockTex1Face[3][0] = 2;
	voxelBlocktypeMap_->blockTex1Face[3][1] = 2;
	voxelBlocktypeMap_->blockTex1Face[3][2] = 2;
	voxelBlocktypeMap_->blockTex1Face[3][3] = 2;
	voxelBlocktypeMap_->blockTex1Face[3][4] = 2;
	voxelBlocktypeMap_->blockTex1Face[3][5] = 2;

	// fabric
	voxelBlocktypeMap_->blockTex1Face[4][0] = 3;
	voxelBlocktypeMap_->blockTex1Face[4][1] = 3;
	voxelBlocktypeMap_->blockTex1Face[4][2] = 3;
	voxelBlocktypeMap_->blockTex1Face[4][3] = 3;
	voxelBlocktypeMap_->blockTex1Face[4][4] = 3;
	voxelBlocktypeMap_->blockTex1Face[4][5] = 3;

	// voxelBlocktypeMap_->blockTex1Face.Push(empty);
	// voxelBlocktypeMap_->blockTex1Face.Push(dirt);
	// voxelBlocktypeMap_->blockTex1Face.Push(grass);
    voxelBlocktypeMap_->blockGeometry.Push(0);
    voxelBlocktypeMap_->blockGeometry.Push(geoSolid);
    voxelBlocktypeMap_->blockGeometry.Push(geoSolid);
    voxelBlocktypeMap_->blockGeometry.Push(geoSolid);
    voxelBlocktypeMap_->blockGeometry.Push(geoSolid);
    //voxelBlocktypeMap_->blocktype.Clear();

	SharedPtr<Texture2DArray> texture(new Texture2DArray(context_));
	Vector<SharedPtr<Image> > images;
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Bowling_grass_pxr128.png")));
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/ground/Brown_dirt_pxr128.png")));
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/brick/Blue_glazed_pxr128.png")));
	images.Push(SharedPtr<Image>(cache->GetResource<Image>("BlockTextures/fabric/Flower_pattern_pxr128.png")));
	if (!texture->SetData(images))
		return;

	voxelBlocktypeMap_->diffuse1Textures = texture;

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
    const float MOVE_SPEED = 25.0f;
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
		// lightNode->Rotate(Quaternion(0.0, 30.0 * timeStep, 0.0));
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

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    VoxelBuilder* builder = GetSubsystem<VoxelBuilder>();

#if 0

	if (counter_ == 0)
	{
		for (unsigned x = 0; x < 1; ++x)
		{
			for (unsigned y = 0; y < 1; ++y)
			{
				Node* node = voxelNode_->CreateChild("VoxelChunk_" + String(x) + "_" + String(y));
				node->SetPosition(Vector3(x * 64, 0, y * 64));
				VoxelChunk* chunk = node->CreateComponent<VoxelChunk>();
				SharedPtr<VoxelMap> map(new VoxelMap(context_));
				map->SetSize(w, h, d);
				chunk->SetVoxelMap(map);
				node->CreateComponent<RigidBody>();
				CollisionShape* shape = node->CreateComponent<CollisionShape>();
				shape->SetVoxelTriangleMesh(chunk);
			}
		}
	}
	else
		return;


    float offset = (float)h / 2.0 + ((counter_ % 10) - 5);
    float sphereSize = 25.0; // +((counter_ % 10) - 5);
    int counter = counter_ % h;

	Node* node = voxelNode_->GetChild("VoxelChunk_" + String(0) + "_" + String(0));
	VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
	VoxelMap* map = chunk->GetVoxelMap();
	map->InitializeBlocktype();

    for (unsigned x = 0; x < w; ++x)
    {
    	for (unsigned z = 0; z < d; ++z)
    	{
    		for (unsigned y = 0; y < h; ++y)
    		{
				//map->SetBlocktype(x, y, z, 1);
				
    			Vector3 v(x, y, z);
    			v = v - Vector3(w/2.0, h/2.0, offset);
				map->SetBlocktype(x,y,z,v.Length() < sphereSize && v.Length() > sphereSize - 5.0 ? 1 : 0);
    		}
    	}
    }
	chunk->Build();

   // for (unsigned x = 0; x < 2; ++x)
   // {
   // 	for (unsigned y = 0; y < 2; ++y)
   // 	{
			//Node* node = voxelNode_->GetChild("VoxelChunk_" + String(x) + "_" + String(y));
			//VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
   // 		builder->BuildVoxelChunk(chunk, voxelDefinition_);
   // 	}
   // }
#endif

#if 1
	if (counter_ != 0)
		return;

    Image* heightMap = cache->GetResource<Image>("Textures/HeightMap.png");
	int numX = heightMap->GetWidth() / 64;
	int numZ = heightMap->GetHeight() / 64;
	const int heightFactor = 12;
	numX = 16;
	numZ = 16;
    for (unsigned a = 0; a < numX; ++a)
    {
		unsigned chunkX = a * 64;
        for (unsigned b = 0; b < numZ; ++b)
        {
			unsigned chunkZ = b * 64;
			//Node* node = voxelNode_->GetChild();
			String chunkName = "VoxelChunk_" + String(a) + "_" + String(b);
			Node* node = voxelNode_->CreateChild(chunkName);
			node->SetPosition(Vector3(a * 64, 0, b * 64));
			VoxelChunk* chunk = node->CreateComponent<VoxelChunk>();
			//chunk->SetOccludee(true);
			//chunk->SetOccluder(true);
			node->CreateComponent<RigidBody>();
			CollisionShape* shape = node->CreateComponent<CollisionShape>();
			shape->SetVoxelTriangleMesh(chunk);

			SharedPtr<VoxelMap> voxelMap(new VoxelMap(context_));
			chunk->SetVoxelMap(voxelMap);
			voxelMap->SetSize(64, 128, 64);
			voxelMap->InitializeBlocktype();
			voxelMap->InitializeVHeight();
			voxelMap->InitializeLighting();
			voxelMap->blocktypeMap = voxelBlocktypeMap_;
			for (unsigned x = 0; x < w; ++x)
			{
				for (unsigned z = 0; z < d; ++z)
				{
					Color c = heightMap->GetPixel(a * 64 + x, b * 64 + z);
					int y =  (255 - ((heightMap->GetPixelInt((chunkX + x) % heightMap->GetWidth(), (chunkZ + z) % heightMap->GetHeight()) & 0x0000FF00) >> 8));
					int nw = (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z - 1) & 0x0000FF00) >> 8));
					int n =  (255 - ((heightMap->GetPixelInt(chunkX + x    , chunkZ + z - 1) & 0x0000FF00) >> 8));
					int ne = (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z - 1) & 0x0000FF00) >> 8));
					int w =  (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z    ) & 0x0000FF00) >> 8));
					int e =  (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z    ) & 0x0000FF00) >> 8));
					int sw = (255 - ((heightMap->GetPixelInt(chunkX + x - 1, chunkZ + z + 1) & 0x0000FF00) >> 8));
					int s =  (255 - ((heightMap->GetPixelInt(chunkX + x    , chunkZ + z + 1) & 0x0000FF00) >> 8));
					int se = (255 - ((heightMap->GetPixelInt(chunkX + x + 1, chunkZ + z + 1) & 0x0000FF00) >> 8));
					int height = y / heightFactor;

					for (unsigned i = 0; i < height; ++i)
					{
						voxelMap->SetBlocktype(x, i, z, (int)(c.Average() * 8) % 4 + 1);
						voxelMap->SetVheight(x, i, z, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
						voxelMap->SetLighting(x, i, z, 0);
					}

					// gather nearby values to get height
					VoxelHeight heights[4];
					float heightValues[4] = {
						(float)(y - sw) / 2.0,
						(float)(y - se) / 2.0,
						(float)(y - nw) / 2.0,
						(float)(y - ne) / 2.0,
					};
					for (unsigned j = 0; j < 4; ++j)
					{
						float hv = heightValues[j];
						if (hv < -0.5)
							heights[j] = VOXEL_HEIGHT_0;
						else if (hv < 0.0)
							heights[j] = VOXEL_HEIGHT_HALF;
						else if (hv < 0.5)
							heights[j] = VOXEL_HEIGHT_1;
						else
							heights[j] = VOXEL_HEIGHT_1;
					}

					voxelMap->SetBlocktype(x, height, z, (int)(c.Average() * 8) % 4 + 1);
					voxelMap->SetVheight(x, height, z, heights[0], heights[1], heights[2], heights[3]);
					{
						int lightHeight = y / heightFactor;
						int heights[9] = { y, nw, n, ne, w, e, sw, s, se };
						int light = 0;
						for (int litY = -1; litY <= 1; ++litY)
							for (int n = 0; n < 9; ++n)
							{
								int neighborHeight = (heights[n] / heightFactor) + litY;
								light += lightHeight + litY > neighborHeight;
							}

						voxelMap->SetLighting(x, height, z, light * 255 / 27);
						//voxelMap->SetLighting(x, height, z, 255);
					}
						

					//voxelMap->SetVheight(x, height, z, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
				}
			}
        }
    }

	// loop through again and set neighbors
	for (int a = 0; a < numX; ++a)
	{
		for (int b = 0; b < numZ; ++b)
		{
			Node* node = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b));
			Node* north = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b+1));
			Node* south = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b-1));
			Node* east  = voxelNode_->GetChild("VoxelChunk_" + String(a+1) + "_" + String(b));
			Node* west  = voxelNode_->GetChild("VoxelChunk_" + String(a-1) + "_" + String(b));
			VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
			VoxelChunk* northChunk = north ? north->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* southChunk = south ? south->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* eastChunk = east ? east->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* westChunk = west ? west->GetComponent<VoxelChunk>() : 0;
			chunk->SetNeighbors(northChunk, southChunk, eastChunk, westChunk);
			chunk->Build();
		}
	}

	for (int a = 0; a < numX; ++a)
	{
		for (int b = 0; b < numZ; ++b)
		{
			Node* node = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b));
			VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
			//chunk->SetVoxelMap(0);
		}
	}
#endif

#if 0
	int btype = 1;
    for (unsigned a = 0; a < 2; ++a)
    {
		unsigned chunkX = a * 64;
        for (unsigned b = 0; b < 2; ++b)
        {
			unsigned chunkZ = b * 64;
			String chunkName = "VoxelChunk_" + String(a) + "_" + String(b);
			Node* node = voxelNode_->CreateChild(chunkName);
			node->SetPosition(Vector3(a * 64, 0, b * 64));
			VoxelChunk* chunk = node->CreateComponent<VoxelChunk>();
			SharedPtr<VoxelMap> voxelMap(new VoxelMap(context_));
			chunk->SetVoxelMap(voxelMap);
			voxelMap->SetSize(64, 64, 64);
			voxelMap->InitializeBlocktype();
			//voxelMap->InitializeVHeight();
			voxelMap->blocktypeMap = voxelBlocktypeMap_;
			for (unsigned x = 0; x < 64; ++x)
			{
				for (unsigned y = 0; y < 10; ++y)
				{ 
					for (unsigned z = 0; z < 64; ++z)
					{
						voxelMap->SetBlocktype(x, y, z, btype);
						//voxelMap->SetVheight(x, i, z, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1, VOXEL_HEIGHT_1);
					}
				}
			}
			btype++;
        }
    }

	// loop through again and set neighbors
	for (int a = 0; a < 2; ++a)
	{
		for (int b = 0; b < 2; ++b)
		{
			Node* node = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b));
			Node* north = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b+1));
			Node* south = voxelNode_->GetChild("VoxelChunk_" + String(a)   + "_" + String(b-1));
			Node* east  = voxelNode_->GetChild("VoxelChunk_" + String(a+1) + "_" + String(b));
			Node* west  = voxelNode_->GetChild("VoxelChunk_" + String(a-1) + "_" + String(b));
			VoxelChunk* chunk = node->GetComponent<VoxelChunk>();
			VoxelChunk* northChunk = north ? north->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* southChunk = south ? south->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* eastChunk = east ? east->GetComponent<VoxelChunk>() : 0;
			VoxelChunk* westChunk = west ? west->GetComponent<VoxelChunk>() : 0;
			chunk->SetNeighbors(northChunk, southChunk, eastChunk, westChunk);
			chunk->Build();
		}
	}
#endif


    counter_++;
	// builder->CompleteWork();
	//builder->BuildVoxelChunk(chunk, voxelMap);
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

	if (drawDebug_)
		scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);

}


