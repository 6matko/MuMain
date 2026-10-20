#include "stdafx.h"
#include "FirstPersonCamera.h"
#include "CameraManager.h"
#include "CameraProjection.h"
#include "FirstPersonAnchor.h"

#include <SDL3/SDL_timer.h>
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/Events/Cinematic/CDirection.h"
#include "Scenes/SceneCore.h"
#include "Scenes/SceneCommon.h"
#include "World/MapInfra/MapManager.h"

extern int LoadingWorld;
extern int MouseWheel;

namespace
{
constexpr float NearPlane = 5.0f;
constexpr float Degrees = 360.0f;
constexpr float Radians = Q_PI / 180.0f;
constexpr float MaxFrameSeconds = 0.1f;
constexpr float Milliseconds = 1000.0f;
constexpr float ZoomStep = 50.0f;
constexpr float MaxDistance = 600.0f;
constexpr float BodyVisibleDistance = 50.0f;
constexpr float HorizontalPitch = -90.0f;

float ShortAngle(float from, float to)
{
    return std::remainder(to - from, Degrees);
}

void ViewForward(float yawDegrees, float pitchDegrees, vec3_t forward)
{
    const float yaw = yawDegrees * Radians;
    const float pitch = pitchDegrees * Radians;
    Vector(-sinf(yaw) * sinf(pitch), -cosf(yaw) * sinf(pitch), -cosf(pitch), forward);
}
}

void FirstPersonCamera::Reset()
{
    m_Blend = 0.0f;
    m_Wanted = false;
    m_World = -1;
    m_LastTick = 0;
}

void FirstPersonCamera::ResetView()
{
    m_Distance = 0.0f;
    m_Pitch = HorizontalPitch;
}

bool FirstPersonCamera::IsWorldReady() const
{
    bool gameplayCamera = true;
#ifdef _EDITOR
    gameplayCamera = CameraManager::Instance().GetCurrentMode() != CameraMode::FreeFly;
#endif
    return gameplayCamera && SceneFlag == MAIN_SCENE && InitMainScene && EnableMainRender && Hero && Hero->Object.Live && Hero->Dead == 0
        && LoadingWorld == 0 && Hero->Object.Teleport == TELEPORT_NONE && !g_Direction.IsDirection();
}

bool FirstPersonCamera::UpdateBaseCamera(ICamera& classic)
{
    if (IsEngaged())
        g_Camera = m_Classic;
    // UI/transition wheel events must not alter the underlying classic distance.
    // Captured gameplay wheel events are handled directly by FirstPersonInput.
    if (SceneFlag == MAIN_SCENE && (IsEngaged() || GameConfig::GetInstance().IsFirstPersonPreferred()))
        MouseWheel = 0;
    const bool locked = classic.Update();
    m_Classic = g_Camera;
    return locked;
}

bool FirstPersonCamera::UpdateClassic(ICamera& classic)
{
    const bool locked = UpdateBaseCamera(classic);
    const bool valid = IsWorldReady();
    if (!valid || m_World != gMapManager.WorldActive)
    {
        Reset();
        if (!valid)
            return locked;
        m_World = gMapManager.WorldActive;
    }
    const bool wanted = GameConfig::GetInstance().IsFirstPersonPreferred();
    if (wanted && !m_Wanted && m_Blend == 0.0f)
    {
        // CreateAngle uses atan2(dx, -dy); camera forward is (sin(yaw), cos(yaw)).
        m_Yaw = 180.0f - Hero->Object.Angle[2];
        m_Pitch = HorizontalPitch;
    }
    m_Wanted = wanted;
    UpdateBlend(wanted);
    if (IsEngaged())
    {
        m_Config = classic.GetConfig();
        ApplyView(m_Classic);
        BuildFrustum();
    }
    return locked;
}

void FirstPersonCamera::UpdateBlend(bool wanted)
{
    const auto& settings = GameConfig::GetInstance().GetFirstPersonSettings();
    const auto now = SDL_GetTicks();
    const float dt = m_LastTick ? std::min((now - m_LastTick) / Milliseconds, MaxFrameSeconds) : 0.0f;
    m_LastTick = now;
    m_Blend = std::clamp(m_Blend + (wanted ? dt : -dt) / settings.transitionSeconds, 0.0f, 1.0f);
}

