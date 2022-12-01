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

#include "CL/sycl/detail/pi.h"

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
    xptiRegisterCallback(GStreamID, (uint16_t)xpti::trace_point_type_t::signal,
                         tpCallback);
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

_pi_plugin* plg;
_pi_event * pi_ev;

XPTI_CALLBACK_API void xptiTraceFinish(const char *stream_name) {
  std::cout << "xptiTraceFinish" << std::endl;
    if (plg != nullptr) {
      uint64_t time1 = 123;
      auto ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_START,
        sizeof(uint64_t),
        &time1,
        nullptr
      );
      uint64_t time2 = 123;
      ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_END,
        sizeof(uint64_t),
        &time2,
        nullptr
      );
      std::cout << ret << " : " << time2 << " " << time1 << std::endl;
    }
}

XPTI_CALLBACK_API void tpCallback(uint16_t TraceType,
                                  xpti::trace_event_data_t *Parent,
                                  xpti::trace_event_data_t *Event,
                                  uint64_t Instance, const void *UserData) {
  auto Payload = xptiQueryPayload(Event);

  std::string Name;
  if (Payload->name_sid() != xpti::invalid_id) {
    Name = truncate(Payload->name);
  } else {
    Name = "<unknown>";
  }
  
  if (Name == "Sub") {
    plg = static_cast<_pi_plugin*>(Event->global_user_data);
    pi_ev = static_cast<_pi_event *>(const_cast<void *>(UserData));

std::cout << "ASSIGN KERNEL : " << pi_ev << std::endl;

    uint64_t time1 = 123;
      auto ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_START,
        sizeof(uint64_t),
        &time1,
        nullptr
      );
      uint64_t time2 = 123;
      ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_END,
        sizeof(uint64_t),
        &time2,
        nullptr
      );
      std::cout << ret << " : " << time2 << " " << time1 << std::endl;
  }

  std::cout << "tpCallback end!" << std::endl;
}

__attribute__((destructor)) static void fwFinialize() {
    std::cout << "DESTROY" << std::endl;
    if (plg != nullptr) {
      uint64_t time1 = 123;
      auto ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_START,
        sizeof(uint64_t),
        &time1,
        nullptr
      );
      uint64_t time2 = 123;
      ret = (*plg->PiFunctionTable.piEventGetProfilingInfo)(
        pi_ev,
        _pi_profiling_info::PI_PROFILING_INFO_COMMAND_END,
        sizeof(uint64_t),
        &time2,
        nullptr
      );
      std::cout << ret << " : " << time2 << " " << time1 << std::endl;
    }
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

// __attribute__((constructor)) static void framework_init() {
//   // printf("Framework initialization\n");
// }

// __attribute__((destructor)) static void framework_fini() {
//   // printf("Framework finalization\n");
// }

#endif
