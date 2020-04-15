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
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/Font.h>

#include "PodVehicle.h"
#include "DebugCircuitComponent.h"

extern String debug_enabledDifficulty;
extern HashMap<String, Vector<int>> debug_enabledPointLists;
extern HashMap<String, bool> debug_enabledAllPositions;

static Node* CreateDebugModel(SharedPtr<Node>& parent, Model* model, const Vector3& pos, const Vector3& scl, const Color& color,
    FillMode fill, String name, const Vector3& textTranslate = Vector3())
{
    auto cache = parent->GetContext()->GetSubsystem<ResourceCache>();
    auto node = parent->CreateChild();
    node->SetPosition(pos);
    node->SetScale(scl);
    // TODO: rotation

    auto mat = new Material(parent->GetContext());
    if (color.a_ < 1.0f)
        mat->SetTechnique(0, cache->GetResource<Technique>("Techniques/NoTextureUnlitAlpha.xml"));
    else
        mat->SetTechnique(0, cache->GetResource<Technique>("Techniques/NoTextureUnlit.xml"));
    mat->SetShaderParameter("MatDiffColor", color);
    mat->SetFillMode(fill);
    mat->SetCullMode(CULL_NONE);

    auto modelObj = node->CreateComponent<StaticModel>();
    modelObj->SetModel(model);
    modelObj->SetMaterial(mat);

    auto textNode = node->CreateChild();
    textNode->Translate(textTranslate);
    textNode->Translate(Vector3(0.0f, 2.0f, 0.0f));
    auto text = textNode->CreateComponent<Text3D>();
    text->SetText(name);
    text->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.sdf"), 24);
    text->SetColor(Color::WHITE);
    text->SetAlignment(HA_CENTER, VA_CENTER);

    return node;
}

static Node* CreateDebugBox(SharedPtr<Node>& parent, const Vector3& pos, const Vector3& scl, const Color& color, FillMode fill, String name = "")
{
    auto cache = parent->GetContext()->GetSubsystem<ResourceCache>();
    auto model = cache->GetResource<Model>("Models/Box.mdl");
    return CreateDebugModel(parent, model, pos, scl, color, fill, name);
}

static Node* CreateDebugPlane(SharedPtr<Node>& parent, const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4,
    const Vector3& normal, const Color& color, FillMode fill, String name = "")
{
    auto cache = parent->GetContext()->GetSubsystem<ResourceCache>();
    SharedPtr<Model> model(new Model(parent->GetContext()));
    SharedPtr<VertexBuffer> vb(new VertexBuffer(parent->GetContext()));
    SharedPtr<IndexBuffer> ib(new IndexBuffer(parent->GetContext()));
    SharedPtr<Geometry> geom(new Geometry(parent->GetContext()));
    Vector3 data[] = {
        p1, normal,
        p2, normal,
        p3, normal,
        p4, normal,
    };
    unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };
    vb->SetSize(4, { VertexElement(TYPE_VECTOR3, SEM_POSITION), VertexElement(TYPE_VECTOR3, SEM_NORMAL) });
    vb->SetData((float*)data);
    ib->SetSize(6, false);
    ib->SetData(indices);
    geom->SetVertexBuffer(0, vb);
    geom->SetIndexBuffer(ib);
    geom->SetDrawRange(TRIANGLE_LIST, 0, 6);
    model->SetNumGeometries(1);
    model->SetGeometry(0, 0, geom);

    BoundingBox bounds;
    bounds.Merge(p1);
    bounds.Merge(p2);
    bounds.Merge(p3);
    bounds.Merge(p4);
    model->SetBoundingBox(bounds);
    return CreateDebugModel(parent, model, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), color, fill, name, bounds.Center());
}

DebugCircuitComponent::DebugCircuitComponent(Context* context) :
    LogicComponent(context)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    // SetUpdateEventMask(USE_FIXEDUPDATE);
}

void DebugCircuitComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<DebugCircuitComponent>();
}

