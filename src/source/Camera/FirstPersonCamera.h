#pragma once

#include "ICamera.h"

// A gameplay view layered over the continuously updated classic camera.
class FirstPersonCamera : public ICamera
{
public:
    bool UpdateClassic(ICamera& classic);
    bool Update() override { return false; }
    void Reset() override;
    void ResetView() override;
    const char* GetName() const override { return "FirstPerson"; }
    const CameraConfig& GetConfig() const override { return m_Config; }
    void SetConfig(const CameraConfig& config) override { m_Config = config; }
    const Frustum& GetFrustum() const override { return m_Frustum; }
    bool ShouldCullObject(const vec3_t p, float radius) const override;
    bool ShouldCullTerrain(int x, int y) const override;
    bool ShouldCullObject2D(float x, float y, float radius) const override;
    bool IsEngaged() const { return m_Blend > 0.0f; }
    bool IsReady() const { return m_Blend == 1.0f && m_Wanted; }
    void Look(float x, float y);
    void Zoom(float ticks);
    float Distance() const { return m_Distance; }
    bool ShouldHideLocalBody() const;
    float Yaw() const { return m_Yaw; }

private:
    bool IsWorldReady() const;
    bool UpdateBaseCamera(ICamera& classic);
    void UpdateBlend(bool wanted);
    void ApplyView(const CameraState& classic);
    void BuildFrustum();
    CameraState m_Classic;
    CameraConfig m_Config;
    Frustum m_Frustum;
    float m_Blend = 0.0f;
    float m_Yaw = 0.0f;
    float m_Pitch = -90.0f;
    float m_Distance = 0.0f;
    unsigned long long m_LastTick = 0;
    int m_World = -1;
    bool m_Wanted = false;
};
