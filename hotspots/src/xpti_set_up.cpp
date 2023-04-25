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
#include <sstream>

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

std::string getReport(const std::vector<xptiUtils::profileEntry>& stat, const std::string& type) {
    std::stringstream ss;
    
    std::string typeToPrint = " ";
    for (size_t i = 0; i < stat.size(); i++) {
        if (i == 0) {
            typeToPrint = type + ":";
        }
        ss << std::setw(typeOffset) << typeToPrint
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
    ss << std::endl;

    return ss.str();
}

std::vector<std::string> getSyclReport() {
    std::vector<std::string> res_report;
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

            res_report.push_back(getReport(report, device.first));
        }
        delete syclCollectorObj;
        syclCollectorObj = nullptr;
    }

    return res_report;
}

std::string getSyclPiReport() {
    std::string report;
    if (piApiCollectorObj != nullptr) {
        const auto& piApiReport = piApiCollectorObj->getProfileReport();

        report = getReport(piApiReport, "PI API calls");

        delete piApiCollectorObj;
        piApiCollectorObj = nullptr;
    }
    return report;
}

int report_printed = 0;

int sycl_prepared = 0;
char *sycl_report = nullptr;

int pi_prepared = 0;
char *pi_report = nullptr;

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
    if (stream_name == std::string(xptiUtils::SYCL_PI_STREAM)) {
        std::string ret_pi = getSyclPiReport();
        pi_report = new char[ret_pi.size() + 1];
        std::memcpy(pi_report, ret_pi.data(), sizeof(char) * ret_pi.size());
        pi_report[ret_pi.size()] = '\0';

        pi_prepared = 1;
    } else if (stream_name == std::string(xptiUtils::SYCL_STREAM)) {
        std::vector<std::string> ret_sycl = getSyclReport();

        size_t alloc_size = 0;
        for (size_t i = 0; i < ret_sycl.size(); i++) {
            alloc_size += ret_sycl[i].size();
        }

        sycl_report = new char[alloc_size + 1];
        for (size_t i = 0; i < ret_sycl.size(); i++) {
            std::memcpy(sycl_report + i * ret_sycl[i].size(), ret_sycl[i].data(),
                        sizeof(char) * ret_sycl[i].size());
        }
        sycl_report[alloc_size] = '\0';

        sycl_prepared = 1;
    }

    if (!report_printed && pi_prepared && sycl_prepared) {
        std::cout << std::setw(typeOffset) << "Type"
                  << std::setw(defaultOffset + 1) << "Time(%)"
                  << std::setw(defaultOffset + 1) << "Time(μs)"
                  << std::setw(defaultOffset + 1) << "Calls"
                  << std::setw(defaultOffset + 1) << "Avg(μs)"
                  << std::setw(defaultOffset + 1) << "Min(μs)"
                  << std::setw(defaultOffset + 1) << "Max(μs)"
                  << std::setw(nameOffset) << "Name"
                  << std::endl;

        std::cout << std::string(sycl_report);
        std::cout << std::string(pi_report);

        delete [] sycl_report;
        delete [] pi_report;

        report_printed = 1;
    }
}
