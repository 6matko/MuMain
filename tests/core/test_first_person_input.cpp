#include "stdafx.h"
#include "doctest.h"

#include <array>
#include <limits>
#include <SDL3/SDL.h>
#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzInterface.h"
#include "Input/FirstPersonInput.h"
#include "Scenes/SceneCore.h"

namespace
{
struct GameplayInputState
{
    EGameScene scene = SceneFlag;
    int error = ErrorMessage;
    bool legacyInput = InputEnable;
    bool goldInput = GoldInputEnable;
    bool guildInput = GuildInputEnable;
    bool preferred = GameConfig::GetInstance().IsFirstPersonPreferred();

    GameplayInputState()
    {
        SceneFlag = MAIN_SCENE;
        ErrorMessage = 0;
        InputEnable = false; // InitializeMainScene's ordinary gameplay state.
        GoldInputEnable = GuildInputEnable = false;
        GameConfig::GetInstance().SetFirstPersonPreferred(true);
    }

    ~GameplayInputState()
    {
        SceneFlag = scene;
        ErrorMessage = error;
        InputEnable = legacyInput;
        GoldInputEnable = goldInput;
        GuildInputEnable = guildInput;
        GameConfig::GetInstance().SetFirstPersonPreferred(preferred);
    }
};
}

TEST_CASE("First-person owns arrow keys in the main scene's normal legacy input state [firstperson]")
{
    GameplayInputState state;
    for (const int key : std::array{VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT})
        CHECK(Input::FirstPerson::OwnsMovementKey(key));
    for (const int key : std::array{'W', 'A', 'S', 'D'})
        CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(key));
}

TEST_CASE("First-person arrow ownership yields to text entry and game messages [firstperson]")
{
    GameplayInputState state;
    GoldInputEnable = true;
    CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(VK_UP));
    GoldInputEnable = false;
    GuildInputEnable = true;
    CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(VK_UP));
    GuildInputEnable = false;
    ErrorMessage = MESSAGE_LOG_OUT;
    CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(VK_UP));
}

TEST_CASE("Classic and character selection retain normal arrow key ownership [firstperson]")
{
    GameplayInputState state;
    GameConfig::GetInstance().SetFirstPersonPreferred(false);
    CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(VK_UP));
    GameConfig::GetInstance().SetFirstPersonPreferred(true);
    SceneFlag = CHARACTER_SCENE;
    CHECK_FALSE(Input::FirstPerson::OwnsMovementKey(VK_UP));
}

TEST_CASE("First-person look discards motion that changes capture and invalid deltas [firstperson]")
{
    SDL_MouseMotionEvent motion{};
    motion.xrel = 800.0f;
    motion.yrel = 600.0f;
    CHECK_FALSE(Input::FirstPerson::IsCurrentLookMotion(motion, false));
    motion.xrel = -10.0f;
    motion.yrel = 4.0f;
    CHECK(Input::FirstPerson::IsCurrentLookMotion(motion, true));
    motion.xrel = std::numeric_limits<float>::quiet_NaN();
    CHECK_FALSE(Input::FirstPerson::IsCurrentLookMotion(motion, true));
    motion.xrel = 0.0f;
    motion.yrel = std::numeric_limits<float>::infinity();
    CHECK_FALSE(Input::FirstPerson::IsCurrentLookMotion(motion, true));
}
