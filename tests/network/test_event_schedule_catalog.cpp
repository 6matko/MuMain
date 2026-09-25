#include "Core/Platform/WinCompat.h"
#include "GameLogic/Events/EventScheduleCatalog.h"

#include "doctest.h"

#include <vector>

namespace
{
constexpr size_t HeaderSize = 6;
constexpr size_t EntrySize = 6;

std::vector<BYTE> MakePacket(BYTE type, BYTE state, uint32_t remaining)
{
    std::vector<BYTE> packet(HeaderSize + EntrySize);
    packet[0] = 0xC2;
    packet[1] = 0;
    packet[2] = static_cast<BYTE>(packet.size());
    packet[3] = 0xF5;
    packet[4] = 0x02;
    packet[5] = 1;
    packet[6] = type;
    packet[7] = state;
    packet[8] = static_cast<BYTE>(remaining);
    packet[9] = static_cast<BYTE>(remaining >> 8);
    packet[10] = static_cast<BYTE>(remaining >> 16);
    packet[11] = static_cast<BYTE>(remaining >> 24);
    return packet;
}
} // namespace

TEST_CASE("event schedule catalog replaces a valid snapshot")
{
    auto& catalog = GameLogic::Events::ScheduleCatalog();
    catalog.Reset();
    const auto packet = MakePacket(static_cast<BYTE>(GameLogic::Events::EventType::BloodCastle),
                                   static_cast<BYTE>(GameLogic::Events::EventScheduleState::Open), 90);

    CHECK(catalog.ReplaceFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    REQUIRE(catalog.GetEvents().size() == 1);
    CHECK(catalog.IsAvailable());
    CHECK(catalog.GetEvents().front().Type == GameLogic::Events::EventType::BloodCastle);
    CHECK(catalog.GetEvents().front().State == GameLogic::Events::EventScheduleState::Open);
    CHECK(catalog.GetRemainingSeconds(catalog.GetEvents().front()) <= 90);
}

TEST_CASE("event schedule catalog rejects truncated snapshots")
{
    auto& catalog = GameLogic::Events::ScheduleCatalog();
    catalog.Reset();
    auto packet = MakePacket(static_cast<BYTE>(GameLogic::Events::EventType::DevilSquare),
                             static_cast<BYTE>(GameLogic::Events::EventScheduleState::Scheduled), 120);
    packet.pop_back();

    CHECK_FALSE(catalog.ReplaceFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(catalog.IsAvailable());
    CHECK(catalog.GetEvents().empty());
}

TEST_CASE("event schedule catalog keeps the previous snapshot when a replacement is invalid")
{
    auto& catalog = GameLogic::Events::ScheduleCatalog();
    catalog.Reset();
    const auto valid = MakePacket(static_cast<BYTE>(GameLogic::Events::EventType::ChaosCastle),
                                  static_cast<BYTE>(GameLogic::Events::EventScheduleState::Running), 0);
    CHECK(catalog.ReplaceFromPacket(valid.data(), static_cast<int32_t>(valid.size())));

    const auto invalid = MakePacket(0xFF, static_cast<BYTE>(GameLogic::Events::EventScheduleState::Scheduled), 30);
    CHECK_FALSE(catalog.ReplaceFromPacket(invalid.data(), static_cast<int32_t>(invalid.size())));
    REQUIRE(catalog.GetEvents().size() == 1);
    CHECK(catalog.GetEvents().front().Type == GameLogic::Events::EventType::ChaosCastle);
}

TEST_CASE("event schedule catalog rejects duplicate events")
{
    auto& catalog = GameLogic::Events::ScheduleCatalog();
    catalog.Reset();
    auto packet = MakePacket(static_cast<BYTE>(GameLogic::Events::EventType::Kanturu),
                             static_cast<BYTE>(GameLogic::Events::EventScheduleState::Scheduled), 30);
    packet[5] = 2;
    packet.insert(packet.end(), packet.begin() + HeaderSize, packet.end());

    CHECK_FALSE(catalog.ReplaceFromPacket(packet.data(), static_cast<int32_t>(packet.size())));
    CHECK_FALSE(catalog.IsAvailable());
}
