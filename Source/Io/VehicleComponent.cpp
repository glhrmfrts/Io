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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

#include "PodVehicle.h"
#include "VehicleComponent.h"

VehicleComponent::VehicleComponent(Context* context) :
    LogicComponent(context)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void VehicleComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<VehicleComponent>();

    URHO3D_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Steering", float, steering_, 0.0f, AM_DEFAULT);
    // Register wheel node IDs as attributes so that the wheel nodes can be reaquired on deserialization. They need to be tagged
    // as node ID's so that the deserialization code knows to rewrite the IDs in case they are different on load than on save
    URHO3D_ATTRIBUTE("Front Left Node", unsigned, frontLeftID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Front Right Node", unsigned, frontRightID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Rear Left Node", unsigned, rearLeftID_, 0, AM_DEFAULT | AM_NODEID);
    URHO3D_ATTRIBUTE("Rear Right Node", unsigned, rearRightID_, 0, AM_DEFAULT | AM_NODEID);
}

void VehicleComponent::ApplyAttributes()
{
    // This function is called on each Serializable after the whole scene has been loaded. Reacquire wheel nodes from ID's
    // as well as all required physics components
    Scene* scene = GetScene();

    frontLeft_ = scene->GetNode(frontLeftID_);
    frontRight_ = scene->GetNode(frontRightID_);
    rearLeft_ = scene->GetNode(rearLeftID_);
    rearRight_ = scene->GetNode(rearRightID_);
    hullBody_ = node_->GetComponent<RigidBody>();

    GetWheelComponents();
}

void VehicleComponent::FixedUpdate(float timeStep)
{
    float newSteering = 0.0f;
    float accelerator = 0.0f;

    // Read controls
    if (controls_.buttons_ & CTRL_LEFT)
        newSteering = -1.0f;
    if (controls_.buttons_ & CTRL_RIGHT)
        newSteering = 1.0f;
    if (controls_.buttons_ & CTRL_FORWARD)
        accelerator = 1.0f;
    if (controls_.buttons_ & CTRL_BACK)
        accelerator = -0.5f;

    // When steering, wake up the wheel rigidbodies so that their orientation is updated
    if (newSteering != 0.0f)
    {
        frontLeftBody_->Activate();
        frontRightBody_->Activate();
        steering_ = steering_ * 0.95f + newSteering * 0.05f;
    }
    else
        steering_ = steering_ * 0.8f + newSteering * 0.2f;

    // Set front wheel angles
    Quaternion steeringRot(0, steering_ * MAX_WHEEL_ANGLE, 0);
    frontLeftAxis_->SetOtherAxis(steeringRot * Vector3::LEFT);
    frontRightAxis_->SetOtherAxis(steeringRot * Vector3::RIGHT);

    Quaternion hullRot = hullBody_->GetRotation();
    if (accelerator != 0.0f)
    {
        // Torques are applied in world space, so need to take the vehicle & wheel rotation into account
        Vector3 torqueVec = Vector3(ENGINE_POWER * accelerator, 0.0f, 0.0f);

        frontLeftBody_->ApplyTorque(hullRot * steeringRot * torqueVec);
        frontRightBody_->ApplyTorque(hullRot * steeringRot * torqueVec);
        rearLeftBody_->ApplyTorque(hullRot * torqueVec);
        rearRightBody_->ApplyTorque(hullRot * torqueVec);
    }

    // Apply downforce proportional to velocity
    Vector3 localVelocity = hullRot.Inverse() * hullBody_->GetLinearVelocity();
    hullBody_->ApplyForce(hullRot * Vector3::DOWN * Abs(localVelocity.z_) * DOWN_FORCE);
}

