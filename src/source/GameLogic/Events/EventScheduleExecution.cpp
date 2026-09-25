#include "stdafx.h"
#include "GameLogic/Events/EventScheduleCatalog.h"

#include "Network/Server/WSclient.h"

namespace GameLogic::Events
{
void EventScheduleCatalog::Request()
{
    const auto now = Clock::now();
    if (m_lastRequestAt != Clock::time_point{} && now - m_lastRequestAt < std::chrono::seconds(5))
    {
        return;
    }

    m_lastRequestAt = now;
    if (SocketClient != nullptr)
    {
        SocketClient->ToGameServer()->SendEventScheduleRequest();
    }
}
} // namespace GameLogic::Events
