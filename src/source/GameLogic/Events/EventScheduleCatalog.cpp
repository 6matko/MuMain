#include "GameLogic/Events/EventScheduleCatalog.h"

#include <algorithm>
#include <limits>

namespace GameLogic::Events
{
namespace
{
constexpr int32_t EntryCountOffset = 5;
constexpr int32_t EntriesOffset = 6;
constexpr int32_t EntrySize = 6;
constexpr int32_t EventTypeOffset = 0;
constexpr int32_t StateOffset = 1;
constexpr int32_t RemainingOffset = 2;
constexpr auto RefreshInterval = std::chrono::seconds(30);
constexpr auto RequestThrottle = std::chrono::seconds(5);

bool IsKnownEvent(BYTE value)
{
    switch (static_cast<EventType>(value))
    {
    case EventType::DevilSquare:
    case EventType::BloodCastle:
    case EventType::ChaosCastle:
    case EventType::Kanturu:
        return true;
    default:
        return false;
    }
}

uint32_t ReadUInt32LittleEndian(const BYTE* data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8)
           | (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}
} // namespace

EventScheduleCatalog& EventScheduleCatalog::Instance()
{
    static EventScheduleCatalog instance;
    return instance;
}

void EventScheduleCatalog::Reset()
{
    m_events.clear();
    m_receivedAt = {};
    m_lastRequestAt = {};
    m_isAvailable = false;
}

bool EventScheduleCatalog::ReplaceFromPacket(const BYTE* data, int32_t size)
{
    if (data == nullptr || size < EntriesOffset)
    {
        return false;
    }

    const auto count = data[EntryCountOffset];
    const auto requiredSize = EntriesOffset + static_cast<int32_t>(count) * EntrySize;
    if (size != requiredSize)
    {
        return false;
    }

    std::vector<EventScheduleEntry> parsed;
    parsed.reserve(count);
    for (int32_t index = 0; index < count; ++index)
    {
        const auto* entry = data + EntriesOffset + index * EntrySize;
        const auto typeValue = entry[EventTypeOffset];
        const auto stateValue = entry[StateOffset];
        if (!IsKnownEvent(typeValue) || stateValue > static_cast<BYTE>(EventScheduleState::Running))
        {
            return false;
        }

        const auto type = static_cast<EventType>(typeValue);
        const auto duplicate = std::find_if(parsed.begin(), parsed.end(),
                                            [type](const EventScheduleEntry& item) { return item.Type == type; });
        if (duplicate != parsed.end())
        {
            return false;
        }

        parsed.push_back({type, static_cast<EventScheduleState>(stateValue),
                          ReadUInt32LittleEndian(entry + RemainingOffset)});
    }

    m_events = std::move(parsed);
    m_receivedAt = Clock::now();
    m_isAvailable = true;
    return true;
}

uint32_t EventScheduleCatalog::GetRemainingSeconds(const EventScheduleEntry& entry) const
{
    if (entry.State == EventScheduleState::Running || m_receivedAt == Clock::time_point{})
    {
        return 0;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - m_receivedAt).count();
    if (elapsed <= 0)
    {
        return entry.RemainingSeconds;
    }

    const auto elapsedSeconds = static_cast<uint64_t>(elapsed);
    return elapsedSeconds >= entry.RemainingSeconds ? 0
                                                     : entry.RemainingSeconds - static_cast<uint32_t>(elapsedSeconds);
}

bool EventScheduleCatalog::ShouldRefresh() const
{
    const auto now = Clock::now();
    if (m_lastRequestAt != Clock::time_point{} && now - m_lastRequestAt < RequestThrottle)
    {
        return false;
    }

    if (!m_isAvailable || now - m_receivedAt >= RefreshInterval)
    {
        return true;
    }

    return std::any_of(m_events.begin(), m_events.end(), [this](const EventScheduleEntry& entry)
                       { return entry.State != EventScheduleState::Running && GetRemainingSeconds(entry) == 0; });
}
} // namespace GameLogic::Events
