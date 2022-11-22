//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//

#include "xpti/xpti_trace_framework.hpp"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <atomic>
#include <sstream>

namespace xpti {
class ThreadID {
public:
  using thread_lut_t = std::unordered_map<std::string, int>;

  inline uint32_t enumID(std::thread::id &curr) {
    std::stringstream s;
    s << curr;
    std::string str(s.str());

    if (m_thread_lookup.count(str)) {
      return m_thread_lookup[str];
    } else {
      uint32_t enumID = m_tid++;
      m_thread_lookup[str] = enumID;
      return enumID;
    }
  }

  inline uint32_t enumID(const std::string &curr) {
    if (m_thread_lookup.count(curr)) {
      return m_thread_lookup[curr];
    } else {
      uint32_t enumID = m_tid++;
      m_thread_lookup[curr] = enumID;
      return enumID;
    }
  }

private:
  std::atomic<uint32_t> m_tid = {0};
  thread_lut_t m_thread_lookup;
};

namespace timer {
#include <cstdint>
using tick_t = uint64_t;
#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
inline xpti::timer::tick_t rdtsc() {
  LARGE_INTEGER qpcnt;
  int rval = QueryPerformanceCounter(&qpcnt);
  return qpcnt.QuadPart;
}
inline uint64_t getTSFrequency() {
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart * 1000;
}
inline uint64_t getCPU() { return GetCurrentProcessorNumber(); }
#else
#include <sched.h>
#include <time.h>
#if __x86_64__ || __i386__ || __i386
inline xpti::timer::tick_t rdtsc() {
  struct timespec ts;
  int status = clock_gettime(CLOCK_REALTIME, &ts);
  (void)status;
  return (static_cast<tick_t>(1000000000UL) * static_cast<tick_t>(ts.tv_sec) +
          static_cast<tick_t>(ts.tv_nsec));
}

inline uint64_t getTSFrequency() { return static_cast<uint64_t>(1E9); }

inline uint64_t getCPU() {
#ifdef __linux__
  return sched_getcpu();
#else
  return 0;
#endif
}
#else
#error Unsupported ISA
#endif

inline std::thread::id getThreadID() { return std::this_thread::get_id(); }
#endif
} // namespace timer
} // namespace xpti

static uint8_t GStreamID = 0;
std::mutex GIOMutex;
xpti::ThreadID GThreadIDEnum;

// The lone callback function we are going to use to demonstrate how to attach
// the collector to the running executable
XPTI_CALLBACK_API void tpCallback(uint16_t trace_type,
                                  xpti::trace_event_data_t *parent,
                                  xpti::trace_event_data_t *event,
                                  uint64_t instance, const void *user_data);

// Based on the documentation, every subscriber MUST implement the
// xptiTraceInit() and xptiTraceFinish() APIs for their subscriber collector to
// be loaded successfully.
XPTI_CALLBACK_API void xptiTraceInit(unsigned int major_version,
                                     unsigned int minor_version,
                                     const char *version_str,
                                     const char *stream_name) {
  // The basic collector will take in streams from anyone as we are just
  // printing out the stream data
  if (stream_name) {
    // Register this stream to get the stream ID; This stream may already have
    // been registered by the framework and will return the previously
    // registered stream ID
    GStreamID = xptiRegisterStream(stream_name);
    // Register our lone callback to all pre-defined trace point types
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::graph_create,
    //                      tpCallback);
    xptiRegisterCallback(
        GStreamID, (uint16_t)xpti::trace_point_type_t::node_create, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::edge_create, tpCallback);
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::region_begin,
    //                      tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::region_end, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::task_begin, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::task_end, tpCallback);
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::barrier_begin,
    //                      tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::barrier_end, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::lock_begin, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::lock_end, tpCallback);
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::transfer_begin,
    //                      tpCallback);
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::transfer_end,
    //                      tpCallback);
    // xptiRegisterCallback(GStreamID,
    //                      (uint16_t)xpti::trace_point_type_t::thread_begin,
    //                      tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::thread_end, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::wait_begin, tpCallback);
    // xptiRegisterCallback(
    //     GStreamID, (uint16_t)xpti::trace_point_type_t::wait_end, tpCallback);
    // xptiRegisterCallback(GStreamID, (uint16_t)xpti::trace_point_type_t::signal,
    //                      tpCallback);
    printf("Registered all callbacks\n");
  } else {
    // handle the case when a bad stream name has been provided
    std::cerr << "Invalid stream - no callbacks registered!\n";
  }
}

//
std::string truncate(std::string Name) {
  size_t Pos = Name.find_last_of(":");
  if (Pos != std::string::npos) {
    return Name.substr(Pos + 1);
  } else {
    return Name;
  }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
  // We do nothing here
}

XPTI_CALLBACK_API void tpCallback(uint16_t TraceType,
                                  xpti::trace_event_data_t *Parent,
                                  xpti::trace_event_data_t *Event,
                                  uint64_t Instance, const void *UserData) {
  auto Payload = xptiQueryPayload(Event);
  xpti::timer::tick_t Time = xpti::timer::rdtsc();
  auto TID = xpti::timer::getThreadID();
  uint32_t CPU = GThreadIDEnum.enumID(TID);
  std::string Name;

  if (Payload->name_sid() != xpti::invalid_id) {
    Name = truncate(Payload->name);
  } else {
    Name = "<unknown>";
  }

  uint64_t ID = Event ? Event->unique_id : 0;
  // Lock while we print information
  std::lock_guard<std::mutex> Lock(GIOMutex);
  // Print the record information
  std::string evName;
  if (UserData != nullptr) {
    evName = reinterpret_cast<const char *>(UserData);
    std::cout << "     === " << evName << " ===" << std::endl;
  }
  // printf("%-25lu: name=%-35s cpu=%3d event_id=%10lu\n", Time, Name.c_str(), CPU,
  //        ID);
  // Go through all available meta-data for an event and print it out
  // if (evName == "memory_transfer_node") {
  //   xpti::metadata_t *Metadata = xptiQueryMetadata(Event);
  //   for (const auto &Item : *Metadata) {
  //     const auto name = std::string(xptiLookupString(Item.first));
  //     if (name == "copy_from" || name == "copy_to") {
  //       std::cout << "          ";
  //       std::cout << name << " : ";
  //       std::cout << xpti::readMetadata(Item) << "\n";
  //     }
  //   }
  // }

  // if (Payload->source_file_sid() != xpti::invalid_id && Payload->line_no > 0) {
  //   printf("---[Source file:line no] %s:%d\n", Payload->source_file,
  //          Payload->line_no);
  // }
}

#if (defined(_WIN32) || defined(_WIN64))

#include <string>
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdReason, LPVOID lpvReserved) {
  switch (fwdReason) {
  case DLL_PROCESS_ATTACH:
    // printf("Framework initialization\n");
    break;
  case DLL_PROCESS_DETACH:
    //
    //  We cannot unload all subscribers here...
    //
    // printf("Framework finalization\n");
    break;
  }

  return TRUE;
}

#else // Linux (possibly macOS?)

__attribute__((constructor)) static void framework_init() {
  // printf("Framework initialization\n");
}

__attribute__((destructor)) static void framework_fini() {
  // printf("Framework finalization\n");
}

#endif
