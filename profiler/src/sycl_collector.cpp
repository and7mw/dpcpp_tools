#include "sycl_collector.h"

#include <chrono>

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

#include <iostream>

std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>> profilerTool::syclCollector::getProfileReport() const {
    using eventToTable = std::unordered_map<std::string, xptiUtils::profileEntry>;
    
    std::unordered_map<size_t, eventToTable> deviceIdToProfileTable;

    size_t totalTime = 0;

    std::unordered_map<size_t, std::string> deviceIdToName;

    for (const auto& task : tasksInfo) {
        const auto& currTasksInfo = task.second;
        const std::string& nodeType = currTasksInfo.at("node_type");
        size_t device = std::stoul(currTasksInfo.at("sycl_device"), nullptr);
        size_t from_id = 0, to_id = 0;
        if (nodeType == "memory_transfer_node") {
            // if copy from/to host assign activity time to device
            from_id = std::stoul(currTasksInfo.at("copy_from_id"), nullptr);
            to_id = std::stoul(currTasksInfo.at("copy_to_id"), nullptr);
            if (!(from_id > 0 && to_id > 0)) {
                device = std::max(from_id, to_id);
            }
        } else {
            if (deviceIdToName.count(device) == 0) {
                deviceIdToName[device] = currTasksInfo.at("sycl_device_name");
            }
        }

        auto& eventToTableForDevice = deviceIdToProfileTable[device];

        std::string eventName = nodeType;
        if (nodeType == "command_group_node") {
            eventName = currTasksInfo.at("kernel_name");
        } else if (nodeType == "memory_transfer_node") {
            eventName += "_" + std::to_string(from_id) + "_" + std::to_string(to_id);
        }
        auto& eventProfileEntry = eventToTableForDevice[eventName];
        eventProfileEntry.name = eventName;

        auto& times = tasksTime.at(task.first);
        for (const auto& time : times) {
            eventProfileEntry.count++;
            
            const size_t currTime = std::chrono::duration_cast<std::chrono::microseconds>(time.end - time.start).count();
            eventProfileEntry.totalTime += currTime;
            totalTime += currTime;

            eventProfileEntry.min = std::min(eventProfileEntry.min, currTime);
            eventProfileEntry.max = std::max(eventProfileEntry.max, currTime);
        }
    }

    for (auto it = deviceIdToProfileTable.begin(); it != deviceIdToProfileTable.end(); it++) {
        for (auto& event : it->second) {
            auto& profileEntry = event.second;
            profileEntry.avg = static_cast<double>(profileEntry.totalTime) / profileEntry.count;
            profileEntry.timePercent = (static_cast<double>(profileEntry.totalTime) / totalTime) * 100.0f;
        }
    }

    std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>> result;

    for (const auto& iter : deviceIdToProfileTable) {
        auto& tableForDevice = result[deviceIdToName.at(iter.first)];
        
        auto& srcTable = iter.second;
        for (auto& event : srcTable) {
            tableForDevice.push_back(event.second);
        }
    }

    for (const auto& qwe : result) {
        std::cout << qwe.first << " " << qwe.second.size() << std::endl;
    }

    return result;
}
