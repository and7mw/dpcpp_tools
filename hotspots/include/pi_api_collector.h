#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>

#include "xpti_utils.h"

namespace profilerTool {
    class piApiCollector {
      public:
        void addFuncStartExec(const std::string name, const uint64_t id);
        void addFuncEndExec(const std::string name, const uint64_t id);

        std::vector<xptiUtils::profileEntry> getProfileReport() const;

      private:
        std::unordered_map<std::string, std::unordered_map<uint64_t, xptiUtils::TimeRecord>> records;
    };
};
