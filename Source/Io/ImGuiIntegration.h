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

#include <Urho3D/Core/Object.h>

namespace Urho3D {
    class Texture2D;
}

using namespace Urho3D;

URHO3D_EVENT(E_IMGUI_NEWFRAME, imguiNewFrame)
{
}

class ImGuiIntegration : public Object
{
    URHO3D_OBJECT(ImGuiIntegration, Object);

public:
    ImGuiIntegration(Context* context);
    ~ImGuiIntegration();

protected:
    // Used to track if there's already a touch, since imgui doesn't have multi-touch support.
    bool touching = false;
    // The touch ID of the single tracked touch
    int single_touchID;

    static const char* GetClipboardText(void* user_data);
    static void SetClipboardText(void* user_data, const char* text);

    // Call ImGui::NewFrame()
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    // Call ImGui::Render()
    void HandleEndRendering(StringHash eventType, VariantMap& eventData);

    // Input
    void HandleKeyUp(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleTextInput(StringHash eventType, VariantMap& eventData);
    void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
    void HandleTouchEnd(StringHash eventType, VariantMap& eventData);
    void HandleTouchMove(StringHash eventType, VariantMap& eventData);
};
