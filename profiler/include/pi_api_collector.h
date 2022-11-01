#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

namespace profilerTool {
    class piApiCollector {
      public:
        struct Record {
            std::chrono::high_resolution_clock::time_point start, end;
        };

        void addFuncStartExec(const std::string name, const uint64_t id);
        void addFuncEndExec(const std::string name, const uint64_t id);

      private:
        std::unordered_map<std::string, std::unordered_map<uint64_t, Record>> records;
    };
};