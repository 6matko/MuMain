#include "stdafx.h"
#include "Input/FirstPersonInput.h"
#include "Input/FirstPersonMovement.h"

#include <SDL3/SDL.h>
#include "Camera/CameraManager.h"
#include "Core/Input/KeyState.h"
#include "Core/Input/UiInputRouter.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzInterface.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "Scenes/SceneCore.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/Scaling/UITransform.h"

extern int LoadingWorld;

namespace
{
SDL_Window* s_CapturedWindow = nullptr;
SDL_Window* s_GameWindow = nullptr;
bool s_KeysArmed = false;
bool s_ButtonsArmed = false;
bool s_CaptureFailureReported = false;
Uint64 s_CaptureGeneration = 0;
int s_CaptureReports = 0;
int s_MotionReports = 0;
constexpr int DiagnosticSampleLimit = 8;

bool ViewRequested()
{
    return SceneFlag == MAIN_SCENE && (GameConfig::GetInstance().IsFirstPersonPreferred()
        || CameraManager::Instance().FirstPerson().IsEngaged());
}

bool PointerNeeded()
{
    // InputEnable is the legacy login/message text-entry flag. MainScene sets it
    // false for ordinary gameplay; actual UI focus owns text input here.
    return Core::Input::IsKeyDown(VK_MENU) || ErrorMessage != 0
        || GoldInputEnable || GuildInputEnable
        || !g_pNewUISystem || g_pNewUISystem->NeedsGameplayPointer()
        || RmlUiRuntime::Instance().GetKeyOwningDocument()
        || mu::ui::window::CInventoryCtrl::GetPickedItem();
}

bool MovementKeysDown()
{
    return Core::Input::IsKeyDown(VK_UP) || Core::Input::IsKeyDown(VK_LEFT)
        || Core::Input::IsKeyDown(VK_DOWN) || Core::Input::IsKeyDown(VK_RIGHT);
}

void ClearWorldButtons()
{
    MouseLButton = MouseLButtonPush = MouseRButton = MouseRButtonPush = false;
    MouseLButtonPop = MouseRButtonPop = MouseLButtonDBClick = false;
    MouseRButtonPress = 0;
}

void CenterUiPointer(SDL_Window* window)
{
    // Relative events never move the UI pointer; reset hover to the aiming point.
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    SDL_Event centered{};
    centered.type = SDL_EVENT_MOUSE_MOTION;
    centered.motion.windowID = SDL_GetWindowID(window);
    centered.motion.x = width * 0.5f;
    centered.motion.y = height * 0.5f;
    Core::Input::RouteToUi(centered, window);
}

bool AcquirePointer(SDL_Window* window)
{
    if (!SDL_SetWindowRelativeMouseMode(window, true))
    {
        if (!s_CaptureFailureReported)
            mu::log::Get("input")->warn("First-person mouse capture failed ({}): {}",
                SDL_GetCurrentVideoDriver(), SDL_GetError());
        s_CaptureFailureReported = true;
        return false;
    }
    s_CaptureFailureReported = false;
    s_CapturedWindow = window;
    ++s_CaptureGeneration;
    if (s_CaptureReports < DiagnosticSampleLimit)
    {
        ++s_CaptureReports;
        mu::log::Get("input")->info("First-person capture: driver={} window={} relative={}",
            SDL_GetCurrentVideoDriver(), SDL_GetWindowID(window), SDL_GetWindowRelativeMouseMode(window));
    }
    ClearWorldButtons();
    CenterUiPointer(window);
    return true;
}
} // namespace

bool Input::FirstPerson::IsCaptured()
{
    return s_CapturedWindow != nullptr;
}

void Input::FirstPerson::ReleasePointer()
{
    if (s_CapturedWindow)
    {
        SDL_SetWindowRelativeMouseMode(s_CapturedWindow, false);
        s_CapturedWindow = nullptr;
        SDL_GetMouseState(&g_fWindowMouseX, &g_fWindowMouseY);
        const auto units = UI::Scaling::ScreenOverlayTransform(WindowWidth, WindowHeight);
        MouseX = static_cast<int>(UI::Scaling::LogicalX(units, g_fWindowMouseX));
        MouseY = static_cast<int>(UI::Scaling::LogicalY(units, g_fWindowMouseY));
        ClearWorldButtons();
    }
    s_KeysArmed = s_ButtonsArmed = false;
}

bool Input::FirstPerson::IsCurrentLookMotion(const SDL_MouseMotionEvent& motion, bool captureUnchanged)
{
    // The event which acquires capture was generated in absolute/UI mode. SDL
    // flushes queued motion on a mode switch, but cannot flush that current event.
    return captureUnchanged && std::isfinite(motion.xrel) && std::isfinite(motion.yrel);
}

void Input::FirstPerson::SetGameWindow(SDL_Window* window)
{
    ReleasePointer();
    s_GameWindow = window;
}

