#pragma once

#include <string>
#include <chrono>
#include <utility>
#include <unordered_map>

namespace dumpExecGraphTool {

struct Node final {
    struct DurationEntry {
        std::chrono::high_resolution_clock::time_point start, end;
    };
    std::vector<DurationEntry> execDuration;

    // TODO: cleanup
    Node(/*const size_t& id, */const std::unordered_map<std::string, std::string> &metainfo) : /*id(id),*/ metainfo(metainfo) {}
    const std::unordered_map<std::string, std::string>& getMetainfo() const { return metainfo; }
    // const size_t getId() const { return id; }
    uint64_t getTotalTime() const {
        uint64_t total = 0;
        for (const auto& d : execDuration) {
            total += std::chrono::duration_cast<std::chrono::microseconds>(d.end - d.start).count();
        }
        return total;
    }

  private:
    std::unordered_map<std::string, std::string> metainfo;
    // size_t id;
};

};
