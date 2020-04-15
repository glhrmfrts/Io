//
// Copyright (c) 2008-2020 the Urho3D project.
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

#define URHO3D_LUA

#ifdef URHO3D_ANGELSCRIPT
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/AngelScript/Script.h>
#endif
#include <Urho3D/Core/Main.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#ifdef URHO3D_LUA
#include <Urho3D/LuaScript/LuaScript.h>
#endif
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/ToolTip.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/FileSelector.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <cmath>
#include "imgui.h"

#include "Application.h"
#include "PodVehicle.h"
#include "PodCircuit.h"
#include "VehicleComponent.h"
#include "CircuitComponent.h"
#include "LoadBindings.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(PodApplication);

PodApplication* GetApplication()
{
    return g_app;
}

PodApplication::PodApplication(Context* context) :
    Application(context),
    commandLineRead_(false)
{
    g_app = this;
    // Register factory and attributes for the Vehicle component so it can be created via CreateComponent, and loaded / saved
    VehicleComponent::RegisterObject(context);
    CircuitComponent::RegisterObject(context);
}

void PodApplication::Setup()
{
    auto* filesystem = GetSubsystem<FileSystem>();

    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = filesystem->GetCurrentDir();

    // On Web platform setup a default windowed resolution similar to the executable samples
    engineParameters_[EP_FULL_SCREEN]  = false;

    engineParameters_[EP_WINDOW_WIDTH] = 1280;
    engineParameters_[EP_WINDOW_HEIGHT] = 760;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;

    // Construct a search path to find the resource prefix with two entries:
    // The first entry is an empty path which will be substituted with program/bin directory -- this entry is for binary when it is still in build tree
    // The second and third entries are possible relative paths from the installed program/bin directory to the asset directory -- these entries are for binary when it is in the Urho3D SDK installation location
    //if (!engineParameters_.Contains(EP_RESOURCE_PREFIX_PATHS))
        //engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../share/Resources;../share/Urho3D/Resources";
}

void PodApplication::Start()
{
    imgui_ = new ImGuiIntegration(context_);
    SubscribeToEvent(E_IMGUI_NEWFRAME, URHO3D_HANDLER(PodApplication, HandleImGuiFrame));

    auto* cache = GetSubsystem<ResourceCache>();

    GetSubsystem<Graphics>()->Maximize();

    pitch_ = 0;
    yaw_ = 0;

    GetSubsystem<Input>()->SetMouseVisible(true);
    uiRoot_ = GetSubsystem<UI>()->GetRoot();
    uiRoot_->SetVisible(true);

    auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);

    // Create console
    console_ = engine_->CreateConsole();
    console_->SetDefaultStyle(style);
    console_->GetBackground()->SetOpacity(0.8f);
    //console_->SetVisible(true);

    // Create debug HUD.
    debugHud_ = engine_->CreateDebugHud();
    debugHud_->SetDefaultStyle(style);

    podv_ = new PodVehicle(context_, "Io/DATA/BINARY/VOITURES/GAMMA.BV4");
    podv_->Load();
    printf("%s\n", podv_->GetName().CString());

    auto model = podv_->GetChassisModel(VC_GOOD);

    auto* img = uiRoot_->CreateChild<Sprite>();
    auto* tex = podv_->GetChassisMaterials(VC_GOOD)[0]->GetTexture(TU_DIFFUSE);
    img->SetTexture(tex);
    img->SetSize(tex->GetWidth(), tex->GetHeight());
    //img->SetBlendMode(BLEND_ADD);
    img->SetHorizontalAlignment(HA_LEFT);
    img->SetVerticalAlignment(VA_BOTTOM);
    img->SetPosition(0, -256);
    img->SetVisible(true);
    //img->SetPosition(Vector2(50, 50));
    //uiRoot_->AddChild(img);

    CreateScene();

    camPosText_ = uiRoot_->CreateChild<Text>();
    camPosText_->SetHorizontalAlignment(HA_CENTER);
    camPosText_->SetVerticalAlignment(VA_BOTTOM);
    camPosText_->SetColor(Color::WHITE);
    camPosText_->SetVisible(true);
    camPosText_->SetPosition(IntVector2(0.0f, -40.0f));
    camPosText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"));
    camPosText_->SetText("Camera Pos: " + String(cameraNode_->GetPosition()));

    CreateVehicle();

    RunScript();

    // Subscribe to Update event for setting the vehicle controls before physics simulation
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PodApplication, HandleUpdate));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PodApplication, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    // UnsubscribeFromEvent(E_SCENEUPDATE);
}

