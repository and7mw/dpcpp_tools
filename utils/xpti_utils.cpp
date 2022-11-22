#include "xpti_utils.h"
#include "xpti_types.h"

#include <vector>
#include <string>
#include <algorithm>

size_t xptiUtils::Task::counter = 0;

namespace {
    const std::unordered_map<std::string, std::vector<std::string>> attrForTask = {
        {xptiUtils::COMMAND_NODE, std::vector<std::string>{xptiUtils::DEVICE_TYPE,
                                                           xptiUtils::DEVICE_ID,
                                                           xptiUtils::DEVICE_NAME,
                                                           xptiUtils::KERNEL_NAME}},
        {xptiUtils::MEM_TRANSF_NODE, std::vector<std::string>{xptiUtils::COPY_FROM,
                                                             xptiUtils::DEVICE_ID,
                                                             xptiUtils::DEVICE_NAME, // TODO: remove?
                                                             xptiUtils::COPY_TO,
                                                             xptiUtils::MEM_OBJ}},
        {xptiUtils::MEM_ALLOC_NODE, std::vector<std::string>{xptiUtils::MEM_OBJ,
                                                             xptiUtils::DEVICE_NAME,
                                                             xptiUtils::DEVICE_ID,
                                                             xptiUtils::DEVICE_TYPE}},
        {xptiUtils::MEM_DEALLOC_NODE, std::vector<std::string>{xptiUtils::DEVICE_NAME,
                                                               xptiUtils::DEVICE_ID,
                                                               xptiUtils::DEVICE_TYPE}}
    };
};

void xptiUtils::addTask(const size_t id,
                        const std::unordered_map<std::string, std::string> &metainfo,
                        std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks) {
    if (tasks.count(id)) {
        throw std::runtime_error("Can't add new task with id: " + std::to_string(id) + ". Already exist!");
    }
    tasks[id] = std::make_shared<Task>(metainfo);
}

void xptiUtils::addTaskStartExec(const size_t id, std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks) {
    if (!tasks.count(id)) {
        throw std::runtime_error("Can't add start time for Task with id: " + std::to_string(id) +
                                 ". Task missing in graph!");
    }
    tasks[id]->execDuration.push_back(xptiUtils::TimeRecord());
    tasks[id]->execDuration.back().start = std::chrono::high_resolution_clock::now();
    tasks[id]->incExecOrder();
}

void xptiUtils::addTaskEndExec(const size_t id, std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks) {
    if (!tasks.count(id)) {
        throw std::runtime_error("Can't add end time for task with id: " + std::to_string(id) +
                                 ". Task missing in graph!");
    }
    if (tasks[id]->execDuration.empty()) {
        throw std::runtime_error("Can't add end time for task with id: " + std::to_string(id) +
                                 ". Start time for Task don't set!");
    }
    tasks[id]->execDuration.back().end = std::chrono::high_resolution_clock::now();
}

#include <iostream>

std::unordered_map<std::string, std::string>
xptiUtils::extractMetadata(xpti::trace_event_data_t *event,const void *userData) {
    const std::string nodeType = reinterpret_cast<const char *>(userData);
    std::unordered_map<std::string, std::string> metadata;

    const xpti::metadata_t *xptiMetadata = xptiQueryMetadata(event);

    metadata[xptiUtils::NODE_TYPE] = nodeType;
    std::cout << "+++ " << nodeType << std::endl;
    if (attrForTask.count(nodeType)) {
        const auto& attrs = attrForTask.at(nodeType);
        for (const auto &item : *xptiMetadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            std::cout << "=== " << typeInfo << std::endl;
            if (std::find(attrs.begin(), attrs.end(), typeInfo) != attrs.end()) {
                metadata[typeInfo] = xpti::readMetadata(item);
            }
        }
    } else {
        for (const auto &item : *xptiMetadata) {
            metadata[xptiLookupString(item.first)] = xpti::readMetadata(item);
        }
    }

    return metadata;
}

#include <iostream>

