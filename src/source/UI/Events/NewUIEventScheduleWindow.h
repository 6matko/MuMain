#pragma once

#include "UI/Core/WindowManager.h"
#include "UI/Events/EventScheduleRmlModel.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <chrono>

namespace mu::ui::window
{
class CEventScheduleWindow : public CObject
{
public:
    ~CEventScheduleWindow() override;
    bool Create(CManager* manager);
    void Release();
    void Show(bool visible) override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override { return true; }
    float GetLayerDepth() override;
    float GetKeyEventOrder() override;
    void OpenningProcess();
    void ClosingProcess();

private:
    void BindModel(Rml::DataModelConstructor& constructor, EventScheduleRmlModel& model);
    void SyncRows();
    CManager* m_Manager = nullptr;
    bool m_CloseRequested = false;
    std::chrono::steady_clock::time_point m_NextUpdate{};
    UI::RmlBridge::ThemedView<EventScheduleRmlModel> m_View{
        "event_schedule",
        [this](Rml::DataModelConstructor& c, EventScheduleRmlModel& model) { BindModel(c, model); },
        {{"Data/Interface/RmlUi/event_schedule.rml"}}};
};
}
