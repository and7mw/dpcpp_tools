#include "pi_api_collector.h"
#include "sycl_collector.h"

#include "xpti_utils.h"
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

// TODO: handle major_version, minor_version, version_str?
XPTI_CALLBACK_API void xptiTraceInit(unsigned int major_version,
                                     unsigned int minor_version,
                                     const char *version_str,
                                     const char *stream_name) {       
    const std::string piApiStream("sycl.pi");
    const std::string syclStream("sycl");

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
    }
}

size_t headerPrinted = 0;

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
    const size_t len = 10;

    if (headerPrinted != 1) {
        std::cout << std::setw(len + 20) << "Type"
                  << std::setw(len + 2) << "Time(%)"
                  << std::setw(len) << "Time(ms)"
                  << std::setw(len) << "Calls"
                  << std::setw(len) << "Avg(ms)"
                  << std::setw(len) << "Min(ms)"
                  << std::setw(len) << "Max(ms)"
                  << std::setw(25) << "Name"
                  << std::endl;
        headerPrinted = 1;
    }
    if (stream_name == std::string("sycl")) {
        if (syclCollectorObj != nullptr) {
            const auto& syclReport = syclCollectorObj->getProfileReport();

            for (const auto& deviceTable : syclReport) {
                const auto& report = deviceTable.second;

                std::string typeToPrint = " ";
                for (size_t i = 0; i < report.size(); i++) {
                    if (i == 0) {
                        typeToPrint = deviceTable.first + ":";
                    }
                    std::cout << std::setw(len + 20) << typeToPrint
                              << std::fixed << std::setprecision(2)
                              << std::setw(len) << report[i].timePercent << "%"
                              << std::setw(len) << report[i].totalTime
                              << std::setw(len) << report[i].count
                              << std::setw(len) << report[i].avg
                              << std::setw(len) << report[i].min
                              << std::setw(len) << report[i].max
                              << std::setw(60) << report[i].name
                              << std::endl;
                    typeToPrint = " ";
                }
            }

            delete syclCollectorObj;
            syclCollectorObj = nullptr;
        }
    }
    if (stream_name == std::string("sycl.pi")) {
        if (piApiCollectorObj != nullptr) {
            const auto& piApiReport = piApiCollectorObj->getProfileReport();

            std::string typeToPrint = " ";
            for (size_t i = 0; i < piApiReport.size(); i++) {
                if (i == 0) {
                    typeToPrint = "PI API calls:";
                }
                std::cout << std::setw(len + 20) << typeToPrint
                          << std::fixed << std::setprecision(2)
                          << std::setw(len) << piApiReport[i].timePercent << "%"
                          << std::setw(len) << piApiReport[i].totalTime
                          << std::setw(len) << piApiReport[i].count
                          << std::setw(len) << piApiReport[i].avg
                          << std::setw(len) << piApiReport[i].min
                          << std::setw(len) << piApiReport[i].max
                          << std::setw(60) << piApiReport[i].name
                          << std::endl;
                typeToPrint = " ";
            }

            delete piApiCollectorObj;
            piApiCollectorObj = nullptr;
        }
    }
}

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

    const auto& taskInfo = xptiUtils::extractTaskInfo(event, user_data);
    syclCollectorObj->addTask(event->unique_id, taskInfo);
}

XPTI_CALLBACK_API void taskExecCallback(uint16_t trace_type,
                                        xpti::trace_event_data_t *parent,
                                        xpti::trace_event_data_t *event,
                                        uint64_t instance,
                                        const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_begin)) {
        syclCollectorObj->addStartTask(event->unique_id);
    } else if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_end)) {
        syclCollectorObj->addEndTask(event->unique_id);
    } else {
        throw std::runtime_error("Can't handle trace point: " + std::to_string(trace_type) +
                                 ". taskExecCallback supports only task_begin and task_end!");
    }
}
