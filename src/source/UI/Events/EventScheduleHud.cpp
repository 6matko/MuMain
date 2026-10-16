#include "stdafx.h"
#include "I18N/All.h"
#include "UI/Events/EventScheduleHud.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
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

CEventScheduleHud::~CEventScheduleHud() { Release(); }

bool CEventScheduleHud::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_Manager = manager;
    m_Manager->AddUIObj(INTERFACE_EVENT_SCHEDULE, this);
    m_View.Ensure();
    Show(true);
    return true;
}

void CEventScheduleHud::Release()
{
    if (m_Manager)
        m_Manager->RemoveUIObj(this);
    m_Manager = nullptr;
    m_View.Release();
}

void CEventScheduleHud::SyncDocVisibility(bool sceneAllowsShow)
{
    m_SceneAllowsShow = sceneAllowsShow;
    m_View.Ensure();
    const auto& catalog = ScheduleCatalog();
    const bool visible = sceneAllowsShow && IsVisible() && catalog.IsAvailable() && !catalog.GetEvents().empty();
    UI::RmlBridge::SyncDocumentVisibilityBehind(m_View.Document(), visible);
}

bool CEventScheduleHud::Update()
{
    if (!m_SceneAllowsShow || !IsVisible())
        return true;
    m_View.Ensure();
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

void CEventScheduleHud::SyncRows()
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

void CEventScheduleHud::BindModel(Rml::DataModelConstructor& c, EventScheduleRmlModel& model)
{
    model.title = StringUtils::WideToNarrow(I18N::Game::EventScheduleTitle);
    c.Bind("title", &model.title);
    c.Bind("available", &model.available);
    auto row = c.RegisterStruct<EventScheduleRow>();
    row.RegisterMember("name", &EventScheduleRow::name);
    row.RegisterMember("status", &EventScheduleRow::status);
    row.RegisterMember("open", &EventScheduleRow::open);
    row.RegisterMember("running", &EventScheduleRow::running);
    c.RegisterArray<std::vector<EventScheduleRow>>();
    c.Bind("rows", &model.rows);
}

float CEventScheduleHud::GetLayerDepth() { return UI::RmlBridge::EventScheduleHudLayerDepth; }
