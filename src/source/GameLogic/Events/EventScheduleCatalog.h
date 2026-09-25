#pragma once

#include "Core/Platform/WinCompat.h"

#include <chrono>
#include <cstdint>
#include <vector>

namespace GameLogic::Events
{
enum class EventType : BYTE
{
    DevilSquare = 1,
    BloodCastle = 2,
    ChaosCastle = 4,
    Kanturu = 7,
};

enum class EventScheduleState : BYTE
{
    Scheduled = 0,
    Open = 1,
    Running = 2,
};

struct EventScheduleEntry
{
    EventType Type;
    EventScheduleState State;
    uint32_t RemainingSeconds;
};

class EventScheduleCatalog
{
public:
    static EventScheduleCatalog& Instance();

    const std::vector<EventScheduleEntry>& GetEvents() const
    {
        return m_events;
    }

    bool IsAvailable() const
    {
        return m_isAvailable;
    }

    void Reset();
    void Request();

    // Replaces the whole snapshot atomically. Invalid packets leave the
    // currently displayed snapshot untouched.
    bool ReplaceFromPacket(const BYTE* data, int32_t size);

    uint32_t GetRemainingSeconds(const EventScheduleEntry& entry) const;
    bool ShouldRefresh() const;

private:
    using Clock = std::chrono::steady_clock;

    std::vector<EventScheduleEntry> m_events;
    Clock::time_point m_receivedAt{};
    Clock::time_point m_lastRequestAt{};
    bool m_isAvailable = false;
};

inline EventScheduleCatalog& ScheduleCatalog()
{
    return EventScheduleCatalog::Instance();
}
} // namespace GameLogic::Events
