#include "stdafx.h"
#include "Input/FirstPersonMovement.h"

#include <SDL3/SDL_timer.h>
#include "Input/FirstPersonInput.h"
#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"

extern int TargetX;
extern int TargetY;

namespace
{
constexpr unsigned long long RequestIntervalMs = 100;
unsigned long long s_LastRequest = 0;
}

void Input::FirstPerson::UpdateTileMovement(bool canStart)
{
    int dx, dy;
    if (!canStart || !Hero || Hero->Movement || !TileDirection(dx, dy)
        || MouseLButton || MouseRButton || Hero->AttackTime != 0)
        return;
    const auto now = SDL_GetTicks();
    if (now - s_LastRequest < RequestIntervalMs)
        return;
    s_LastRequest = now;
    const int x = Hero->PositionX + dx;
    const int y = Hero->PositionY + dy;
    if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
        return;
    // A one-neighbour request finishes before another is issued; release/turn never
    // leaves a distant queued destination. The pathfinder enforces the normal walls.
    if (!PathFinding2(Hero->PositionX, Hero->PositionY, x, y, &Hero->Path))
        return;
    // Do not take a detour around a blocked neighbour for a directional key.
    constexpr int AdjacentPathPoints = 2;
    if (Hero->Path.PathNum != AdjacentPathPoints)
        return;
    TargetX = x;
    TargetY = y;
    Hero->MovementType = MOVEMENT_MOVE;
    SendMove(Hero, &Hero->Object);
}