xptiUtils::perTaskStatistic xptiUtils::getPerTaskStatistic(
        const std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks) {
    std::unordered_map<size_t, std::string> deviceIdToName;
    const std::string hostDeviceName = "Host";
    deviceIdToName[0] = hostDeviceName;
std::cout << "1.0" << std::endl;
    // extract device info
    for (const auto& task : tasks) {
        const auto& metadata = task.second->getMetainfo();
        for (const auto& qqq : metadata) {
            std::cout << qqq.first << " " << qqq.second << std::endl;
        }
std::cout << "1.0.1" << std::endl;
        const auto id = std::stoul(metadata.at(xptiUtils::DEVICE_ID), nullptr);
std::cout << "1.0.2" << std::endl;
        deviceIdToName[id] = metadata.at(xptiUtils::DEVICE_NAME);
std::cout << "1.0.3" << std::endl;
    }
    // exit(1);
std::cout << "1.1" << std::endl;
    // main loop
    xptiUtils::perTaskStatistic statistic;
    for (const auto& task : tasks) {
std::cout << "1.2" << std::endl;
        const auto& taskMetadata = task.second->getMetainfo();

        auto& taskStat = statistic[task.first];
        taskStat.order = task.second->getExecOrder();
        taskStat.deviceName = deviceIdToName.at(std::stoul(taskMetadata.at(xptiUtils::DEVICE_ID), nullptr));
std::cout << "1.3" << std::endl;
        const std::string& nodeType = taskMetadata.at(xptiUtils::NODE_TYPE);
std::cout << "1.4" << std::endl;
        std::string taskName;
        if (nodeType == xptiUtils::COMMAND_NODE) {
            taskName = taskMetadata.at(xptiUtils::KERNEL_NAME);
            const size_t pos = taskName.find_last_of(":");
            if (pos != std::string::npos) {
                taskName = taskName.substr(pos + 1);
            }
        } else {
            taskName = nodeType;
            // memory_allocation_node -> Memory Allocation
            const auto pos = taskName.find("_node");
            if (pos != std::string::npos) {
                taskName.erase(taskName.begin() + pos, taskName.end());
            }
            taskName[0] = std::toupper(taskName[0]);
            for (size_t i = 1; i < taskName.size(); i++) {
                if (taskName[i] == '_' && (i + 1) < taskName.size()) {
                    taskName[i + 1] = std::toupper(taskName[i + 1]);
                    taskName[i] = ' ';
                }
            }

            if (taskName == "Memory Transfer") {
                const auto fromDevice = deviceIdToName.at(std::stoul(taskMetadata.at(xptiUtils::COPY_FROM), nullptr));
                const auto toDevice = deviceIdToName.at(std::stoul(taskMetadata.at(xptiUtils::COPY_TO), nullptr));
                taskName += " from " + fromDevice + " to " + toDevice;

                if (fromDevice != hostDeviceName) {
                    taskStat.deviceName = fromDevice;
                } else {
                    taskStat.deviceName = toDevice;
                }
            }
        }
std::cout << "1.5" << std::endl;
        // fill statistic
        taskStat.name = taskName;
        taskStat.id = task.first;
        taskStat.type = nodeType;
std::cout << "1.6" << std::endl;
        auto& metadata = taskStat.metadata;
        metadata += /*"[" + taskMetadata.at(xptiUtils::DEVICE_TYPE) + "]" +*/ taskMetadata.at(xptiUtils::DEVICE_NAME) + "\n";
        if (taskStat.type == xptiUtils::MEM_ALLOC_NODE) {
            metadata += "Mem obj id: " + taskMetadata.at(xptiUtils::MEM_OBJ) + "\n";
        }
std::cout << "1.7" << std::endl;
        // fill metrics
        auto& times = task.second->execDuration;;
        for (const auto& time : times) {
            taskStat.count++;
            const size_t currTime = std::chrono::duration_cast<std::chrono::microseconds>(time.end - time.start).count();
            taskStat.totalTime += currTime;

            taskStat.min = std::min(taskStat.min, currTime);
            taskStat.max = std::max(taskStat.max, currTime);
        }
    }

    return statistic;
}
