#include "doctest.h"
#include "Input/FirstPersonMovement.h"

#include <array>

TEST_CASE("First-person movement follows camera yaw through all eight tile directions [firstperson]")
{
    constexpr std::array<int, 8> expectedX{0, 1, 1, 1, 0, -1, -1, -1};
    constexpr std::array<int, 8> expectedY{1, 1, 0, -1, -1, -1, 0, 1};
    for (int sector = 0; sector < 8; ++sector)
    {
        CAPTURE(sector);
        int x, y;
        Input::FirstPerson::DirectionForYaw(sector * 45.0f, 1, 0, x, y);
        CHECK(x == expectedX[sector]);
        CHECK(y == expectedY[sector]);
        Input::FirstPerson::DirectionForYaw(sector * 45.0f, -1, 0, x, y);
        CHECK(x == -expectedX[sector]);
        CHECK(y == -expectedY[sector]);
    }
}

TEST_CASE("First-person strafing and combined keys rotate together [firstperson]")
{
    int x, y;
    Input::FirstPerson::DirectionForYaw(0.0f, 0, 1, x, y);
    CHECK(x == 1);
    CHECK(y == 0);
    Input::FirstPerson::DirectionForYaw(0.0f, 1, -1, x, y);
    CHECK(x == -1);
    CHECK(y == 1);
    Input::FirstPerson::DirectionForYaw(45.0f, 1, -1, x, y);
    CHECK(x == 0);
    CHECK(y == 1);
    Input::FirstPerson::DirectionForYaw(90.0f, -1, 1, x, y);
    CHECK(x == -1);
    CHECK(y == -1);
}

TEST_CASE("First-person direction wraps yaw and ignores released or opposing keys [firstperson]")
{
    for (float yaw : std::array{-720.0f, -360.0f, 0.0f, 360.0f, 720.0f})
    {
        CAPTURE(yaw);
        int x, y;
        Input::FirstPerson::DirectionForYaw(yaw, 1, 0, x, y);
        CHECK(x == 0);
        CHECK(y == 1);
        Input::FirstPerson::DirectionForYaw(yaw, 0, 0, x, y);
        CHECK(x == 0);
        CHECK(y == 0);
    }
}
