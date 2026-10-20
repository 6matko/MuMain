#include "stdafx.h"
#include "doctest.h"

#include "Camera/CameraManager.h"
#include "Camera/FirstPersonAnchor.h"
#include "Camera/FirstPersonSettings.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Render/Models/ZzzBMD.h"
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Scenes/SceneCommon.h"
#include "Scenes/SceneCore.h"
#include "World/MapInfra/MapManager.h"
#include <SDL3/SDL_timer.h>

extern int LoadingWorld;

namespace
{
struct GameplayCameraState
{
    EGameScene scene = SceneFlag;
    bool initialized = InitMainScene;
    bool render = EnableMainRender;
    bool preferred = GameConfig::GetInstance().IsFirstPersonPreferred();
    CHARACTER* hero = Hero;
    CameraState camera = g_Camera;
    int loading = LoadingWorld;
    int world = gMapManager.WorldActive;

    ~GameplayCameraState()
    {
        SceneFlag = scene;
        InitMainScene = initialized;
        EnableMainRender = render;
        GameConfig::GetInstance().SetFirstPersonPreferred(preferred);
        Hero = hero;
        g_Camera = camera;
        LoadingWorld = loading;
        gMapManager.WorldActive = world;
    }
};
}

TEST_CASE("First-person head follows animated riding and jumping positions without shared model state [firstperson]")
{
    vec34_t bones[1]{};
    OBJECT object;
    object.BoneTransform = bones;
    object.EnableBoneMatrix = true;
    object.Scale = 1.5f;
    Vector(100.0f, 200.0f, 30.0f, object.Position);
    BMD model;
    model.NumBones = 1;
    model.BoneHead = 0;
    model.Bones = new Bone_t[1]{}; // Owned and released by BMD.
    // A different character/item preview last used the shared BMD.
    model.BodyScale = 99.0f;
    Vector(-900.0f, -900.0f, -900.0f, model.BodyOrigin);
    const FirstPersonSettings settings;
    vec3_t eye;

    bones[0][0][3] = 5.0f;
    bones[0][1][3] = -10.0f;
    bones[0][2][3] = 180.0f;
    Camera::FirstPerson::EyePosition(object, model, settings, eye);
    CHECK(eye[0] == doctest::Approx(107.5f));
    CHECK(eye[1] == doctest::Approx(185.0f));
    const float standingEye = eye[2];
    CHECK(standingEye == doctest::Approx(318.0f));

    // The animation raises the rider/jumping skeleton without moving its origin.
    bones[0][2][3] += 90.0f;
    Camera::FirstPerson::EyePosition(object, model, settings, eye);
    CHECK(eye[2] - standingEye == doctest::Approx(135.0f));
    // Flying mounts can additionally raise the object's world origin.
    object.Position[2] += 30.0f;
    Camera::FirstPerson::EyePosition(object, model, settings, eye);
    CHECK(eye[2] - standingEye == doctest::Approx(165.0f));

    SUBCASE("Uncomputed poses use fallback height instead of stale bones")
    {
        object.EnableBoneMatrix = false;
    }
    SUBCASE("Models without a head use fallback height")
    {
        model.BoneHead = -1;
    }
    SUBCASE("Dummy head bones use fallback height")
    {
        model.Bones[0].Dummy = true;
    }
    Camera::FirstPerson::EyePosition(object, model, settings, eye);
    CHECK(eye[0] == object.Position[0]);
    CHECK(eye[1] == object.Position[1]);
    CHECK(eye[2] == doctest::Approx(object.Position[2] + settings.eyeHeight * object.Scale));
}

TEST_CASE("First-person zoom respects F10 lock and keeps independent bounded distance [firstperson]")
{
    auto& manager = CameraManager::Instance();
    const bool wasLocked = manager.IsZoomLocked();
    if (!wasLocked)
        manager.ToggleZoomLock();
    FirstPersonCamera camera;
    camera.Zoom(-3.0f);
    CHECK(camera.Distance() == 0.0f);
    manager.ToggleZoomLock();
    camera.Zoom(-3.0f);
    CHECK(camera.Distance() == 150.0f);
    camera.Reset(); // World transitions retain the independent distance.
    CHECK(camera.Distance() == 150.0f);
    camera.Zoom(-1000.0f);
    CHECK(camera.Distance() == 600.0f);
    camera.Zoom(1000.0f);
    CHECK(camera.Distance() == 0.0f);
    camera.Zoom(-2.0f);
    camera.ResetView();
    CHECK(camera.Distance() == 0.0f);
    if (wasLocked)
        manager.ToggleZoomLock();
}

TEST_CASE("First-person F11 restores horizontal look and accepts subsequent mouse motion [firstperson]")
{
    GameplayCameraState saved;
    CHARACTER player;
    Hero = &player;
    Hero->Object.Live = true;
    Hero->Object.Type = MODEL_PLAYER;
    Hero->Object.Scale = 1.0f;
    Hero->Object.Teleport = TELEPORT_NONE;
    Hero->Dead = 0;
    SceneFlag = MAIN_SCENE;
    InitMainScene = EnableMainRender = true;
    LoadingWorld = 0;
    gMapManager.WorldActive = 0;
    GameConfig::GetInstance().SetFirstPersonPreferred(true);
    FirstPersonCamera camera;
    FirstPersonCamera base;
    camera.UpdateClassic(base);
    constexpr int BlendSteps = 8;
    constexpr int BlendStepMs = 100;
    for (int step = 0; step < BlendSteps; ++step)
    {
        SDL_Delay(BlendStepMs);
        camera.UpdateClassic(base);
    }
    REQUIRE(camera.IsReady());
    camera.Look(0.0f, 200.0f);
    camera.UpdateClassic(base);
    CHECK(g_Camera.Angle[0] > -90.0f);
    camera.ResetView();
    camera.UpdateClassic(base);
    CHECK(g_Camera.Angle[0] == doctest::Approx(-90.0f));
    camera.Look(0.0f, -10.0f);
    camera.UpdateClassic(base);
    CHECK(g_Camera.Angle[0] < -90.0f);
}
