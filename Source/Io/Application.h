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

#pragma once

#undef URHO3D_ANGELSCRIPT

#include <Urho3D/Engine/Application.h>
#include "ImGuiIntegration.h"

class PodVehicle;
class PodCircuit;
class VehicleComponent;
class CircuitComponent;
class PodApplication;
 
using namespace Urho3D;

namespace Urho3D
{
class UIElement;
class Scene;
class Node;
class Console;
class DebugHud;
class StaticModel;
class Text;
}

static PodApplication* g_app;

/// PodApplication application runs a script specified on the command line.
class PodApplication : public Application
{
    URHO3D_OBJECT(PodApplication, Application);

public:
    /// Construct.
    explicit PodApplication(Context* context);

    /// Setup before engine initialization. Verify that a script file has been specified.
    void Setup() override;
    /// Setup after engine initialization. Load the script and execute its start function.
    void Start() override;
    /// Cleanup after the main loop. Run the script's stop function if it exists.
    void Stop() override;

    String GetCircuitName();

    /// Create static scene content.
    void CreateScene();
    /// Create the vehicle.
    void CreateVehicle();
    /// Run the script
    void RunScript();
    /// Read input and move the camera.
    void MoveCamera(float timeStep);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    void HandleCircuitSelected(StringHash eventType, VariantMap& eventData);

    void HandleImGuiFrame(StringHash eventType, VariantMap& eventData);

    void LoadCircuit(const String& fileName);

    /// Script file name.
    String scriptFileName_;
    /// Flag whether CommandLine.txt was already successfully read.
    bool commandLineRead_;

    SharedPtr<UIElement> uiRoot_;

    SharedPtr<Scene> scene_;

    SharedPtr<Node> cameraNode_;

    SharedPtr<VehicleComponent> vehicle_;

    SharedPtr<Console> console_;

    SharedPtr<DebugHud> debugHud_;

    PodVehicle* podv_;

    UniquePtr<PodCircuit> circ_;

    SharedPtr<CircuitComponent> circObject_;

    SharedPtr<Text> camPosText_;

    SharedPtr<ImGuiIntegration> imgui_;

    /// Camera yaw angle.
    float yaw_;
    /// Camera pitch angle.
    float pitch_;
};

PodApplication* GetApplication();