#include "exec_graph.h"
#include "xpti_types.h"

#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstdlib>

void dumpExecGraphTool::ExecGraph::addEdge(const size_t parent,
                                           const size_t child,
                                           const std::string& name) {
    edges.push_back(std::make_shared<Edge>(parent, child, name));
}

void dumpExecGraphTool::ExecGraph::serialize() const {
    const std::vector<std::pair<float, std::string>> painter{
        {50.0f, "red"},
        {20.0f, "orange"},
        {10.0f, "yellow"},
        {0.0f, "green"},
    };

    const auto& statistics = xptiUtils::getPerTaskStatistic(tasks);

    uint64_t total = 0;
    for (const auto& task : statistics) {
        total += task.second.totalTime;
    }

    // generate first part of graph
    std::stringstream firstPartGraph;
    firstPartGraph << "digraph graphname {" << std::endl;
    for (const auto& task : statistics) {
        firstPartGraph << "N" << task.first;
        firstPartGraph << " [label=\"" << task.second.name << "\n" << task.second.metadata;
        firstPartGraph << "Avg time: " << std::fixed << std::setprecision(2)
             << static_cast<double>(task.second.totalTime) / task.second.count
             << " ms, launched " << task.second.count << " times";
        const float percent = static_cast<double>(task.second.totalTime) / total * 100.0f;
        firstPartGraph << std::fixed << std::setprecision(2) << ", " << percent << " %";
        firstPartGraph << "\"";
        if (task.second.type != xptiUtils::COMMAND_NODE) {
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
    char currDirPath[] = "./";
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
    auto uniqueEdges = edges;
    auto lastEdge = std::unique(uniqueEdges.begin(), uniqueEdges.end(),
                                [](const std::shared_ptr<Edge>& lhs, const std::shared_ptr<Edge>& rhs) {
                                    return lhs->getChild() == rhs->getChild() &&
                                            lhs->getParent() == rhs->getParent();
                                });
    uniqueEdges.erase(lastEdge, uniqueEdges.end());
    dumpGraphUniq << firstPartGraph.str();
    for (const auto& edge : uniqueEdges) {
        dumpGraphUniq << "N" << edge->getParent() << " -> N" << edge->getChild() << std::endl;
    }
    dumpGraphUniq << "}" << std::endl;
    // mult edges
    dumpGraphMult << firstPartGraph.str();
    for (const auto& edge : edges) {
        dumpGraphMult << "N" << edge->getParent() << " -> N" << edge->getChild() << std::endl;
    }
    dumpGraphMult << "}" << std::endl;
    
    // close files
    dumpGraphUniq.close();
    dumpGraphMult.close();
}

dumpExecGraphTool::ExecGraph::~ExecGraph() {
    serialize();
}
