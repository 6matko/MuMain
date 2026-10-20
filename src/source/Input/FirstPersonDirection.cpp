#include "Input/FirstPersonMovement.h"

#include <cmath>

void Input::FirstPerson::DirectionForYaw(float yawDegrees, int forward, int strafe, int& x, int& y)
{
    x = y = 0;
    if (!forward && !strafe)
        return;
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float Radians = Pi / 180.0f;
    constexpr float SectorAngle = Pi / 4.0f;
    const float yaw = yawDegrees * Radians;
    const float dx = std::sin(yaw) * forward + std::cos(yaw) * strafe;
    const float dy = std::cos(yaw) * forward - std::sin(yaw) * strafe;
    const float snapped = std::round(std::atan2(dy, dx) / SectorAngle) * SectorAngle;
    x = static_cast<int>(std::round(std::cos(snapped)));
    y = static_cast<int>(std::round(std::sin(snapped)));
}
