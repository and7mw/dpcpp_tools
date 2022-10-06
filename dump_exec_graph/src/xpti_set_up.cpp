#include "exec_graph.h"
#include "xpti/xpti_trace_framework.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <algorithm>

std::unique_ptr<dumpExecGraphTool::ExecGraph> execGraph = nullptr;
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

// TODO: handle major_version, minor_version, version_str?
XPTI_CALLBACK_API void xptiTraceInit(unsigned int major_version,
                                     unsigned int minor_version,
                                     const char *version_str,
                                     const char *stream_name) {    
    const std::string expectedStream("sycl");

    if (stream_name == expectedStream) {
        execGraph = std::make_unique<dumpExecGraphTool::ExecGraph>();

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
    }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {}

namespace {
    const std::unordered_map<std::string, std::vector<std::string>> attrForNode = {
        {"command_group_node", std::vector<std::string>{"sycl_device_type",
                                                        "sycl_device",
                                                        "sycl_device_name",
                                                        "kernel_name"}},
        {"memory_transfer_node", std::vector<std::string>{"copy_from",
                                                          "copy_to",
                                                          "memory_object"}},
        {"memory_allocation_node", std::vector<std::string>{"memory_object",
                                                            "sycl_device_name",
                                                            "sycl_device",
                                                            "sycl_device_type"}},
        {"memory_deallocation_node", std::vector<std::string>{"sycl_device_name",
                                                              "sycl_device",
                                                              "sycl_device_type"}}
    };
};

XPTI_CALLBACK_API void nodeCreateCallback(uint16_t trace_type,
                                          xpti::trace_event_data_t *parent,
                                          xpti::trace_event_data_t *event,
                                          uint64_t instance,
                                          const void *user_data) {
    std::lock_guard<std::mutex> lock(mutex);

    const std::string nodeType = reinterpret_cast<const char *>(user_data);
    const xpti::metadata_t *metadata = xptiQueryMetadata(event);
    std::unordered_map<std::string, std::string> metainfo;

    metainfo["node_type"] = nodeType;
    if (attrForNode.count(nodeType)) {
        const auto& attrs = attrForNode.at(nodeType);
        for (const auto &item : *metadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            if (std::find(attrs.begin(), attrs.end(), typeInfo) != attrs.end()) {
                metainfo[typeInfo] = xpti::readMetadata(item);
            }
        }
    } else {
        for (const auto &item : *metadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            metainfo[typeInfo] = xpti::readMetadata(item);
        }
    }

    execGraph->addNode(event->unique_id, metainfo);
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
        execGraph->addNodeStartExec(event->unique_id);
    } else if (trace_type == static_cast<uint16_t>(xpti::trace_point_type_t::task_end)) {
        execGraph->addNodeEndExec(event->unique_id);
    } else {
        throw std::runtime_error("Can't handle trace point: " + std::to_string(trace_type) +
                                 ". Support only task_begin and task_end!");
    }
}