void FirstPersonCamera::Look(float x, float y)
{
    const auto& settings = GameConfig::GetInstance().GetFirstPersonSettings();
    if (!IsReady() || !std::isfinite(x) || !std::isfinite(y))
        return;
    m_Yaw = std::remainder(m_Yaw + x * settings.sensitivity, Degrees);
    m_Pitch = std::clamp(m_Pitch + y * settings.sensitivity,
        HorizontalPitch - settings.pitchLimit, HorizontalPitch + settings.pitchLimit);
}

void FirstPersonCamera::Zoom(float ticks)
{
    if (CameraManager::Instance().IsZoomLocked())
        return;
    m_Distance = std::clamp(m_Distance - ticks * ZoomStep, 0.0f, MaxDistance);
}

bool FirstPersonCamera::ShouldHideLocalBody() const
{
    return IsReady() && m_Distance < BodyVisibleDistance;
}

void FirstPersonCamera::ApplyView(const CameraState& classic)
{
    const auto& settings = GameConfig::GetInstance().GetFirstPersonSettings();
    const float t = m_Blend * m_Blend * (3.0f - 2.0f * m_Blend);
    vec3_t eye;
    Camera::FirstPerson::EyePosition(Hero->Object, Models[Hero->Object.Type], settings, eye);
    vec3_t forward;
    ViewForward(m_Yaw, m_Pitch, forward);
    for (int i = 0; i < 3; ++i)
    {
        const float position = eye[i] - forward[i] * m_Distance;
        g_Camera.Position[i] = classic.Position[i] + (position - classic.Position[i]) * t;
    }
    g_Camera.Angle[0] = classic.Angle[0] + ShortAngle(classic.Angle[0], m_Pitch) * t;
    g_Camera.Angle[1] = classic.Angle[1] * (1.0f - t);
    g_Camera.Angle[2] = classic.Angle[2] + ShortAngle(classic.Angle[2], m_Yaw) * t;
    m_Config.hFov += (settings.fieldOfView - m_Config.hFov) * t;
    m_Config.nearPlane += (NearPlane - m_Config.nearPlane) * t;
    g_Camera.FOV = HFovToVFov(m_Config.hFov, 1.0f);
    g_Camera.ViewNear = m_Config.nearPlane;
    g_Camera.UpdateMatrix();
}

void FirstPersonCamera::BuildFrustum()
{
    vec3_t forward;
    ViewForward(g_Camera.Angle[2], g_Camera.Angle[0], forward);
    vec3_t up = {0.0f, 0.0f, 1.0f};
    vec3_t right;
    CrossProduct(forward, up, right);
    VectorNormalize(right);
    CrossProduct(right, forward, up);
    const float aspect = CameraProjection::WorldAspectRatio();
    m_Frustum.BuildFromCamera(g_Camera.Position, forward, up, g_Camera.FOV, aspect,
        m_Config.nearPlane, m_Config.farPlane, m_Config.terrainCullRange);
}

bool FirstPersonCamera::ShouldCullObject(const vec3_t p, float radius) const
{
    return !m_Frustum.TestSphere(p, radius);
}

bool FirstPersonCamera::ShouldCullTerrain(int x, int y) const
{
    vec3_t p = {(x + 0.5f) * TERRAIN_SCALE, (y + 0.5f) * TERRAIN_SCALE, 0.0f};
    // Ground trapezoids degenerate at the horizon. Keep the normal bounded terrain range.
    return fabsf(p[0] - g_Camera.Position[0]) > m_Config.terrainCullRange
        || fabsf(p[1] - g_Camera.Position[1]) > m_Config.terrainCullRange;
}

bool FirstPersonCamera::ShouldCullObject2D(float x, float y, float radius) const
{
    // Ground-only tests must not cull tall objects seen horizontally.
    return fabsf(x - g_Camera.Position[0]) > m_Config.objectCullRange + radius
        || fabsf(y - g_Camera.Position[1]) > m_Config.objectCullRange + radius;
}
