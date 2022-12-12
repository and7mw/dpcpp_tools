#include "exec_graph.h"
#include "xpti_utils.h"
#include "xpti_types.h"
#include "xpti/xpti_trace_framework.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <algorithm>

dumpExecGraphTool::ExecGraph* execGraph = nullptr;
std::mutex mutex;

XPTI_CALLBACK_API void nodeCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data);

XPTI_CALLBACK_API void edgeCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data);

XPTI_CALLBACK_API void taskCallback(uint16_t trace_type,
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
    if (stream_name == std::string(xptiUtils::SYCL_STREAM)) {
        execGraph = new dumpExecGraphTool::ExecGraph();

        const auto streamId = xptiRegisterStream(stream_name);

        // TODO: handle case with several graphs
        // xptiRegisterCallback(streamId,
        //                      static_cast<uint16_t>(xpti::trace_point_type_t::graph_create),
        //                      graphCreateCallback);

        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::node_create),
                             nodeCreateCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::edge_create),
                             edgeCreateCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::task_begin),
                             taskCallback);
        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::task_end),
                             taskCallback);

        xptiRegisterCallback(streamId,
                             static_cast<uint16_t>(xpti::trace_point_type_t::signal),
                             signalCallback);
    }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
    if (stream_name == std::string(xptiUtils::SYCL_STREAM)) {
        execGraph->serialize();
        delete execGraph;
    }
}

XPTI_CALLBACK_API void nodeCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    const auto& nodeInfo = xptiUtils::extractMetadata(event, user_data);

    xptiUtils::addTask(event->unique_id, nodeInfo, execGraph->tasks);
}

XPTI_CALLBACK_API void edgeCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    const auto payload = xptiQueryPayload(event);
    std::string name = "<unknown>";
    if (payload->name_sid() != xpti::invalid_id) {
        name = payload->name;
    }
    execGraph->addEdge(event->source_id, event->target_id, name);
}

XPTI_CALLBACK_API void taskCallback(uint16_t trace_type,
                                    xpti::trace_event_data_t *parent,
                                    xpti::trace_event_data_t *event,
                                    uint64_t instance,
                                    const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_begin)) {
        xptiUtils::addTaskStartExec(event->unique_id, execGraph->tasks);
    } else if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_end)) {
        xptiUtils::addTaskEndExec(event->unique_id, execGraph->tasks);
    } else {
        throw std::runtime_error("Can't handle trace point: " + std::to_string(trace_type) +
                                 ". Support only task_begin and task_end!");
    }
}

XPTI_CALLBACK_API void signalCallback(uint16_t trace_type,
                                      xpti::trace_event_data_t *parent,
                                      xpti::trace_event_data_t *event,
                                      uint64_t instance,
                                      const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    xptiUtils::addSignalHandler(event, user_data, execGraph->tasks);
}
