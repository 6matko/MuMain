#include "stdafx.h"
#include "I18N/All.h"
#include "UI/Events/NewUIEventScheduleWindow.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlStackingOrder.h"
#include "Core/Utilities/StringUtils.h"
#include "GameLogic/Events/EventScheduleCatalog.h"

using namespace mu::ui::window;
using namespace GameLogic::Events;

namespace
{
constexpr uint32_t SecondsPerMinute = 60;
constexpr uint32_t SecondsPerHour = 60 * SecondsPerMinute;
constexpr auto DisplayRefreshInterval = std::chrono::seconds(1);
constexpr float KeyEventOrder = 10.f;
const wchar_t* GetEventName(EventType type)
{
    switch (type)
    {
    case EventType::DevilSquare:
        return I18N::Game::DevilSquare;
    case EventType::BloodCastle:
        return I18N::Game::BloodCastle;
    case EventType::ChaosCastle:
        return I18N::Game::ChaosCastle;
    case EventType::Kanturu:
        return I18N::Game::Kanturu;
    default:
        return L"";
    }
}

std::wstring FormatDuration(uint32_t totalSeconds)
{
    const auto hours = totalSeconds / SecondsPerHour;
    const auto minutes = (totalSeconds % SecondsPerHour) / SecondsPerMinute;
    const auto seconds = totalSeconds % SecondsPerMinute;
    wchar_t text[32] = {};
    if (hours > 0)
    {
        mu_swprintf_s(text, L"%02u:%02u:%02u", hours, minutes, seconds);
    }
    else
    {
        mu_swprintf_s(text, L"%02u:%02u", minutes, seconds);
    }

    return text;
}


std::string EventStatus(const EventScheduleEntry& entry)
{
    if (entry.State == EventScheduleState::Running)
        return StringUtils::WideToNarrow(I18N::Game::EventScheduleInProgress);
    const auto duration = FormatDuration(ScheduleCatalog().GetRemainingSeconds(entry));
    const auto* format = entry.State == EventScheduleState::Open ? I18N::Game::EventScheduleClosesIn
                                                               : I18N::Game::EventScheduleStartsIn;
    wchar_t status[128] = {};
    mu_swprintf_s(status, format, duration.c_str());
    return StringUtils::WideToNarrow(status);
}
}

CEventScheduleWindow::~CEventScheduleWindow() { Release(); }

bool CEventScheduleWindow::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_Manager = manager;
    m_Manager->AddUIObj(INTERFACE_EVENT_SCHEDULE, this);
    m_View.Ensure();
    Show(false);
    return true;
}

void CEventScheduleWindow::Release()
{
    if (m_Manager)
        m_Manager->RemoveUIObj(this);
    m_Manager = nullptr;
    m_View.Release();
}

void CEventScheduleWindow::Show(bool visible)
{
    CObject::Show(visible);
    m_View.Ensure();
    UI::RmlBridge::SyncDocumentVisibilityInFront(m_View.Document(), visible);
    m_NextUpdate = {};
}

bool CEventScheduleWindow::UpdateMouseEvent()
{
    return !UI::RmlBridge::IsPointerOver(m_View.Document());
}

bool CEventScheduleWindow::UpdateKeyEvent()
{
    if (!IsVisible() || !IsPress(VK_ESCAPE))
        return true;
    g_pNewUISystem->Hide(INTERFACE_EVENT_SCHEDULE);
    return false;
}

bool CEventScheduleWindow::Update()
{
    m_View.Ensure();
    UI::RmlBridge::SyncDocumentVisibilityInFront(m_View.Document(), IsVisible());
    if (m_CloseRequested)
    {
        m_CloseRequested = false;
        g_pNewUISystem->Hide(INTERFACE_EVENT_SCHEDULE);
    }
    if (!IsVisible())
        return true;
    if (ScheduleCatalog().ShouldRefresh())
        ScheduleCatalog().Request();
    const auto now = std::chrono::steady_clock::now();
    if (now >= m_NextUpdate)
    {
        SyncRows();
        m_NextUpdate = now + DisplayRefreshInterval;
    }
    return true;
}

void CEventScheduleWindow::SyncRows()
{
    auto& model = m_View.GetModel();
    model.available = ScheduleCatalog().IsAvailable();
    model.rows.clear();
    for (const auto& entry : ScheduleCatalog().GetEvents())
        model.rows.push_back({StringUtils::WideToNarrow(GetEventName(entry.Type)), EventStatus(entry),
                              entry.State == EventScheduleState::Open, entry.State == EventScheduleState::Running});
    m_View.MarkDirty("available");
    m_View.MarkDirty("rows");
}

void CEventScheduleWindow::BindModel(Rml::DataModelConstructor& c, EventScheduleRmlModel& model)
{
    model.title = StringUtils::WideToNarrow(I18N::Game::EventScheduleTitle);
    model.unavailable = StringUtils::WideToNarrow(I18N::Game::EventScheduleNotSupported);
    model.close = StringUtils::WideToNarrow(I18N::Game::Close388);
    c.Bind("title", &model.title);
    c.Bind("unavailable", &model.unavailable);
    c.Bind("close", &model.close);
    c.Bind("available", &model.available);
    auto row = c.RegisterStruct<EventScheduleRow>();
    row.RegisterMember("name", &EventScheduleRow::name);
    row.RegisterMember("status", &EventScheduleRow::status);
    row.RegisterMember("open", &EventScheduleRow::open);
    row.RegisterMember("running", &EventScheduleRow::running);
    c.RegisterArray<std::vector<EventScheduleRow>>();
    c.Bind("rows", &model.rows);
    c.BindEventCallback("schedule_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
                        { m_CloseRequested = true; });
}

float CEventScheduleWindow::GetLayerDepth() { return UI::RmlBridge::ForegroundPanelLayerDepth; }
float CEventScheduleWindow::GetKeyEventOrder() { return KeyEventOrder; }
void CEventScheduleWindow::OpenningProcess()
{
    m_CloseRequested = false;
    m_NextUpdate = {};
    ScheduleCatalog().Request();
}
void CEventScheduleWindow::ClosingProcess() { m_View.Hide(); }
