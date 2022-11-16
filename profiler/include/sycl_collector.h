#pragma once

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>

#include "xpti_utils.h"

namespace profilerTool {
    class syclCollector {
      public:        
        std::unordered_map<std::string, std::vector<xptiUtils::profileEntry>> getProfileReport() const;

      // private:
        std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks;
    };
};
