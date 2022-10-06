#pragma once

#include <unordered_map>
#include <vector>
#include <memory>

#include "node.h"
#include "edge.h"

namespace dumpExecGraphTool {

class ExecGraph final {
    std::unordered_map<size_t, std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Edge>> edges;

  public:
    void addNode(const size_t id, const std::unordered_map<std::string, std::string> &metainfo);
    void addEdge(const size_t parent, const size_t child, const std::string& name);
    void addNodeStartExec(const size_t id);
    void addNodeEndExec(const size_t id);
    void serialize() const;
    ~ExecGraph();
};

};
