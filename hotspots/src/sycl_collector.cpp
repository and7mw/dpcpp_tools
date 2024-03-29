#include "sycl_collector.h"
#include "xpti_types.h"

#include <chrono>
#include <algorithm>

using namespace xptiUtils;
using perTaskTypeStat = profilerTool::syclCollector::perTaskTypeStat;

std::unordered_map<std::string, perTaskTypeStat> profilerTool::syclCollector::getProfileReport() const {
    // device to statistic
    std::unordered_map<std::string, perTaskTypeStat> result;

    std::unordered_map<std::string, uint64_t> deviceTotalTime;
    const auto& tasksMetrics = getPerTaskStatistic(tasks);
    for (const auto& task : tasksMetrics) {
        const auto taskMetrics = task.second;
        auto& typeTaskMetrics = result[taskMetrics.deviceName][taskMetrics.name];

        typeTaskMetrics.totalTime += taskMetrics.totalTime;
        typeTaskMetrics.count += taskMetrics.count;
        typeTaskMetrics.min = std::min(taskMetrics.min, typeTaskMetrics.min);
        typeTaskMetrics.max = std::max(taskMetrics.max, typeTaskMetrics.max);
        typeTaskMetrics.name = taskMetrics.name;

        // total device time
        if (deviceTotalTime.count(taskMetrics.deviceName) == 0) {
            deviceTotalTime[taskMetrics.deviceName] = 0;
            deviceTotalTime[taskMetrics.deviceName] += taskMetrics.totalTime;
        } else {
            deviceTotalTime[taskMetrics.deviceName] += taskMetrics.totalTime;
        }
    }

    for (auto it = result.begin(); it != result.end(); it++) {
        for (auto& event : it->second) {
            auto& profileEntry = event.second;
            profileEntry.avg = static_cast<double>(profileEntry.totalTime) / profileEntry.count;
            profileEntry.timePercent = (static_cast<double>(profileEntry.totalTime) / deviceTotalTime.at(it->first)) * 100.0f;
        }
    }

    return result;
}
