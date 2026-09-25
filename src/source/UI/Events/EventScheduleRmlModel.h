#pragma once

#include <string>
#include <vector>

namespace mu::ui::window
{
struct EventScheduleRow
{
    std::string name;
    std::string status;
    bool open = false;
    bool running = false;
};

struct EventScheduleRmlModel
{
    std::string title;
    std::string unavailable;
    std::string close;
    bool available = false;
    std::vector<EventScheduleRow> rows;
};
}
