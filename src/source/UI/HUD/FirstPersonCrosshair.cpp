#include "stdafx.h"
#include "UI/HUD/FirstPersonCrosshair.h"

#include <memory>
#include "Camera/CameraState.h"
#include "Input/FirstPersonInput.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlThemedView.h"

namespace
{
struct CrosshairModel
{
    float x = 0.0f;
    float y = 0.0f;
};
using CrosshairView = UI::RmlBridge::ThemedView<CrosshairModel>;
std::unique_ptr<CrosshairView> s_View;

void BindCrosshair(Rml::DataModelConstructor& constructor, CrosshairModel& model)
{
    constructor.Bind("aim_x", &model.x);
    constructor.Bind("aim_y", &model.y);
}

void EnsureCrosshair()
{
    if (!s_View)
        s_View = std::make_unique<CrosshairView>("first_person_crosshair", BindCrosshair,
            std::vector<UI::RmlBridge::ThemedDocumentSpec>{{"Data/Interface/RmlUi/first_person_crosshair.rml"}});
    s_View->Ensure();
}
}

void UI::HUD::SyncFirstPersonCrosshair()
{
    if (!Input::FirstPerson::IsCaptured() && !s_View)
        return;
    EnsureCrosshair();
    auto* document = s_View->Document();
    if (!document)
        return;
    const float ratio = document->GetContext()->GetDensityIndependentPixelRatio();
    auto& model = s_View->GetModel();
    const float x = g_Camera.ScreenCenterX / ratio;
    const float y = g_Camera.ScreenCenterY / ratio;
    if (model.x != x) { model.x = x; s_View->MarkDirty("aim_x"); }
    if (model.y != y) { model.y = y; s_View->MarkDirty("aim_y"); }
    UI::RmlBridge::SyncDocumentVisibilityInFront(document, Input::FirstPerson::IsCaptured());
}

void UI::HUD::ReleaseFirstPersonCrosshair()
{
    s_View.reset();
}
