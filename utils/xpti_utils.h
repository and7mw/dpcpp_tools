#pragma once

#include <utility>
#include <unordered_map>

#include "xpti/xpti_trace_framework.hpp"

namespace xptiUtils {
    using taskInfo = std::unordered_map<std::string, std::string>;
    taskInfo extractTaskInfo(xpti::trace_event_data_t *event, const void *user_data);
};
