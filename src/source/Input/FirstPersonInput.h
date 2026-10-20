#pragma once

union SDL_Event;
struct SDL_Window;
struct SDL_MouseMotionEvent;

namespace Input::FirstPerson
{
void SetGameWindow(SDL_Window* window);
void UpdateOwnership();
void ReleasePointer();
bool IsCurrentLookMotion(const SDL_MouseMotionEvent& motion, bool captureUnchanged);
bool HandleMotion(const SDL_Event& event);
bool HandleWheel(const SDL_Event& event);
void PrepareButtonEvent(SDL_Event& event);
bool IsCaptured();
bool BlocksWorldActions();
bool OwnsMovementKey(int key);
bool TileDirection(int& x, int& y);
void CenterPicking();
}