void DebugCircuitComponent::SetPodCircuit(PodCircuit* circ)
{
    circ_ = circ;

    auto cache = GetSubsystem<ResourceCache>();

    if (!debugGroup_)
        debugGroup_ = GetNode()->CreateChild("Debug");

    debugGroup_->RemoveAllChildren();

    for (const auto& dec : circ->GetDecorationInstances())
    {
        auto model = circ->GetDecorationModel(dec.Index);
        auto& bounds = model->GetBoundingBox();
        auto node = CreateDebugBox(debugGroup_, dec.Position, (bounds.max_ - bounds.min_), Color::RED, FILL_WIREFRAME, String("Decoration ") + String(dec.Index));
        node->SetRotation(Quaternion(dec.Rotation));
    }

    for (const auto& light : circ->GetGlobalLights())
        CreateDebugBox(debugGroup_, light.Position, Vector3(5.0f, 5.0f, 5.0f), Color::YELLOW, FILL_SOLID, "GlobalLight");

    for (int i = 0; i < circ->GetSectors().Size(); i++)
    {
        const auto lights = circ->GetSectorLights(i);
        if (!lights) continue;

        for (const auto& light : *lights)
        {
            String name = "Light";
            if (light.Type == 1)
                name = "SpotLight";
            CreateDebugBox(debugGroup_, light.Position, Vector3(5.0f, 5.0f, 5.0f), Color::YELLOW, FILL_SOLID, name);
        }
    }

    int sidx = 0;
    for (auto& sound : circ->soundSection_.Sounds)
    {
        fp1616_t* values = reinterpret_cast<fp1616_t*>(&sound.Data[1]);
        Vector3 pos = PodTransform(Vector3FromFP1616(values));
        CreateDebugBox(debugGroup_, pos, Vector3(3.0f, 3.0f, 3.0f), Color::RED, FILL_SOLID, "Sound " + String(sidx++));
    }

    for (const auto& zone : circ->GetRepairZones())
    {
        Vector3 scl;
        scl.x_ = std::abs(zone.Positions[0].x_ - zone.Positions[2].x_);
        scl.z_ = std::abs(zone.Positions[2].z_ - zone.Positions[3].z_);
        scl.y_ = zone.Height;

        Color color = Color::BLUE;
        color.a_ = 0.75f;
        CreateDebugBox(debugGroup_, zone.CenterPos + Vector3(0.0f, zone.Height*0.5f, 0.0f), scl, color, FILL_SOLID, "RepairZone");
    }

    {
        for (auto& start : circ_->designationForward_.Starts)
        {
            fp1616_t* values = reinterpret_cast<fp1616_t*>(start.Data);
            Vector3 pos = PodTransform(Vector3FromFP1616(values));
            CreateDebugBox(debugGroup_, pos, Vector3(3.0f, 3.0f, 3.0f), Color::GREEN, FILL_SOLID, "Start");
        }

        int mi = 0;
        for (auto& macro : circ_->designationForward_.MacroSection.DesignationMacros)
        {
            CreateDebugPlane(debugGroup_, macro.PlanePos1, macro.PlanePos2, macro.PlanePos3, macro.PlanePos4, macro.PlaneNormal,
                Color(0.0f, 1.0f, 1.0f, 0.5f), FILL_SOLID, "DesignationMacro " + String(mi++));
        }
    }

    auto DebugDifficulty = [this](PodCircuit::Difficulty& d) {
        if (debug_enabledDifficulty != d.Path.Name) return;

        int si = 0;
        for (const auto& section : d.Level.Config1s)
        {
            Color c = Color(0.0f, 1.0f, 0.0f, 0.75f);
            CreateDebugBox(debugGroup_, section.Position, Vector3(5.0f, 5.0f, 5.0f), c, FILL_SOLID, "Section " + String(si++));
        }

        for (int i = 0; i < d.Path.PointLists.Size(); i++)
        {
            auto& enabledPointLists = debug_enabledPointLists[d.Path.Name];
            if (!enabledPointLists.Size())
                enabledPointLists.Resize(d.Path.PointLists.Size());
            if (!enabledPointLists[i]) continue;

            auto& list = d.Path.PointLists[i];
            for (const auto& point : list.Points)
                CreateDebugBox(debugGroup_, d.Path.Positions[point.PositionIndex], Vector3(3.0f, 3.0f, 3.0f), Color(1.0f, 0.0f, 1.0f, 1.0f), FILL_SOLID);
        }

        if (debug_enabledAllPositions[d.Path.Name])
            for (const auto& pos : d.Path.Positions)
                CreateDebugBox(debugGroup_, pos, Vector3(3.0f, 3.0f, 3.0f), Color(1.0f, 0.0f, 1.0f, 1.0f), FILL_SOLID);
    };
    DebugDifficulty(circ->difficultyForwardEasy_);
    DebugDifficulty(circ->difficultyForwardNormal_);
    DebugDifficulty(circ->difficultyForwardHard_);
    DebugDifficulty(circ->difficultyReverseEasy_);
    DebugDifficulty(circ->difficultyReverseNormal_);
    DebugDifficulty(circ->difficultyReverseHard_);
}
