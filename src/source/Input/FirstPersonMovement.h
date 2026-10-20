#pragma once

namespace Input::FirstPerson
{
// Called after the original MovePath tick and its status/action checks.
void UpdateTileMovement(bool canStart);
void DirectionForYaw(float yawDegrees, int forward, int strafe, int& x, int& y);
}
