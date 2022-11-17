#pragma once

#include <utility>
#include <unordered_map>
#include <limits>
#include <string>
#include <chrono>

#include "xpti/xpti_trace_framework.hpp"

namespace xptiUtils {
    struct profileEntry {
        profileEntry() = default;

        // metrics
        float timePercent = 0.0f;
        size_t totalTime = 0;
        size_t count = 0;
        double avg = 0.0f;
        size_t min = std::numeric_limits<size_t>::max();
        size_t max = 0.0;
        std::string name;

        // aux info
        size_t id;
        std::string type;
        std::string deviceName;
        std::string metadata;
    };
};

namespace xptiUtils {
    struct TimeRecord {
        std::chrono::high_resolution_clock::time_point start, end;
    };

    struct Task final {
        std::vector<TimeRecord> execDuration;

        Task(const std::unordered_map<std::string, std::string> &metainfo) : metainfo(metainfo) {}

        const std::unordered_map<std::string, std::string>& getMetainfo() const { return metainfo; }
        uint64_t getTotalTime() const {
            uint64_t total = 0;
            for (const auto& d : execDuration) {
                total += std::chrono::duration_cast<std::chrono::microseconds>(d.end - d.start).count();
            }
            return total;
        }

      private:
        std::unordered_map<std::string, std::string> metainfo;
    };

    void addTask(const size_t id,
                 const std::unordered_map<std::string, std::string> &metainfo,
                 std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks);
    void addTaskStartExec(const size_t id, std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks);
    void addTaskEndExec(const size_t id, std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> &tasks);
};

namespace xptiUtils {
    std::unordered_map<std::string, std::string> extractMetadata(xpti::trace_event_data_t *event,
                                                                 const void *userData);
    
    using perTaskStatistic = std::unordered_map<size_t, xptiUtils::profileEntry>;
    perTaskStatistic getPerTaskStatistic(const std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks);
};
