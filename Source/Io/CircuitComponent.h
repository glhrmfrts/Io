//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include <Urho3D/Scene/LogicComponent.h>
#include "PodCircuit.h"

namespace Urho3D
{

class Constraint;
class Node;
class RigidBody;

}

using namespace Urho3D;

class SectorComponent;

/// Circuit component, responsible for managing physics and rendering components for a circuit.
class CircuitComponent : public LogicComponent
{
    URHO3D_OBJECT(CircuitComponent, LogicComponent)

public:
    /// Construct.
    explicit CircuitComponent(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Perform post-load after deserialization. Acquire the components from the scene nodes.
    /// void ApplyAttributes() override;
    /// Handle physics world update. Called by LogicComponent base class.
    /// void FixedUpdate(float timeStep) override;

    /// Create rendering and physics components. Called by the application.
    void SetPodCircuit(PodCircuit* circ);

private:

    PodCircuit* circ_;

    SharedPtr<StaticModel> circModel_;

    SharedPtr<Node> circObjectsGroup_;

    PODVector<SectorComponent*> sectors_;
};
