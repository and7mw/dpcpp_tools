#include "sycl_collector.h"

void profilerTool::syclCollector::addTask(const uint64_t id,
                                          const std::unordered_map<std::string, std::string> &info) {
    if (tasksInfo.count(id)) {
        throw std::runtime_error("Can't add info about task with id: " + std::to_string(id) + ". Info have added already!");
    }
    tasksInfo[id] = info;
}

void profilerTool::syclCollector::addStartTask(const uint64_t id) {
    auto& times = tasksTime[id];
    times.push_back(Record());
    times.back().start = std::chrono::high_resolution_clock::now();
}

void profilerTool::syclCollector::addEndTask(const uint64_t id) {
    auto& times = tasksTime[id];
    if (times.empty()) {
        throw std::runtime_error("Can't add end time for task with id: " + std::to_string(id) +
                                 ". Start time for node hasn't set!");
    }
    times.back().start = std::chrono::high_resolution_clock::now();
}