void Input::FirstPerson::UpdateOwnership()
{
    auto* window = SDL_GetKeyboardFocus();
    const bool pointerNeeded = PointerNeeded();
    const bool ready = ViewRequested() && CameraManager::Instance().FirstPerson().IsReady()
        && Hero && Hero->Object.Live && Hero->Dead == 0 && LoadingWorld == 0
        && Hero->Object.Teleport == TELEPORT_NONE;
    if (!ready || !window || window != s_GameWindow || pointerNeeded)
    {
        if (s_CapturedWindow && s_CaptureReports <= DiagnosticSampleLimit)
            mu::log::Get("input")->info("First-person release: ready={} gameFocus={} ui={}",
                ready, window && window == s_GameWindow, pointerNeeded);
        ReleasePointer();
        if (ViewRequested() && !pointerNeeded)
            ClearWorldButtons();
        return;
    }
    if (s_CapturedWindow && !SDL_GetWindowRelativeMouseMode(window))
        ReleasePointer();
    if (!s_CapturedWindow && !AcquirePointer(window))
        return;
    if (!MovementKeysDown())
        s_KeysArmed = true;
    if (!(SDL_GetMouseState(nullptr, nullptr) & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)))
        s_ButtonsArmed = true;
    if (!s_ButtonsArmed)
        ClearWorldButtons();
    CenterPicking();
}

bool Input::FirstPerson::HandleMotion(const SDL_Event& event)
{
    const bool wasCaptured = IsCaptured();
    const auto generation = s_CaptureGeneration;
    UpdateOwnership();
    if (event.type != SDL_EVENT_MOUSE_MOTION || !IsCaptured()
        || event.motion.windowID != SDL_GetWindowID(s_CapturedWindow))
        return false;
    if (IsCurrentLookMotion(event.motion, wasCaptured && generation == s_CaptureGeneration))
    {
        if (s_MotionReports < DiagnosticSampleLimit)
        {
            ++s_MotionReports;
            mu::log::Get("input")->info("First-person motion: delta=({}, {}) position=({}, {})",
                event.motion.xrel, event.motion.yrel, event.motion.x, event.motion.y);
        }
        CameraManager::Instance().FirstPerson().Look(event.motion.xrel, event.motion.yrel);
    }
    return true;
}

void Input::FirstPerson::PrepareButtonEvent(SDL_Event& event)
{
    UpdateOwnership();
    if (!IsCaptured())
        return;
    int width, height;
    SDL_GetWindowSize(s_CapturedWindow, &width, &height);
    event.button.x = static_cast<float>(g_Camera.ScreenCenterX) * width / std::max(1u, WindowWidth);
    event.button.y = static_cast<float>(g_Camera.ScreenCenterY) * height / std::max(1u, WindowHeight);
}

bool Input::FirstPerson::HandleWheel(const SDL_Event& event)
{
    UpdateOwnership();
    if (event.type != SDL_EVENT_MOUSE_WHEEL || !IsCaptured()
        || event.wheel.windowID != SDL_GetWindowID(s_CapturedWindow))
        return false;
    const float ticks = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
    CameraManager::Instance().FirstPerson().Zoom(ticks);
    return true; // Consume locked ticks too; never change the saved classic zoom.
}

bool Input::FirstPerson::BlocksWorldActions()
{
    return ViewRequested() && (!IsCaptured() || !s_ButtonsArmed);
}

bool Input::FirstPerson::OwnsMovementKey(int key)
{
    const bool movement = key == VK_UP || key == VK_LEFT || key == VK_DOWN || key == VK_RIGHT;
    return movement && ViewRequested() && !PointerNeeded();
}

bool Input::FirstPerson::TileDirection(int& x, int& y)
{
    x = y = 0;
    if (!IsCaptured() || !s_KeysArmed || PointerNeeded())
        return false;
    const int forward = int(Core::Input::IsKeyDown(VK_UP)) - int(Core::Input::IsKeyDown(VK_DOWN));
    const int strafe = int(Core::Input::IsKeyDown(VK_RIGHT)) - int(Core::Input::IsKeyDown(VK_LEFT));
    if (!forward && !strafe)
        return false;
    DirectionForYaw(CameraManager::Instance().FirstPerson().Yaw(), forward, strafe, x, y);
    return true;
}

void Input::FirstPerson::CenterPicking()
{
    if (!IsCaptured())
        return;
    g_fWindowMouseX = static_cast<float>(g_Camera.ScreenCenterX);
    g_fWindowMouseY = static_cast<float>(g_Camera.ScreenCenterY);
    MouseX = static_cast<int>((g_Camera.ScreenCenterX - g_fScreenOffset_x) / g_fScreenRate_x);
    MouseY = static_cast<int>((g_Camera.ScreenCenterY - g_fScreenOffset_y) / g_fScreenRate_y);
}
