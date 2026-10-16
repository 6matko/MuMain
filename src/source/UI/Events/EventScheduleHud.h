#pragma once

#include "UI/Core/WindowManager.h"
#include "UI/Events/EventScheduleRmlModel.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <chrono>

namespace mu::ui::window
{
class CEventScheduleHud : public CObject
{
public:
    ~CEventScheduleHud() override;
    bool Create(CManager* manager);
    void Release();
    void SyncDocVisibility(bool sceneAllowsShow);
    bool UpdateMouseEvent() override { return true; }
    bool UpdateKeyEvent() override { return true; }
    bool Update() override;
    bool Render() override { return true; }
    float GetLayerDepth() override;

private:
    void BindModel(Rml::DataModelConstructor& constructor, EventScheduleRmlModel& model);
    void SyncRows();
    CManager* m_Manager = nullptr;
    bool m_SceneAllowsShow = false;
    std::chrono::steady_clock::time_point m_NextUpdate{};
    UI::RmlBridge::ThemedView<EventScheduleRmlModel> m_View{
        "event_schedule",
        [this](Rml::DataModelConstructor& c, EventScheduleRmlModel& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/event_schedule.rml"}}};
};
}
