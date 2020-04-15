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
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/RigidBody.h>

#include "PodVehicle.h"
#include "CircuitComponent.h"
#include "DebugCircuitComponent.h"


class SectorComponent : public LogicComponent
{
    URHO3D_OBJECT(SectorComponent, LogicComponent)

public:
    /// Construct.
    explicit SectorComponent(Context* context)
        : LogicComponent(context) {}

    /// Register object factory and attributes.
    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<SectorComponent>();
    }

    void SetInfo(PodCircuit* c, int sectorIndex)
    {
        circ_ = c;
        sectorIndex_ = sectorIndex;
    }

    void PostUpdate(float dt) override
    {
        auto& modelGen = circ_->GetSectorModelGen(sectorIndex_);
        modelGen.UpdateTexCoords();
    }

private:
    int sectorIndex_;
    PodCircuit* circ_;
};

class FaceAnimComponent : public LogicComponent
{
    URHO3D_OBJECT(FaceAnimComponent, LogicComponent)

public:
    /// Construct.
    explicit FaceAnimComponent(Context* context)
        : LogicComponent(context) {}

    /// Register object factory and attributes.
    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<FaceAnimComponent>();

        URHO3D_ATTRIBUTE("TexIndex", int, texIndex_, 0.0f, AM_DEFAULT);
        URHO3D_ATTRIBUTE("TexCoord", Matrix4, texCoord_, Matrix4(), AM_DEFAULT);
        URHO3D_ATTRIBUTE("FlagUpdate", bool, flagUpdate_, false, AM_DEFAULT);
    }

    void SetInfo(PodCircuit* c, int sectorIndex, int faceIndex, int faceType)
    {
        circ_ = c;
        sectorIndex_ = sectorIndex;
        faceIndex_ = faceIndex;
        faceType_ = faceType;
        prevFlagUpdate_ = flagUpdate_;
    }

    void Update(float dt) override
    {
        if (flagUpdate_ != prevFlagUpdate_)
        {
            auto& sector = circ_->GetSector(sectorIndex_);
            FaceData* face;
            if (faceType_ == 3)
                face = sector.Object.TriFaces[faceIndex_];
            else
                face = sector.Object.QuadFaces[faceIndex_];

            face->UseAnimTexCoord = true;
            face->AnimTexCoord[0] = Vector2(texCoord_.m00_, texCoord_.m01_);
            face->AnimTexCoord[1] = Vector2(texCoord_.m10_, texCoord_.m11_);
            face->AnimTexCoord[2] = Vector2(texCoord_.m20_, texCoord_.m21_);
            face->AnimTexCoord[3] = Vector2(texCoord_.m30_, texCoord_.m31_);

            auto& modelGen = circ_->GetSectorModelGen(sectorIndex_);
            modelGen.InvalidateTexCoords();
        }
        prevFlagUpdate_ = flagUpdate_;
    }

private:
    Matrix4 texCoord_;
    int texIndex_;
    int sectorIndex_;
    int faceIndex_;
    int faceType_;
    PodCircuit* circ_;
    float time_ = 0;
    bool flagUpdate_ = false;
    bool prevFlagUpdate_ = false;
};


static void CreateLight(const SharedPtr<Node>& parent, const PodCircuit::Light& data)
{
    auto node = parent->CreateChild();
    node->SetPosition(data.Position);
    node->Rotate(Quaternion(data.Rotation));

    auto light = node->CreateComponent<Light>();
    light->SetCastShadows(true);
    light->SetEnabled(true);

    if (data.Type == 2)
    {
        light->SetLightType(LIGHT_POINT);
        light->SetRange(100.0f); // TODO: figure out range param?
    }
    else if (data.Type == 1)
    {
        light->SetLightType(LIGHT_SPOT);
        light->SetRange(200.0f);
        light->SetFov(90.0f);
    }
    
    light->SetColor(Color::RED); // TODO: color param?
}

CircuitComponent::CircuitComponent(Context* context) :
    LogicComponent(context)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    // SetUpdateEventMask(USE_FIXEDUPDATE);
}

void CircuitComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<CircuitComponent>();
    DebugCircuitComponent::RegisterObject(context);
    SectorComponent::RegisterObject(context);
    FaceAnimComponent::RegisterObject(context);
}