void PodApplication::RunScript()
{
    scriptFileName_ = "Io/Scripts/Editor.lua";

#ifdef URHO3D_LUA
    // Instantiate and register the Lua script subsystem
    auto* luaScript = new LuaScript(context_);
    context_->RegisterSubsystem(luaScript);
    LoadBindings(luaScript->GetState());

    SubscribeToEvent("CircuitSelected", URHO3D_HANDLER(PodApplication, HandleCircuitSelected));

    // If script loading is successful, proceed to main loop
    if (luaScript->ExecuteFile(scriptFileName_))
    {
        luaScript->ExecuteFunction("Start");
        return;
    }
#else
    ErrorExit("Lua is not enabled!");
    return;
#endif
}

void PodApplication::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node(context_);
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, -20.0f));

    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(500.0f);
    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);
    light->SetColor(Color(0.5f, 0.5f, 0.5f));

    /*
    // Create heightmap terrain with collision
    Node* terrainNode = scene_->CreateChild("Terrain");
    terrainNode->SetPosition(Vector3::ZERO);
    auto* terrain = terrainNode->CreateComponent<Terrain>();
    terrain->SetPatchSize(64);
    terrain->SetSpacing(Vector3(2.0f, 0.1f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
    terrain->SetSmoothing(true);
    terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
    terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
    // The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
    // terrain patches and other objects behind it
    terrain->SetOccluder(true);    

    auto* body = terrainNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry
    auto* shape = terrainNode->CreateComponent<CollisionShape>();
    shape->SetTerrain();
    */
    /*
    // Create 1000 mushrooms in the terrain. Always face outward along the terrain normal
    const unsigned NUM_MUSHROOMS = 1000;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* objectNode = scene_->CreateChild("Mushroom");
        Vector3 position(Random(2000.0f) - 1000.0f, 0.0f, Random(2000.0f) - 1000.0f);
        position.y_ = 0.0f;// terrain->GetHeight(position) - 0.1f;
        objectNode->SetPosition(position);
        // Create a rotation quaternion from up vector to terrain normal
        //objectNode->SetRotation(Quaternion(Vector3::UP, terrain->GetNormal(position)));
        objectNode->SetScale(3.0f);
        auto* object = objectNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        object->SetCastShadows(true);

        auto* body = objectNode->CreateComponent<RigidBody>();
        body->SetCollisionLayer(2);
        auto* shape = objectNode->CreateComponent<CollisionShape>();
        shape->SetTriangleMesh(object->GetModel(), 0);
    }
    */

    //scene_->GetComponent<PhysicsWorld>()->SetGravity(Vector3(0.0f, 0.0f, 0.0f));
}

void PodApplication::CreateVehicle()
{
    Node* vehicleNode = scene_->CreateChild("Vehicle");
    vehicleNode->SetPosition(Vector3(-208.0f, 72.0f, -73.0f));

    // Create the vehicle logic component
    vehicle_ = vehicleNode->CreateComponent<VehicleComponent>();
    // Create the rendering and physics components
    vehicle_->SetPodVehicle(podv_);
}

void PodApplication::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    if (input->GetKeyDown(KEY_ESCAPE))
        engine_->Exit();

    // Movement speed as world units per second
    const float MOVE_SPEED = 100.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.4f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    }
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

#if 0
    if (input->GetKeyDown(KEY_RIGHT))
        yaw_ += 90.0f * timeStep;
    else if (input->GetKeyDown(KEY_LEFT))
        yaw_ += -90.0f * timeStep;

    if (input->GetKeyDown(KEY_DOWN))
        pitch_ += 90.0f * timeStep;
    else if (input->GetKeyDown(KEY_UP))
        pitch_ += -90.0f * timeStep;
