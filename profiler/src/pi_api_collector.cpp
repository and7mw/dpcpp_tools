#include "pi_api_collector.h"

#include <stdexcept>

void profilerTool::piApiCollector::addFuncStartExec(const std::string name, const uint64_t id) {
    auto &times = records[name];

    if (times.count(id) == 0) {
        const std::string error = "Function " + name + " with id: " +
                                   std::to_string(id) + " already started execution!";
        throw std::runtime_error(error);
    }

    auto &curr_record = times[id];
    curr_record.start = std::chrono::high_resolution_clock::now();
}

void profilerTool::piApiCollector::addFuncEndExec(const std::string name, const uint64_t id) {
    const std::string error = "Function " + name + " with id: " +
                               std::to_string(id) + " haven't started execution!";

    if (records.count(name)) {
        throw std::runtime_error(error);
    }
    
    auto &times = records[name];
    if (times.count(id)) {
        throw std::runtime_error(error);
    }

    auto &curr_record = times[id];
    curr_record.end = std::chrono::high_resolution_clock::now();
}
