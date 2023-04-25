#include "pi_api_collector.h"
#include "sycl_collector.h"

#include "xpti_utils.h"
#include "xpti_types.h"
#include "xpti/xpti_trace_framework.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <iomanip>

profilerTool::piApiCollector* piApiCollectorObj = nullptr;
profilerTool::syclCollector* syclCollectorObj = nullptr;

std::mutex mutex;

XPTI_CALLBACK_API void piApiCallback(uint16_t trace_type,
                                     xpti::trace_event_data_t *parent,
                                     xpti::trace_event_data_t *event,
                                     uint64_t instance,
                                     const void *user_data);

XPTI_CALLBACK_API void taskCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data);

XPTI_CALLBACK_API void taskExecCallback(uint16_t trace_type,
                                        xpti::trace_event_data_t *parent,
                                        xpti::trace_event_data_t *event,
                                        uint64_t instance,
                                        const void *user_data);

XPTI_CALLBACK_API void signalCallback(uint16_t trace_type,
                                      xpti::trace_event_data_t *parent,
                                      xpti::trace_event_data_t *event,
                                      uint64_t instance,
                                      const void *user_data);

// TODO: handle major_version, minor_version, version_str?
XPTI_CALLBACK_API void xptiTraceInit(unsigned int major_version,
                                     unsigned int minor_version,
                                     const char *version_str,
                                     const char *stream_name) {       
    const std::string piApiStream(xptiUtils::SYCL_PI_STREAM);
    const std::string syclStream(xptiUtils::SYCL_STREAM);

    if (stream_name == piApiStream) {
        piApiCollectorObj = new profilerTool::piApiCollector();
        const auto streamId = xptiRegisterStream(stream_name);

        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::function_begin),
                             piApiCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::function_end),
                             piApiCallback); 
    } else if (stream_name == syclStream) {
        syclCollectorObj = new profilerTool::syclCollector();
        const auto streamId = xptiRegisterStream(stream_name);

        // TODO: handle case with several graphs
        // xptiRegisterCallback(streamId,
        //                      static_cast<uint16_t>(xpti::trace_point_type_t::graph_create),
        //                      graphCreateCallback);

        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::node_create),
                             taskCreateCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::task_begin),
                             taskExecCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::task_end),
                             taskExecCallback);

        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::signal),
                             signalCallback);
    }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {}

XPTI_CALLBACK_API void piApiCallback(uint16_t trace_type,
                                     xpti::trace_event_data_t *parent,
                                     xpti::trace_event_data_t *event,
                                     uint64_t instance,
                                     const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::function_begin)) {
        piApiCollectorObj->addFuncStartExec(static_cast<const char*>(user_data), instance);
    } else if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::function_end)) {
        piApiCollectorObj->addFuncEndExec(static_cast<const char*>(user_data), instance);
    } else {
        throw std::runtime_error("Can't handle trace point: " + std::to_string(trace_type) +
                                 ". piApiCallback supports only function_begin and function_end!");
    }
}

XPTI_CALLBACK_API void taskCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    const auto& taskInfo = xptiUtils::extractMetadata(event, user_data);

    xptiUtils::addTask(event->unique_id, taskInfo, syclCollectorObj->tasks);
}

XPTI_CALLBACK_API void taskExecCallback(uint16_t trace_type,
                                        xpti::trace_event_data_t *parent,
                                        xpti::trace_event_data_t *event,
                                        uint64_t instance,
                                        const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_begin)) {
        xptiUtils::addTaskStartExec(event->unique_id, syclCollectorObj->tasks);
    } else if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_end)) {
        xptiUtils::addTaskEndExec(event->unique_id, syclCollectorObj->tasks);
    } else {
        throw std::runtime_error("Can't handle trace point: " + std::to_string(trace_type) +
                                 ". taskExecCallback supports only task_begin and task_end!");
    }
}

XPTI_CALLBACK_API void signalCallback(uint16_t trace_type,
                                      xpti::trace_event_data_t *parent,
                                      xpti::trace_event_data_t *event,
                                      uint64_t instance,
                                      const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    xptiUtils::addSignalHandler(event, user_data, syclCollectorObj->tasks);
}

const size_t defaultOffset = 10;
const size_t typeOffset = defaultOffset + 20;
const size_t nameOffset = defaultOffset + 50;

void printReport(const std::vector<xptiUtils::profileEntry>& stat, const std::string& type) {
    std::string typeToPrint = " ";
    for (size_t i = 0; i < stat.size(); i++) {
        if (i == 0) {
            typeToPrint = type + ":";
        }
        std::cout << std::setw(typeOffset) << typeToPrint
                  << std::fixed << std::setprecision(2)
                  << std::setw(defaultOffset) << stat[i].timePercent << "%"
                  << std::setw(defaultOffset) << stat[i].totalTime
                  << std::setw(defaultOffset) << stat[i].count
                  << std::setw(defaultOffset) << stat[i].avg
                  << std::setw(defaultOffset) << stat[i].min
                  << std::setw(defaultOffset) << stat[i].max
                  << std::setw(nameOffset) << stat[i].name
                  << std::endl;
        typeToPrint = " ";
    }
    std::cout << std::endl;
}

void printSyclReport() {
    if (syclCollectorObj != nullptr) {
        const auto& syclReport = syclCollectorObj->getProfileReport();
        for (const auto& device : syclReport) {
            const auto& reportTmp = device.second;
            std::vector<xptiUtils::profileEntry> report;
            for (const auto& it : reportTmp) {
                report.emplace_back(it.second);
            }

            std::sort(report.begin(), report.end(),
                      [](const xptiUtils::profileEntry& lhs,
                         const xptiUtils::profileEntry& rhs) {
                return lhs.totalTime > rhs.totalTime;
            });

            printReport(report, device.first);
        }
        delete syclCollectorObj;
        syclCollectorObj = nullptr;
    }
}

void printSyclPiReport() {
    if (piApiCollectorObj != nullptr) {
        const auto& piApiReport = piApiCollectorObj->getProfileReport();

        printReport(piApiReport, "PI API calls");

        delete piApiCollectorObj;
        piApiCollectorObj = nullptr;
    }
}

__attribute__((destructor)) static void fwFinialize() {
    std::cout << std::setw(typeOffset) << "Type"
              << std::setw(defaultOffset + 1) << "Time(%)"
              << std::setw(defaultOffset + 1) << "Time(μs)"
              << std::setw(defaultOffset + 1) << "Calls"
              << std::setw(defaultOffset + 1) << "Avg(μs)"
              << std::setw(defaultOffset + 1) << "Min(μs)"
              << std::setw(defaultOffset + 1) << "Max(μs)"
              << std::setw(nameOffset) << "Name"
              << std::endl;

    printSyclReport();
    printSyclPiReport();
}