#endif

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void PodApplication::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    auto* input = GetSubsystem<Input>();

    if (vehicle_)
    {
        auto* ui = GetSubsystem<UI>();

        // Get movement controls and assign them to the vehicle component. If UI has a focused element, clear controls
        if (!ui->GetFocusElement())
        {
            vehicle_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_UP));
            vehicle_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_DOWN));
            vehicle_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_LEFT));
            vehicle_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_RIGHT));

            // Add yaw & pitch from the mouse motion or touch input. Used only for the camera, does not affect motion
            if (false)
            {
                for (unsigned i = 0; i < input->GetNumTouches(); ++i)
                {
                    TouchState* state = input->GetTouch(i);
                    if (!state->touchedElement_)    // Touch on empty space
                    {
                        auto* camera = cameraNode_->GetComponent<Camera>();
                        if (!camera)
                            return;

                        auto* graphics = GetSubsystem<Graphics>();
                        vehicle_->controls_.yaw_ += 0.2f * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                        vehicle_->controls_.pitch_ += 0.2f * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
                    }
                }
            }
            else
            {
                vehicle_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
                vehicle_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
            }
            // Limit pitch
            vehicle_->controls_.pitch_ = Clamp(vehicle_->controls_.pitch_, 0.0f, 80.0f);
        }
        else
            vehicle_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT, false);
    }

    {
        // Check for loading / saving the scene
        if (input->GetKeyPress(KEY_F5))
        {
            File saveFile(context_, GetSubsystem<FileSystem>()->GetCurrentDir() + "Data/Scenes/PodCircuit.xml",
                FILE_WRITE);
            scene_->SaveXML(saveFile);
        }
        if (input->GetKeyPress(KEY_F7))
        {
            File loadFile(context_, GetSubsystem<FileSystem>()->GetCurrentDir() + "Data/Scenes/VehicleDemo.xml", FILE_READ);
            scene_->LoadXML(loadFile);
        }
    }

    float timeStep = eventData[P_TIMESTEP].GetFloat();

    MoveCamera(timeStep);
    camPosText_->SetText("Camera Pos: " + String(cameraNode_->GetPosition()));

    if (input->GetKeyPress(KEY_F1))
        console_->SetVisible(!console_->IsVisible());
    if (input->GetKeyPress(KEY_F2))
        debugHud_->ToggleAll();
}

void PodApplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    if (!vehicle_)
        return;

    //Node* vehicleNode = vehicle_->GetNode();

#if 0
    // Physics update has completed. Position camera behind vehicle
    Quaternion dir(vehicleNode->GetRotation().YawAngle(), Vector3::UP);
    dir = dir * Quaternion(vehicle_->controls_.yaw_, Vector3::UP);
    dir = dir * Quaternion(vehicle_->controls_.pitch_, Vector3::RIGHT);

    Vector3 cameraTargetPos = vehicleNode->GetPosition() - dir * Vector3(0.0f, 0.0f, 20.0f);
    Vector3 cameraStartPos = vehicleNode->GetPosition();

    // Raycast camera against static objects (physics collision mask 2)
    // and move it closer to the vehicle if something in between
    Ray cameraRay(cameraStartPos, cameraTargetPos - cameraStartPos);
    float cameraRayLength = (cameraTargetPos - cameraStartPos).Length();
    PhysicsRaycastResult result;
    scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, cameraRay, cameraRayLength, 2);
    if (result.body_)
        cameraTargetPos = cameraStartPos + cameraRay.direction_ * (result.distance_ - 0.5f);

    cameraNode_->SetPosition(cameraTargetPos);
    cameraNode_->SetRotation(dir);
#endif
}

void PodApplication::HandleCircuitSelected(StringHash eventType, VariantMap& eventData)
{
    String fileName = eventData["circuit_file"].GetString();
    LoadCircuit(fileName);
}

String PodApplication::GetCircuitName()
{
    return circ_->GetProjectName();
}

void PodApplication::LoadCircuit(const String& fileName)
{
    if (fileName.Empty()) return;

    circ_ = new PodCircuit(context_, fileName);
    if (!circ_->Load())
        return;

    if (!circObject_)
    {
        auto circNode = scene_->CreateChild("Circuit");
        circNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        circObject_ = circNode->CreateComponent<CircuitComponent>();
    }
    else
    {
        auto circNode = scene_->GetChild("Circuit");
        circObject_ = circNode->GetComponent<CircuitComponent>();
    }
    circObject_->SetPodCircuit(circ_.Get());
}

static void ImGuiSliderRotation(const Matrix3& mat)
{
    float* rot = (float*)&mat;
    ImGui::SliderFloat3("Rotation[0]", rot, -2.0f, 2.0f);
    ImGui::SliderFloat3("Rotation[1]", rot + 3, -2.0f, 2.0f);
    ImGui::SliderFloat3("Rotation[2]", rot + 6, -2.0f, 2.0f);
}

static void ImGuiGoto(const Vector3& pos)
{
    if (ImGui::Button("Goto"))
    {
        g_app->cameraNode_->SetPosition(pos + Vector3(20.0f, 20.0f, 20.0f));
        g_app->cameraNode_->LookAt(pos);
    }
}

