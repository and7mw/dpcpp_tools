#include "sycl_collector.h"
#include "xpti_types.h"

#include <chrono>
#include <algorithm>

using namespace xptiUtils;

std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>> profilerTool::syclCollector::getProfileReport() const {
    // using eventToTable = std::unordered_map<std::string, xptiUtils::profileEntry>;
    
    // std::unordered_map<size_t, eventToTable> deviceIdToProfileTable;

    // size_t totalTime = 0;

    // std::unordered_map<size_t, std::string> deviceIdToName;
    // deviceIdToName[0] = "Host";

    // for (const auto& task : tasks) {
    //     const auto& currtasks = task.second;
    //     const std::string& nodeType = currtasks.at("node_type");
    //     size_t device = std::stoul(currtasks.at(xptiUtils::DEVICE_ID), nullptr);
    //     size_t from_id = 0, to_id = 0;
    //     if (nodeType == xptiUtils::MEM_TRANSF_NODE) {
    //         // if copy from/to host assign activity time to device
    //         from_id = std::stoul(currtasks.at(xptiUtils::COPY_FROM), nullptr);
    //         to_id = std::stoul(currtasks.at(xptiUtils::COPY_TO), nullptr);
    //         if (!(from_id > 0 && to_id > 0)) {
    //             device = std::max(from_id, to_id);
    //         }
    //     } else {
    //         if (deviceIdToName.count(device) == 0) {
    //             deviceIdToName[device] = currtasks.at(xptiUtils::DEVICE_NAME);
    //         }
    //     }

    //     auto& eventToTableForDevice = deviceIdToProfileTable[device];

    //     std::string eventName = nodeType;

    //     std::string eventTableKey = eventName;

    //     if (nodeType == xptiUtils::COMMAND_NODE) {
    //         eventName = currtasks.at("short_kernel_name");
    //     } else {
    //         const auto pos = eventName.find("_node");
    //         if (pos != std::string::npos) {
    //             eventName.erase(eventName.begin() + pos, eventName.end());
    //         }
    //         eventName[0] = std::toupper(eventName[0]);
    //         for (size_t i = 1; i < eventName.size(); i++) {
    //             if (eventName[i] == '_' && (i + 1) < eventName.size()) {
    //                 eventName[i + 1] = std::toupper(eventName[i + 1]);
    //                 eventName[i] = ' ';
    //             }
    //         }

    //         if (eventName == "Memory Transfer") {
    //             eventTableKey += "_" + std::to_string(from_id) + "_" + std::to_string(to_id);
    //         }
    //     }
    //     auto& eventProfileEntry = eventToTableForDevice[eventTableKey];
    //     if (eventName == "Memory Transfer") {
    //         eventProfileEntry.from = from_id;
    //         eventProfileEntry.to = to_id;
    //     }

    //     eventProfileEntry.name = eventName;
    //     eventProfileEntry.type = nodeType;

    //     auto& times = tasksTime.at(task.first);
    //     for (const auto& time : times) {
    //         eventProfileEntry.count++;
            
    //         const size_t currTime = std::chrono::duration_cast<std::chrono::microseconds>(time.end - time.start).count();
    //         eventProfileEntry.totalTime += currTime;
    //         totalTime += currTime;

    //         eventProfileEntry.min = std::min(eventProfileEntry.min, currTime);
    //         eventProfileEntry.max = std::max(eventProfileEntry.max, currTime);
    //     }
    // }

    // for (auto it = deviceIdToProfileTable.begin(); it != deviceIdToProfileTable.end(); it++) {
    //     for (auto& event : it->second) {
    //         auto& profileEntry = event.second;
    //         profileEntry.avg = static_cast<double>(profileEntry.totalTime) / profileEntry.count;
    //         profileEntry.timePercent = (static_cast<double>(profileEntry.totalTime) / totalTime) * 100.0f;
    //     }
    // }

    // std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>> result;

    // for (auto& iter : deviceIdToProfileTable) {
    //     auto& tableForDevice = result[deviceIdToName.at(iter.first)];
        
    //     auto& srcTable = iter.second;
    //     for (auto& event : srcTable) {
    //         if (event.second.type == xptiUtils::MEM_TRANSF_NODE) {
    //             auto& eventName = event.second.name;
    //             eventName += " from " + deviceIdToName.at(event.second.from);
    //             eventName += " to " + deviceIdToName.at(event.second.to);

    //         }
    //         tableForDevice.push_back(event.second);
    //     }
    // }

    // for (auto& table : result) {
    //     std::sort(table.second.begin(), table.second.end(),
    //               [](const xptiUtils::profileEntry& lhs,
    //                  const xptiUtils::profileEntry& rhs) {
    //         return lhs.totalTime > rhs.totalTime;
    //     });
    // }   

    // return result;
    return std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>>();
}
