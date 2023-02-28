#include "exec_graph.h"
#include "xpti_types.h"

#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <unordered_set>

void dumpExecGraphTool::ExecGraph::addEdge(const size_t parent,
                                           const size_t child,
                                           const std::string& name) {
    edges.push_back(std::make_shared<Edge>(parent, child, name));
}

struct EdgeHash {
    size_t operator()(const std::shared_ptr<dumpExecGraphTool::Edge>& edge) const {
        return std::hash<size_t>()(edge->getChild()) << 2 +
               std::hash<size_t>()(edge->getParent()) << 4 +
               std::hash<std::string>()(edge->getName()) << 6;
    }
};

struct EdgeEq {
    size_t operator()(const std::shared_ptr<dumpExecGraphTool::Edge>& lhs,
                      const std::shared_ptr<dumpExecGraphTool::Edge>& rhs) const {
        return lhs->getChild() == rhs->getChild() &&
               lhs->getParent() == rhs->getParent() &&
               lhs->getName() == rhs->getName();
    }
};

void dumpExecGraphTool::ExecGraph::serialize() const {
    const std::vector<std::pair<float, std::string>> painter{
        {50.0f, "red"},
        {20.0f, "orange"},
        {10.0f, "yellow"},
        {0.0f, "green"},
    };

    auto statMap = xptiUtils::getPerTaskStatistic(tasks);
    std::vector<xptiUtils::profileEntry> statistics;
    statistics.reserve(statMap.size());

    uint64_t total = 0;
    for (const auto& task : statMap) {
        total += task.second.totalTime;
        statistics.emplace_back(task.second);
    }

    std::sort(statistics.begin(), statistics.end(), [](const xptiUtils::profileEntry& lhs,
                                                       const xptiUtils::profileEntry& rhs) {
        return lhs.order.front() < rhs.order.front();
    });

    // generate first part of graph
    std::stringstream firstPartGraph;
    firstPartGraph << "digraph graphname {" << std::endl;
    for (const auto& task : statistics) {
        firstPartGraph << "N" << task.id;
        firstPartGraph << " [label=\"" << task.name << "\n" << task.metadata;
        firstPartGraph << "Avg time: " << std::fixed << std::setprecision(2)
                       << static_cast<double>(task.totalTime) / task.count
                       << " μs, launched " << task.count << " times";
        const float percent = static_cast<double>(task.totalTime) / total * 100.0f;
        firstPartGraph << std::fixed << std::setprecision(2) << ", " << percent << " %";
        firstPartGraph << "\"";
        if (task.type != xptiUtils::COMMAND_NODE) {
            firstPartGraph << ", shape=box";
        }
        for (const auto& p : painter) {
            if (percent >= p.first) {
                firstPartGraph << ", style=filled, fillcolor=" << p.second;
                break;
            }
        }
        firstPartGraph << "];" << std::endl;
    }

    // open files for dump
    // TODO: restore
    // char currDirPath[] = "./";
    char currDirPath[] = "/home/maxim/master_science_work/";
    char* dumpPath = std::getenv(xptiUtils::GRAPH_DUMP_PATH);
    if (dumpPath == nullptr) {
        dumpPath = currDirPath;
    }
    std::ofstream dumpGraphUniq, dumpGraphMult;
    std::string dumpGraphUniqName("graph_uniq_edges.dot"), dumpGraphMultName("graph_mult_edges.dot");
    dumpGraphUniq.open(dumpPath + dumpGraphUniqName);
    dumpGraphMult.open(dumpPath + dumpGraphMultName);
    if (dumpGraphUniq.fail()) {
        throw std::runtime_error("Can't open " + dumpGraphUniqName);
    }
    if (dumpGraphMult.fail()) {
        throw std::runtime_error("Can't open " + dumpGraphMultName);
    }

    // generate second part of graph
    // unique edges
    std::unordered_set<std::shared_ptr<Edge>, EdgeHash, EdgeEq> uniqueEdges(edges.begin(), edges.end());
    dumpGraphUniq << firstPartGraph.str();
    for (const auto& edge : uniqueEdges) {
        dumpGraphUniq << "N" << edge->getParent() << " -> N" << edge->getChild()
                      << " [label=\"" << edge->getName() << "\"];" << std::endl;
    }
    dumpGraphUniq << "}" << std::endl;
    // mult edges
    dumpGraphMult << firstPartGraph.str();
    for (const auto& edge : edges) {
        dumpGraphMult << "N" << edge->getParent() << " -> N" << edge->getChild()
                      << " [label=\"" << edge->getName() << "\"];" << std::endl;
    }
    dumpGraphMult << "}" << std::endl;

    // close files
    dumpGraphUniq.close();
    dumpGraphMult.close();
}