static void ImGuiLight(const PodCircuit::Light& l, int sectorIndex = -1)
{
    String label = "Light";
    if (l.Type == 1)
        label = "SpotLight";
    if (l.Type == 0)
        label = "ConeLight";
    if (ImGui::TreeNodeEx(&l, 0, label.CString()))
    {
        ImGuiGoto(l.Position);
        ImGui::SliderFloat3("Position", (float*)&l.Position, -1000.0f, 1000.0f);
        ImGuiSliderRotation(l.Rotation);
        ImGui::SliderInt("Diffusion", (int*)&l.Diffusion, 0, 10000000);
        ImGui::SliderInt("Value1", (int*)&l.Value1, 0, 10000000);
        ImGui::SliderInt("Value2", (int*)&l.Value2, 0, 10000000);
        ImGui::SliderInt("Value3", (int*)&l.Value3, 0, 10000000);
        if (sectorIndex != -1) ImGui::Text("Sector %d", sectorIndex);
        ImGui::TreePop();
    }
}

static void ImGuiBytes(unsigned char* data, int count)
{
    static int mode = 0;
    if (ImGui::Button("Bytes"))
    {
        mode = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Ints"))
    {
        mode = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Floats"))
    {
        mode = 2;
    }
    ImGui::NewLine();

    if (mode == 0)
    {
        for (int j = 0; j < count; j++)
        {
            if (j > 0 && j % 4 == 0)
            {
                ImGui::Text(" ");
                ImGui::SameLine();
            }
            unsigned char b = data[j];
            ImGui::Text("%x", b & 0xFF); ImGui::SameLine();
        }
    }
    else if (mode == 1)
    {
        for (int j = 0; j < count; j += sizeof(int))
        {
            int* num = reinterpret_cast<int*>(data+j);
            ImGui::Text("%d ", *num); ImGui::SameLine();
        }
    }
    else if (mode == 2)
    {
        for (int j = 0; j < count; j += sizeof(fp1616_t))
        {
            fp1616_t* num = reinterpret_cast<fp1616_t*>(data + j);
            float f = FloatFromFP1616(*num);
            ImGui::Text("%.2f ", f); ImGui::SameLine();
        }
    }

    ImGui::NewLine();
    ImGui::Spacing();
}

static int debug_reverbMacro1;
static int debug_reverbMacro2;

static void ImGuiMacros(const Vector<PodCircuit::Macro>& macros)
{
    int i = 0;
    for (const auto& m : macros)
    {
        if (ImGui::TreeNodeEx(&m, 0, "Macro %d", i))
        {
            ImGui::Text("ev: %d (%s) ", m.EventIndex, g_app->circ_->events_[m.EventIndex].Name.CString()); ImGui::SameLine();
            ImGui::Text("param: %d  ", m.ParamIndex); ImGui::SameLine();
            ImGui::Text("next: %d ", m.NextMacro); ImGui::SameLine();
            ImGui::NewLine();
            ImGui::TreePop();
        }
        i++;
    }
}

static void ImGuiMacros(const Vector<PodCircuit::Macro1>& macros)
{
    int i = 0;
    for (const auto& m : macros)
    {
        if (ImGui::TreeNodeEx(&m, 0, "Macro %d", i))
        {
            ImGui::Text("Params: "); ImGui::SameLine();
            ImGui::Text("%d ", m.Value1); ImGui::SameLine();
            ImGui::NewLine();
            ImGui::TreePop();
        }
        i++;
    }
}

static void ImGuiMacros(const Vector<PodCircuit::Macro2>& macros)
{
    int i = 0;
    for (const auto& m : macros)
    {
        if (ImGui::TreeNodeEx(&m, 0, "Macro %d", i))
        {
            ImGui::Text("Params: "); ImGui::SameLine();
            ImGui::Text("%d ", m.Value1); ImGui::SameLine();
            ImGui::Text("%d ", m.Value2); ImGui::SameLine();
            ImGui::NewLine();
            ImGui::TreePop();
        }
        i++;
    }
}

static void ImGuiDesignation(PodCircuit::Designation& d)
{
    ImGui::Text("Value1: %d", d.Value1);
    ImGui::Text("Value2: %d", d.Value2);
    ImGui::Text("Value3: %d", d.Value3);
    ImGui::Text("Value4: %d", d.Value4);
    ImGui::Text("Value5: %d", d.Value5);
    ImGui::Text("Value6: %d", d.Value6);
    ImGui::Text("Value7: %d", d.Value7);
    ImGui::Text("Value8: %d", d.Value8);
    ImGui::Text("Value9: %d", d.Value9);
    ImGui::Text("ValueA[0]: %d", d.ValueA[0]);
    ImGui::Text("ValueA[1]: %d", d.ValueA[1]);
    ImGui::Text("ValueA[2]: %d", d.ValueA[2]);
    ImGui::Text("ValueA[3]: %d", d.ValueA[3]);
    ImGui::Text("ValueA[4]: %d", d.ValueA[4]);

    ImGui::Text("MacroSection.Name: %s", d.MacroSection.Name.CString());
    if (ImGui::TreeNode("MacroSection.Macros"))
    {
        ImGuiMacros(d.MacroSection.Macros);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("MacroSection.DesignationMacros"))
    {
        int si = 0;
        for (auto& macro : d.MacroSection.DesignationMacros)
        {
            if (ImGui::TreeNodeEx(&macro, 0, "DesignationMacro %d", si))
            {
                ImGui::Text("MacroIndex1: %d", macro.MacroIndex1);
                ImGui::Text("MacroIndex2: %d", macro.MacroIndex2);
                ImGui::Text("Pos: (%s) (%s) (%s) (%s)", macro.PlanePos1.ToString().CString(), macro.PlanePos2.ToString().CString(),
                    macro.PlanePos3.ToString().CString(), macro.PlanePos4.ToString().CString());
                ImGui::Text("Normal: (%s)", macro.PlaneNormal.ToString().CString());
                ImGui::TreePop();
            }
            si++;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("MacroSection.DesignationValues"))
    {
        int si = 0;
        for (auto& v : d.MacroSection.DesignationValues)
        {
            if (ImGui::TreeNodeEx(&v, 0, "DesignationValue %d", si))
            {
                for (int i = 0; i < 8; i++)
                    ImGui::Text("Data[%d]: %d", i, v.Data[i]);
                ImGui::TreePop();
            }
            si++;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("PhaseMacros"))
    {
        ImGuiMacros(d.PhaseMacros);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Phases"))
    {
        int pi = 0;
        for (auto& phase : d.Phases)
        {
            ImGui::Text("Phase[%d].Name: %s", pi, phase.Name.CString());
            ImGui::Text("Phase[%d].MacroIndex: %d", pi, phase.MacroIndex);
            ImGui::Spacing();
            pi++;
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Starts"))
    {
        int si = 0;
        for (auto& start : d.Starts)
        {
            ImGui::Text("Start[%d]: ", si++);
            ImGuiBytes(start.Data, 36);
        }
        ImGui::TreePop();
    }
}

String debug_enabledDifficulty;
HashMap<String, Vector<int>> debug_enabledPointLists;
HashMap<String, bool> debug_enabledAllPositions;
static bool debug_reloadObjects;

static void ImGuiDifficulty(PodCircuit::Difficulty& d)
{
    auto& enabledPointLists = debug_enabledPointLists[d.Path.Name];
    if (!enabledPointLists.Size())
        enabledPointLists.Resize(d.Path.PointLists.Size());

    if (ImGui::Button("Set Current To Visualize"))
    {
        debug_enabledDifficulty = d.Path.Name;
        debug_reloadObjects = true;
    }
    ImGui::Text("Name: %s", d.Name.CString());
    ImGui::Text("PathName: %s", d.Path.Name.CString());
    ImGui::Text("LevelName: %s", d.Level.Name.CString());
    ImGui::Text("%d positions", d.Path.Positions.Size());

    unsigned int maxPointUnk1 = 0;
    unsigned int maxPointUnk2 = 0;
    unsigned int maxPointUnk3 = 0;

    int pi = 0;
    for (const auto& plist : d.Path.PointLists)
    {
        ImGui::PushID(pi);

        ImGui::Text("PointList %d", pi); ImGui::SameLine(); ImGui::Text("size: %d", plist.Points.Size()); ImGui::SameLine();

        String btntext = "Enable";
        if (enabledPointLists[pi])
            btntext = "Disable";

        if (ImGui::Button(btntext.CString()))
        {
            if (enabledPointLists[pi])
                enabledPointLists[pi] = 0;
            else
                enabledPointLists[pi] = 1;
            debug_reloadObjects = true;
        }

        if (ImGui::TreeNode("Points"))
        {
            int ppi = 0;
            for (const auto& point : plist.Points)
            {
                if (ImGui::TreeNodeEx(&point, 0, "Point %d", ppi))
                {
                    ImGui::Text("PosIndex: %d", point.PositionIndex);
                    ImGui::Text("Unknown1: %d", point.Unknown1);
                    ImGui::Text("Unknown2: %d", point.Unknown2);
                    ImGui::Text("Unknown3: %d", point.Unknown3);
                    ImGui::TreePop();
                }
                ppi++;
            }
            ImGui::TreePop();
        }
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        {
            int ppi = 0;
            for (const auto& point : plist.Points)
            {
                maxPointUnk1 = max(maxPointUnk1, point.Unknown1);
                maxPointUnk2 = max(maxPointUnk2, point.Unknown2);
                maxPointUnk3 = max(maxPointUnk3, point.Unknown3);
                ppi++;
            }
        }

        pi++;
        ImGui::PopID();
    }

    String alltext = "Enable All Positions";
    if (debug_enabledAllPositions[d.Path.Name])
        alltext = "Disable All Positions";
    if (ImGui::Button(alltext.CString()))
    {
        debug_enabledAllPositions[d.Path.Name] = !debug_enabledAllPositions[d.Path.Name];
        debug_reloadObjects = true;
        for (int& enabled : enabledPointLists)
        {
            if (debug_enabledAllPositions[d.Path.Name]) enabled = 0;
            else enabled = 1;
        }
    }

    if (ImGui::TreeNode("Constraint"))
    {
        ImGui::Text("ConstraintName: %s", d.Path.ConstraintName.CString());
        int ci = 0;
        for (auto& c : d.Path.Constraints)
        {
            if (ImGui::TreeNodeEx(&c, 0, "Constraint %d", ci))
            {
                ImGui::Text("DesignationIndex: %d", c.DesignationIndex);
                ImGui::Text("Unknown1: %d", c.Unknown1);
                ImGui::Text("ConstraintAttack: %d", c.ConstraintAttack);
                ImGui::TreePop();
            }
            ci++;
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Level"))
    {
        ImGui::Text("TrackLength: %.2f", d.Level.TrackLength);
        ImGui::Text("Unknown2: %d", d.Level.Unknown2);
        if (ImGui::TreeNode("Config1s"))
        {
            int ic1 = 0;
            for (auto& c1 : d.Level.Config1s)
            {
                if (ImGui::TreeNodeEx(&c1, 0, "Config1 %d", ic1))
                {
                    ImGui::Text("Position: {%.2f, %.2f, %.2f}", c1.Position.x_, c1.Position.y_, c1.Position.z_);
                    ImGui::Text("RemainingLength: %.2f", c1.RemainingLength);

                    ImGui::Text("Value3: %d", c1.Value3);
                    int pi = 0;
                    for (unsigned int v : c1.Value4)
                    {
                        ImGui::Text("Value4[%d]: %d", pi, v);
                        pi++;
                    }
                    pi = 0;
                    for (unsigned int v : c1.Value5)
                    {
                        ImGui::Text("Value5[%d]: %d", pi, v);
                        pi++;
                    }
                    ImGui::TreePop();
                }
                ic1++;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Config2s"))
        {
            int ic1 = 0;
            for (const auto& c1 : d.Level.Config2s)
            {
                if (ImGui::TreeNodeEx(&c1, 0, "Config2 %d", ic1))
                {
                    ImGui::Text("Value1: %d", c1.Value1);
                    ImGui::Text("Value2: %d", c1.Value2);
                    int pi = 0;
                    for (int v : c1.Value3)
                    {
                        ImGui::Text("Value3[%d]: %d", pi, v);
                        pi++;
                    }
                    pi = 0;
                    ImGui::Text("Value4: %d", c1.Value4);
                    for (float v : c1.Value5)
                    {
                        ImGui::Text("Value5[%d]: %.2f", pi, v);
                        pi++;
                    }
                    ImGui::TreePop();
                }
                ic1++;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Config3s"))
        {
            int ic1 = 0;
            for (const auto& c1 : d.Level.Config3s)
            {
                if (ImGui::TreeNodeEx(&c1, 0, "Config3 %d", ic1))
                {
                    ImGui::Text("Value1: %d", c1.Value1);
                    ImGui::Text("Value2: %d", c1.Value2);
                    ImGui::Text("Value3: %d", c1.Value3);
                    ImGui::TreePop();
                }
                ic1++;
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Investigations"))
    {
        ImGui::Text("maxPointUnk1: %d", maxPointUnk1);
        ImGui::Text("maxPointUnk2: %d", maxPointUnk2);
        ImGui::Text("maxPointUnk3: %d", maxPointUnk3);
        ImGui::TreePop();
    }
}

static void ImGuiCompetitors(PodCircuit::CompetitorSection& sec)
{
    ImGui::Text("DifficultyName: %s", sec.DifficultyName.CString());
    ImGui::Text("Name: %s", sec.DifficultyName.CString());
    int ci = 0;
    for (auto& comp : sec.Competitors)
    {
        if (ImGui::TreeNodeEx(&comp, 0, "Competitor %d", ci++))
        {
            ImGui::Text("Name: %s", comp.Name.CString());
            ImGui::Text("Number: %s", comp.Number.CString());
            ImGui::Text("Value1: %d", comp.Value1);
            ImGui::Text("Value2: %d", comp.Value2);
            ImGui::Text("Value3: %d", comp.Value3);
            ImGui::TreePop();
        }
    }
}

void PodApplication::HandleImGuiFrame(StringHash eventType, VariantMap& eventData)
{
    if (!circ_) return;

    ImGui::Begin("Circuit Info");
    ImGui::Text("Press enter to load a new circuit");

    if (ImGui::Button("Update Objects"))
        debug_reloadObjects = true;

    if (debug_reloadObjects)
        circObject_->SetPodCircuit(circ_.Get());

    debug_reloadObjects = false;

    ImGui::SameLine();
    if (ImGui::Button("Reload"))
        LoadCircuit(circ_->GetFileName());

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Name: %s", circ_->trackName_.CString());
    ImGui::Text("ProjectName: %s", circ_->projectName_.CString());
    ImGui::Text("LOD: "); ImGui::SameLine();
    for (int i = 0; i < 16; i++)
    {
        ImGui::Text("%d ", circ_->levelOfDetail_[i]); ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::CollapsingHeader("Events"))
    {
        int ei = 0;
        for (const auto& ev : circ_->events_)
        {
            if (ImGui::TreeNodeEx(&ev, 0, "Event %d (%s)", ei++, ev.Name.CString()))
            {
                ImGui::Text("ParamCount: %d", ev.ParamCount);
                ImGui::Text("ParamSize: %d", ev.ParamSize);
                ImGui::Text("ParamData:");
                {
                    for (int i = 0; i < ev.ParamCount; i++)
                    {
                        for (int j = 0; j < ev.ParamSize; j++)
                        {
                            if (j > 0 && j % 4 == 0)
                            {
                                ImGui::Text(" ");
                                ImGui::SameLine();
                            }
                            unsigned char b = ev.ParamData[i * ev.ParamSize + j];
                            ImGui::Text("%x", b & 0xFF); ImGui::SameLine();
                        }
                        ImGui::NewLine();
                    }
                    ImGui::Spacing();
                }
                ImGui::TreePop();
            }
        }
    }
    if (ImGui::CollapsingHeader("Macros"))
    {
        ImGuiMacros(circ_->macrosBase_);
        ImGuiMacros(circ_->macros_);
        ImGuiMacros(circ_->macrosInit_);
        ImGuiMacros(circ_->macrosActive_);
        ImGuiMacros(circ_->macrosInactive_);
        ImGuiMacros(circ_->macrosReplace_);
        ImGuiMacros(circ_->macrosExchange_);
    }
    if (ImGui::CollapsingHeader("Env Macros"))
    {
        ImGuiMacros(circ_->envSection_.Macros);
    }
    if (ImGui::CollapsingHeader("Sectors"))
    {
        unsigned int num = circ_->GetSectors().Size();
        ImGui::Text("%d sectors", num);
    }
    if (ImGui::CollapsingHeader("Decoration"))
    {
        unsigned int num = circ_->GetDecorations().Size();
        ImGui::Text("%d decoration types", num);

        for (const auto& dec : circ_->GetDecorationInstances())
        {
            if (ImGui::TreeNodeEx(&dec, 0, "Decoration %d", dec.Index))
            {
                ImGuiGoto(dec.Position);
                ImGui::SliderFloat3("Position", (float*)&dec.Position, -1000.0f, 1000.0f);
                ImGuiSliderRotation(dec.Rotation);
                ImGui::TreePop();
            }
        }
    }
    if (ImGui::CollapsingHeader("Lights"))
    {
        unsigned int num = circ_->GetGlobalLights().Size();
        ImGui::Text("%d global lights", num);

        for (const auto& light : circ_->GetGlobalLights())
            ImGuiLight(light);

        unsigned int numsec = circ_->GetSectors().Size();
        for (unsigned int i = 0; i < numsec; i++)
        {
            auto lights = circ_->GetSectorLights(i);
            if (lights)
            {
                for (const auto& light : *lights)
                    ImGuiLight(light, i);
            }
        }
    }
    if (ImGui::CollapsingHeader("Sounds"))
    {
        ImGui::Text("Name: %s", circ_->soundSection_.Name.CString());
        int i = 0;
        for (auto& sound : circ_->soundSection_.Sounds)
        {
            if (ImGui::TreeNodeEx(&sound, 0, "Sound %d", i))
            {
                for (int j = 0; j < 14; j++)
                {
                    float f = FloatFromFP1616(sound.Data[j]);
                    ImGui::Text("Data[%d]: %d (float: %.2f)", j, sound.Data[j], f);
                }
                ImGui::TreePop();
            }
            i++;
        }
    }
    if (ImGui::CollapsingHeader("Animation2 (texture animations)"))
    {
        for (const auto& section : circ_->GetTextureAnimations())
        {
            if (ImGui::TreeNode(section.Name.CString()))
            {
                if (ImGui::TreeNode("Animations"))
                {
                    int ai = 0;
                    for (const auto& anim : section.Animations)
                    {
                        if (ImGui::TreeNodeEx(&anim, 0, "Anim %d", ai++))
                        {
                            if (ImGui::TreeNode("Keys"))
                            {
                                int i = 0;
                                for (const auto& key : anim.Keys)
                                {
                                    if (ImGui::TreeNodeEx(&key, 0, "Key %d", i++))
                                    {
                                        ImGui::Text("TextureIndex: %d", key.TextureIndex);
                                        ImGui::SliderFloat2("TexCoord[0]", (float*)&key.TexCoord[0], 0.0f, 1.0f);
                                        ImGui::SliderFloat2("TexCoord[1]", (float*)&key.TexCoord[1], 0.0f, 1.0f);
                                        ImGui::SliderFloat2("TexCoord[2]", (float*)&key.TexCoord[2], 0.0f, 1.0f);
                                        ImGui::SliderFloat2("TexCoord[3]", (float*)&key.TexCoord[3], 0.0f, 1.0f);
                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                            if (ImGui::TreeNode("Frames"))
                            {
                                int i = 0;
                                for (const auto& f : anim.Frames)
                                {
                                    if (ImGui::TreeNodeEx(&f, 0, "Frame %d", i++))
                                    {
                                        ImGui::Text("Time: %.2f", f.Time);
                                        ImGui::Text("KeyIndex: %d", f.KeyIndex);
                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Sector Animations"))
                {
                    int si = 0;
                    for (const auto& sec : section.SectorAnimations)
                    {
                        if (ImGui::TreeNodeEx(&sec, 0, "Sector Anim %d", si++))
                        {
                            ImGui::Text("AnimIndex: %d", sec.AnimIndex);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
    }
    if (ImGui::CollapsingHeader("Designation"))
    {
        if (ImGui::TreeNode("Forward"))
        {
            ImGuiDesignation(circ_->designationForward_);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Reverse"))
        {
            ImGuiDesignation(circ_->designationReverse_);
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Difficulty"))
    {
        ImGui::Text("Current: %s", debug_enabledDifficulty.CString());
        if (ImGui::TreeNode("Forward"))
        {
            if (ImGui::TreeNode("Easy"))
            {
                ImGuiDifficulty(circ_->difficultyForwardEasy_);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Medium"))
            {
                ImGuiDifficulty(circ_->difficultyForwardNormal_);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Hard"))
            {
                ImGuiDifficulty(circ_->difficultyForwardHard_);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Reverse"))
        {
            if (ImGui::TreeNode("Easy"))
            {
                ImGuiDifficulty(circ_->difficultyReverseEasy_);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Medium"))
            {
                ImGuiDifficulty(circ_->difficultyReverseNormal_);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Hard"))
            {
                ImGuiDifficulty(circ_->difficultyReverseHard_);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Competitors"))
    {
        if (ImGui::TreeNode("Easy"))
        {
            ImGuiCompetitors(circ_->competitorsEasy_);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Medium"))
        {
            ImGuiCompetitors(circ_->competitorsNormal_);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Hard"))
        {
            ImGuiCompetitors(circ_->competitorsHard_);
            ImGui::TreePop();
        }
    }
    if (ImGui::CollapsingHeader("Investigations"))
    {
        ImGui::Text("reverb macro 1: %d", debug_reverbMacro1);
        ImGui::Text("reverb macro 2: %d", debug_reverbMacro2);
    }
    ImGui::End();
}

void PodApplication::Stop()
{
    auto* luaScript = GetSubsystem<LuaScript>();
    if (luaScript && luaScript->GetFunction("Stop", true))
        luaScript->ExecuteFunction("Stop");
}