#pragma once

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>

namespace profilerTool {
    class syclCollector {
      public:
        // TODO: extract
        struct Record {
            std::chrono::high_resolution_clock::time_point start, end;
        };
        
        void addTask(const uint64_t id, const std::unordered_map<std::string, std::string> &info);
        void addStartTask(const uint64_t id);
        void addEndTask(const uint64_t id);

      private:
        std::unordered_map<uint64_t, std::unordered_map<std::string, std::string>> tasksInfo;
        std::unordered_map<uint64_t, std::vector<Record>> tasksTime;
    };
};
