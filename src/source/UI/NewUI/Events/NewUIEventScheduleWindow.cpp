#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Events/NewUIEventScheduleWindow.h"

#include "Audio/DSPlaySound.h"
#include "GameLogic/Events/EventScheduleCatalog.h"
#include "UI/NewUI/NewUISystem.h"

#include <string>

using namespace SEASON3B;
using GameLogic::Events::EventScheduleEntry;
using GameLogic::Events::EventScheduleState;
using GameLogic::Events::EventType;
using GameLogic::Events::ScheduleCatalog;

namespace
{
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
    const auto hours = totalSeconds / 3600;
    const auto minutes = (totalSeconds % 3600) / 60;
    const auto seconds = totalSeconds % 60;
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

void SetTextColor(BYTE red, BYTE green, BYTE blue)
{
    g_pRenderText->SetTextColor(red, green, blue, 255);
    g_pRenderText->SetBgColor(0);
}
} // namespace

CNewUIEventScheduleWindow::CNewUIEventScheduleWindow()
    : m_pNewUIMng(nullptr)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
}

CNewUIEventScheduleWindow::~CNewUIEventScheduleWindow()
{
    Release();
}

bool CNewUIEventScheduleWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_EVENT_SCHEDULE, this);
    LoadImages();
    SetPos(x, y);

    wchar_t closeText[256] = {};
    mu_swprintf_s(closeText, I18N::Game::CloseS, L"Esc");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    Show(false);
    return true;
}

void CNewUIEventScheduleWindow::Release()
{
    UnloadImages();
    if (m_pNewUIMng != nullptr)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIEventScheduleWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH,
                               EXIT_BUTTON_HEIGHT);
}

bool CNewUIEventScheduleWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, INTERFACE_EVENT_SCHEDULE) || m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(INTERFACE_EVENT_SCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return !CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT);
}

bool CNewUIEventScheduleWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(INTERFACE_EVENT_SCHEDULE) && IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_EVENT_SCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIEventScheduleWindow::Update()
{
    auto& catalog = ScheduleCatalog();
    if (catalog.ShouldRefresh())
    {
        catalog.Request();
    }

    return true;
}

bool CNewUIEventScheduleWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderFrame();

    g_pRenderText->SetFont(g_hFontBold);
    SetTextColor(255, 220, 120);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::EventScheduleTitle, WINDOW_WIDTH, 0,
                              RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
    RenderEvents();
    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

float CNewUIEventScheduleWindow::GetLayerDepth()
{
    return UI::Layout::ForegroundPanelLayerDepth;
}

float CNewUIEventScheduleWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CNewUIEventScheduleWindow::OpenningProcess()
{
    ScheduleCatalog().Request();
}

void CNewUIEventScheduleWindow::ClosingProcess()
{
}

void CNewUIEventScheduleWindow::RenderFrame()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WINDOW_HEIGHT - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);
    RenderImage(IMAGE_BACK, x, y, float(WINDOW_WIDTH), float(WINDOW_HEIGHT));
    RenderImage(IMAGE_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f,
                       float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT),
                       float(FRAME_SIDE_WIDTH), middleHeight, 0.f, 0.f, float(FRAME_SIDE_WIDTH),
                       float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImage(IMAGE_BOTTOM, x, y + float(WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH),
                float(FRAME_BOTTOM_HEIGHT));
}

void CNewUIEventScheduleWindow::RenderEvents()
{
    auto& catalog = ScheduleCatalog();
    if (!catalog.IsAvailable())
    {
        SetTextColor(220, 220, 220);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP,
                                  I18N::Game::EventScheduleNotSupported, CONTENT_WIDTH, ROW_HEIGHT * 3);
        return;
    }

    int row = 0;
    for (const auto& entry : catalog.GetEvents())
    {
        const auto y = m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT;
        SetTextColor(235, 235, 235);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, y, GetEventName(entry.Type), CONTENT_WIDTH, 0,
                                  RT3_SORT_LEFT);

        wchar_t status[128] = {};
        if (entry.State == EventScheduleState::Running)
        {
            mu_swprintf_s(status, L"%s", I18N::Game::EventScheduleInProgress);
            SetTextColor(255, 190, 100);
        }
        else
        {
            const auto duration = FormatDuration(catalog.GetRemainingSeconds(entry));
            const auto* format = entry.State == EventScheduleState::Open ? I18N::Game::EventScheduleClosesIn
                                                                         : I18N::Game::EventScheduleStartsIn;
            mu_swprintf_s(status, format, duration.c_str());
            if (entry.State == EventScheduleState::Open)
            {
                SetTextColor(120, 255, 140);
            }
            else
            {
                SetTextColor(170, 210, 255);
            }
        }

        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, y + 14, status, CONTENT_WIDTH, 0, RT3_SORT_RIGHT);
        ++row;
    }
}

void CNewUIEventScheduleWindow::LoadImages()
{
    LoadBitmap(L"Interface/newui_msgbox_back.jpg", IMAGE_BACK, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back01.tga", IMAGE_TOP, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-L.tga", IMAGE_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back02-R.tga", IMAGE_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface/newui_item_back03.tga", IMAGE_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface/newui_exit_00.tga", IMAGE_BTN_EXIT, GL_LINEAR);
}

void CNewUIEventScheduleWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_BACK);
    DeleteBitmap(IMAGE_TOP);
    DeleteBitmap(IMAGE_LEFT);
    DeleteBitmap(IMAGE_RIGHT);
    DeleteBitmap(IMAGE_BOTTOM);
    DeleteBitmap(IMAGE_BTN_EXIT);
}
