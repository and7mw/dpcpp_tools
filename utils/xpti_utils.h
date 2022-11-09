#pragma once

#include <utility>
#include <unordered_map>
#include <limits>

#include "xpti/xpti_trace_framework.hpp"

namespace xptiUtils {
    struct profileEntry {
        explicit profileEntry(const std::string& name) : name(name) {}
        float timePercent = 0.0f;
        size_t totalTime = 0;
        size_t count = 0;
        double avg = 0.0f;
        size_t min = std::numeric_limits<size_t>::max();
        size_t max = 0.0;
        std::string name;
    };

    using taskInfo = std::unordered_map<std::string, std::string>;
    taskInfo extractTaskInfo(xpti::trace_event_data_t *event, const void *user_data);
};
