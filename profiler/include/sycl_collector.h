#pragma once

#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>

#include "xpti_utils.h"

namespace profilerTool {
    class syclCollector {
      public:
        using perTaskTypeStat = std::unordered_map<std::string, xptiUtils::profileEntry>;      
        std::unordered_map<std::string, perTaskTypeStat> getProfileReport() const;

      // private:
        std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks;
    };
};
