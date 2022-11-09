#include "pi_api_collector.h"
#include "xpti_utils.h"

#include <stdexcept>
#include <vector>
#include <algorithm>

void profilerTool::piApiCollector::addFuncStartExec(const std::string name, const uint64_t id) {
    auto &times = records[name];
    if (times.count(id) != 0) {
        const std::string error = "Function " + name + " with id: " +
                                   std::to_string(id) + " already started execution!";
        throw std::runtime_error(error);
    }

    auto &curr_record = times[id];
    curr_record.start = std::chrono::high_resolution_clock::now();
}

void profilerTool::piApiCollector::addFuncEndExec(const std::string name, const uint64_t id) {
    const std::string error = "Function " + name + " with id: " +
                               std::to_string(id) + " haven't started execution!";

    if (records.count(name) == 0) {
        throw std::runtime_error(error);
    }
    
    auto &times = records[name];
    if (times.count(id) == 0) {
        throw std::runtime_error(error);
    }

    auto &curr_record = times[id];
    curr_record.end = std::chrono::high_resolution_clock::now();
}

std::vector<xptiUtils::profileEntry> profilerTool::piApiCollector::getProfileReport() const {
    std::vector<xptiUtils::profileEntry> table;
    table.reserve(records.size());
    size_t totalTime = 0;
    for (const auto& func : records) {
        table.push_back(xptiUtils::profileEntry(func.first));
        auto& currEntry = table.back();
        size_t totalFuncTime = 0;
        for (const auto& entry : func.second) {
            currEntry.count++;
            const size_t currTime = std::chrono::duration_cast<std::chrono::microseconds>(entry.second.end - entry.second.start).count();
            totalFuncTime += currTime;

            currEntry.min = std::min(currEntry.min, currTime);
            currEntry.max = std::max(currEntry.max, currTime);
        }
        currEntry.avg = static_cast<double>(totalFuncTime) / currEntry.count;
        currEntry.totalTime = totalFuncTime;
        totalTime += totalFuncTime;
    }
    std::sort(table.begin(), table.end(),
              [](const xptiUtils::profileEntry& lhs,
                 const xptiUtils::profileEntry& rhs) {
        return lhs.totalTime > rhs.totalTime;
    });
    for (auto& entry : table) {
        entry.timePercent = (static_cast<double>(entry.totalTime) / totalTime) * 100.0;
    }
    return table;
}