void VehicleComponent::SetPodVehicle(PodVehicle* podv)
{
    podv_ = podv;

    // This function is called only from the main program when initially creating the vehicle, not on scene load
    auto* cache = GetSubsystem<ResourceCache>();

    auto* hullObject = node_->CreateComponent<StaticModel>();
    hullBody_ = node_->CreateComponent<RigidBody>();
    auto* hullShape = node_->CreateComponent<CollisionShape>();

    //node_->Rotate(Quaternion(0.0f, 90.0f, 0.0f));

    //node_->SetScale(Vector3(1.5f, 1.0f, 3.0f));
    hullObject->SetModel(podv_->GetChassisModel(VC_GOOD));
    const auto& mats = podv_->GetChassisMaterials(VC_GOOD);
    for (int i = 0; i < mats.Size(); i++)
        hullObject->SetMaterial(i, mats[i]);

    hullObject->SetCastShadows(true);
    hullShape->SetBox(Vector3::ONE);
    hullBody_->SetMass(4.0f);
    hullBody_->SetLinearDamping(0.2f); // Some air resistance
    hullBody_->SetAngularDamping(0.5f);
    hullBody_->SetCollisionLayer(1);

    InitWheel("FrontLeft", VR_FRONT_L, frontLeft_, frontLeftID_);
    InitWheel("FrontRight", VR_FRONT_R, frontRight_, frontRightID_);
    InitWheel("RearLeft", VR_REAR_L, rearLeft_, rearLeftID_);
    InitWheel("RearRight", VR_REAR_R, rearRight_, rearRightID_);

    //Vector3 offset = podv_->GetChassisOffset();
    //node_->SetPosition(node_->LocalToWorld(offset));

    GetWheelComponents();
}

void VehicleComponent::InitWheel(const String& name, PodVehicleRegion reg, WeakPtr<Node>& wheelNode, unsigned& wheelNodeID)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Note: do not parent the wheel to the hull scene node. Instead create it on the root level and let the physics
    // constraint keep it together
    Vector3 offset = podv_->GetWheelOffset(reg);
    Vector3 coffs = podv_->GetChassisOffset()*0.5f;
    auto model = podv_->GetWheelModel(reg);
    const auto& mats = podv_->GetWheelMaterials(reg);

    wheelNode = GetScene()->CreateChild(name);
    wheelNode->SetPosition(node_->LocalToWorld(offset - coffs));
    //wheelNode->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));
    //wheelNode->SetScale(Vector3(0.8f, 0.5f, 0.8f));
    // Remember the ID for serialization
    wheelNodeID = wheelNode->GetID();

    auto* wheelObject = wheelNode->CreateComponent<StaticModel>();
    auto* wheelBody = wheelNode->CreateComponent<RigidBody>();
    auto* wheelShape = wheelNode->CreateComponent<CollisionShape>();
    auto* wheelConstraint = wheelNode->CreateComponent<Constraint>();

    wheelObject->SetModel(model);
    for (int i = 0; i < mats.Size(); i++)
        wheelObject->SetMaterial(i, mats[i]);

    wheelObject->SetCastShadows(true);
    wheelShape->SetSphere(1.0f);
    wheelBody->SetFriction(1.0f);
    wheelBody->SetMass(1.0f);
    wheelBody->SetLinearDamping(0.2f); // Some air resistance
    wheelBody->SetAngularDamping(0.75f); // Could also use rolling friction
    wheelBody->SetCollisionLayer(1);
    wheelConstraint->SetConstraintType(CONSTRAINT_HINGE);
    wheelConstraint->SetOtherBody(GetComponent<RigidBody>()); // Connect to the hull body
    wheelConstraint->SetWorldPosition(wheelNode->GetPosition()); // Set constraint's both ends at wheel's location

    // TODO: investigate axis issues
    wheelConstraint->SetAxis(Vector3::RIGHT); // Wheel rotates around its local Y-axis
    wheelConstraint->SetOtherAxis(offset.x_ >= 0.0 ? Vector3::RIGHT : Vector3::RIGHT); // Wheel's hull axis points either left or right

    wheelConstraint->SetLowLimit(Vector2(-180.0f, 0.0f)); // Let the wheel rotate freely around the axis
    wheelConstraint->SetHighLimit(Vector2(180.0f, 0.0f));
    wheelConstraint->SetDisableCollision(true); // Let the wheel intersect the vehicle hull
}

void VehicleComponent::GetWheelComponents()
{
    frontLeftAxis_ = frontLeft_->GetComponent<Constraint>();
    frontRightAxis_ = frontRight_->GetComponent<Constraint>();
    frontLeftBody_ = frontLeft_->GetComponent<RigidBody>();
    frontRightBody_ = frontRight_->GetComponent<RigidBody>();
    rearLeftBody_ = rearLeft_->GetComponent<RigidBody>();
    rearRightBody_ = rearRight_->GetComponent<RigidBody>();
}
