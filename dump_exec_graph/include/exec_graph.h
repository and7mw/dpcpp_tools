#pragma once

#include <unordered_map>
#include <vector>
#include <memory>

#include "xpti_utils.h"
#include "edge.h"

namespace dumpExecGraphTool {

class ExecGraph final {
    // std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks;
    std::vector<std::shared_ptr<Edge>> edges;

  public:
    std::unordered_map<size_t, std::shared_ptr<xptiUtils::Task>> tasks;

    void addEdge(const size_t parent, const size_t child, const std::string& name);
    void serialize() const;
    ~ExecGraph();
};

};