void CircuitComponent::SetPodCircuit(PodCircuit* circ)
{
    circ_ = circ;

    if (!circObjectsGroup_)
        circObjectsGroup_ = GetNode()->CreateChild("Objects");

    circObjectsGroup_->RemoveAllChildren();
    sectors_.Clear();

    for (unsigned int i = 0; i < circ->GetSectors().Size(); i++)
    {
        auto node = circObjectsGroup_->CreateChild();
        node->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

        auto modelObj = node->CreateComponent<StaticModel>();
        modelObj->SetModel(circ->GetSectorModel(i));
        //modelObj->SetCastShadows(true);

        int mi = 0;
        for (const auto& mat : circ->GetSectorMaterials(i))
            modelObj->SetMaterial(mi++, mat);

        // Create collision
        //node->CreateComponent<RigidBody>()->SetCollisionLayer(2);
        //auto colObj = node->CreateComponent<CollisionShape>();
        //colObj->SetTriangleMesh(circ->GetSectorModel(i));

        // Create sector lights
        auto lights = circ->GetSectorLights(i);
        if (lights)
        {
            for (const auto& light : *lights)
                CreateLight(circObjectsGroup_, light);
        }

        auto sector = node->CreateComponent<SectorComponent>();
        sector->SetInfo(circ_, i);
        sectors_.Push(sector);
    }

    for (const auto& dec : circ->GetDecorationInstances())
    {
        auto node = circObjectsGroup_->CreateChild();
        node->SetPosition(dec.Position);
        node->Rotate(Quaternion(dec.Rotation));
        // TODO: rotation

        auto modelObj = node->CreateComponent<StaticModel>();
        modelObj->SetModel(circ->GetDecorationModel(dec.Index));
        //modelObj->SetCastShadows(true);
        
        int i = 0;
        for (const auto& mat : circ->GetDecorationMaterials(dec.Index))
            modelObj->SetMaterial(i++, mat);
    }

    for (const auto& light : circ->GetGlobalLights())
        CreateLight(circObjectsGroup_, light);

    for (const auto& anim : circ->GetTextureAnimations())
    {
        for (const auto& obj : anim.SectorAnimations)
        {
            auto& texAnim = anim.Animations[obj.AnimIndex];
            for (const auto& sec : obj.Sectors)
            {
                SharedPtr<ValueAnimation> texIndexAnim(new ValueAnimation(context_));
                SharedPtr<ValueAnimation> texCoordAnim(new ValueAnimation(context_));
                SharedPtr<ValueAnimation> flagAnim(new ValueAnimation(context_));

                bool flagValue = true;
                for (const auto& frame : texAnim.Frames)
                {
                    auto& key = texAnim.Keys[frame.KeyIndex];
                    texIndexAnim->SetKeyFrame(frame.Time, key.TextureIndex);

                    Matrix4 mat(
                        key.TexCoord[0].x_, key.TexCoord[0].y_, 0, 0,
                        key.TexCoord[1].x_, key.TexCoord[1].y_, 0, 0,
                        key.TexCoord[2].x_, key.TexCoord[2].y_, 0, 0,
                        key.TexCoord[3].x_, key.TexCoord[3].y_, 0, 0
                    );
                    texCoordAnim->SetKeyFrame(frame.Time, mat);
                    flagAnim->SetKeyFrame(frame.Time, flagValue);
                    flagValue = !flagValue;
                }

                auto sector = sectors_[sec.SectorIndex];
                for (const auto& face : sec.Faces)
                {
                    // TODO: looping
                    auto faceNode = sector->GetNode()->CreateChild();
                    auto faceAnim = faceNode->CreateComponent<FaceAnimComponent>();
                    faceAnim->SetInfo(circ_, sec.SectorIndex, face.FaceIndex, face.FaceType);
                    faceAnim->SetAttributeAnimation("TexIndex", texIndexAnim);
                    faceAnim->SetAttributeAnimation("TexCoord", texCoordAnim);
                    faceAnim->SetAttributeAnimation("FlagUpdate", flagAnim);
                }
            }
        }
    }

    auto debug = GetNode()->GetOrCreateComponent<DebugCircuitComponent>();
    debug->SetPodCircuit(circ);
}