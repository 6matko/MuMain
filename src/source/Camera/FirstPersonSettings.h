#pragma once

// Client-local prototype tuning. The Options selector changes only the preferred mode.
struct FirstPersonSettings
{
    float fieldOfView = 90.0f;
    float eyeHeight = 180.0f;
    float headOffset = 12.0f;
    float sensitivity = 0.15f;
    float pitchLimit = 80.0f;
    float transitionSeconds = 0.65f;
};
