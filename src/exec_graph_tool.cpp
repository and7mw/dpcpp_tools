#include "exec_graph.h"
#include "xpti_trace_framework.h"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

static uint8_t GStreamID = 0;
std::mutex mutex;

XPTI_CALLBACK_API void tpCallback(uint16_t trace_type,
                                  xpti::trace_event_data_t *parent,
                                  xpti::trace_event_data_t *event,
                                  uint64_t instance, const void *user_data);

std::unique_ptr<ExecGraph> execGraph = nullptr;

XPTI_CALLBACK_API void xptiTraceInit(unsigned int major_version,
                                     unsigned int minor_version,
                                     const char *version_str,
                                     const char *stream_name) {
  if (stream_name) {
    GStreamID = xptiRegisterStream(stream_name);
    char *tstr;
    xptiRegisterString("sycl_device", &tstr);

    // FIXME: handle case when several graphs created
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::graph_create,
    //                      tpCallback);

    xptiRegisterCallback(
        GStreamID, (uint16_t)xpti::trace_point_type_t::node_create, tpCallback);
    xptiRegisterCallback(
        GStreamID, (uint16_t)xpti::trace_point_type_t::edge_create, tpCallback);

    xptiRegisterCallback(
        GStreamID, (uint16_t)xpti::trace_point_type_t::task_begin, tpCallback);
    xptiRegisterCallback(
        GStreamID, (uint16_t)xpti::trace_point_type_t::task_end, tpCallback);

    execGraph = std::make_unique<ExecGraph>();

    printf("Registered all callbacks\n");
  } else {
    printf("Invalid stream - no callbacks registered!\n");
  }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
  printf("xptiTraceFinish: %lu %lu\n", execGraph->nodes.size(), execGraph->edges.size());
}

XPTI_CALLBACK_API void tpCallback(uint16_t TraceType,
                                  xpti::trace_event_data_t *Parent,
                                  xpti::trace_event_data_t *Event,
                                  uint64_t Instance, const void *UserData) {
  std::lock_guard<std::mutex> Lock(mutex);
  execGraph->modifyExecGraph(TraceType, Event);
}

__attribute__((constructor)) static void framework_init() {}

__attribute__((destructor)) static void framework_fini() {}
