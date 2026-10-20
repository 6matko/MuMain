#include "stdafx.h"
#include "doctest.h"
#include "TestModelSlots.h"

#include "Camera/FirstPersonAnchor.h"
#include "Camera/FirstPersonSettings.h"
#include "Engine/Object/ZzzCharacter.h"

#include <array>
#include <filesystem>

TEST_CASE("First-person head anchor rises with shipped riding and Hell Fire animations [data][firstperson]")
{
    TestModelSlots slots;
    BMD& model = Models[MODEL_PLAYER];
    const auto folder = (std::filesystem::path(MU_TEST_DATA_DIR) / "Player").wstring() + L"/";
    REQUIRE(model.Open2(folder.c_str(), L"player.bmd"));
    constexpr int PlayerHead = 20; // Same head configured when the player model loads.
    model.BoneHead = PlayerHead;
    model.BodyHeight = 0.0f;
    static vec34_t bones[MAX_BONES];
    OBJECT object;
    object.Type = MODEL_PLAYER;
    object.Scale = 1.0f;
    object.BoneTransform = bones;
    Vector(100.0f, 200.0f, 30.0f, object.Position);
    Vector(0.0f, 0.0f, 0.0f, object.Angle);
    Vector(0.0f, 0.0f, 0.0f, object.HeadAngle);
    const FirstPersonSettings settings;
    auto eyeHeight = [&](int action, float frame)
    {
        object.CurrentAction = object.PriorAction = action;
        object.AnimationFrame = object.PriorAnimationFrame = frame;
        AnimateCharacterBones(&object);
        vec3_t eye;
        Camera::FirstPerson::EyePosition(object, model, settings, eye);
        return eye[2];
    };
    const float standing = eyeHeight(PLAYER_STOP_MALE, 0.0f);
    for (int action : std::array{PLAYER_STOP_RIDE, PLAYER_STOP_RIDE_HORSE, PLAYER_FENRIR_STAND})
    {
        CAPTURE(action);
        REQUIRE(action < model.NumActions);
        CHECK(eyeHeight(action, 0.0f) > standing);
    }
    float highest = standing;
    for (int action : std::array{PLAYER_SKILL_HELL_BEGIN, PLAYER_SKILL_HELL_START})
    {
        REQUIRE(action < model.NumActions);
        for (int frame = 0; frame < model.Actions[action].NumAnimationKeys; ++frame)
            highest = std::max(highest, eyeHeight(action, static_cast<float>(frame)));
    }
    constexpr float MinimumJumpRise = 30.0f;
    CHECK(highest > standing + MinimumJumpRise);
}
