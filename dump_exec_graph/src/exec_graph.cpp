#include "exec_graph.h"
#include "xpti_types.h"

#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

void dumpExecGraphTool::ExecGraph::addNode(const size_t id,
                                           const std::unordered_map<std::string, std::string> &metainfo) {
    if (nodes.count(id)) {
        throw std::runtime_error("Can't add node with id: " + std::to_string(id) + ". Already exist in graph!");
    }
    nodes[id] = std::make_shared<Node>(metainfo);
}

void dumpExecGraphTool::ExecGraph::addEdge(const size_t parent,
                                           const size_t child,
                                           const std::string& name) {
    edges.push_back(std::make_shared<Edge>(parent, child, name));
}

void dumpExecGraphTool::ExecGraph::addNodeStartExec(const size_t id) {
    if (!nodes.count(id)) {
        throw std::runtime_error("Can't add start time for node with id: " + std::to_string(id) +
                                 ". Node missing in graph!");
    }
    nodes[id]->execDuration.push_back(Node::DurationEntry());
    nodes[id]->execDuration.back().start = std::chrono::high_resolution_clock::now();
}

void dumpExecGraphTool::ExecGraph::addNodeEndExec(const size_t id) {
    if (!nodes.count(id)) {
        throw std::runtime_error("Can't add end time for node with id: " + std::to_string(id) +
                                 ". Node missing in graph!");
    }
    if (nodes[id]->execDuration.empty()) {
        throw std::runtime_error("Can't add end time for node with id: " + std::to_string(id) +
                                 ". Start time for node don't set!");
    }
    nodes[id]->execDuration.back().end = std::chrono::high_resolution_clock::now();
}

void dumpExecGraphTool::ExecGraph::serialize() const {
    const std::vector<std::pair<float, std::string>> painter{
        {50.0f, "red"},
        {20.0f, "orange"},
        {10.0f, "yellow"},
        {0.0f, "green"},
    };

    std::ofstream dump;
    // TODO: make path os agnostic
    dump.open("/home/maxim/master_science_work/dump.dot");
    if (dump.fail()) {
        throw std::runtime_error("Opening file for dumping failed!");
    }

    uint64_t total = 0;
    for (const auto& node : nodes) {
        total += node.second->getTotalTime();
    }

    // TODO: make metadata serialization more pretty
    auto nodeSerialize = [](const std::shared_ptr<Node> node) {
        const auto& metadata = node->getMetainfo();
        std::string result;

        for (const auto& mi : metadata) {
            result += mi.first + " : " + mi.second + "\n";
        }

        const double avgTime = static_cast<double>(node->getTotalTime()) / node->execDuration.size();
        std::stringstream stream;
        stream << "Avg time: " << std::fixed << std::setprecision(2) << avgTime
               << " ms, launched " << std::to_string(node->execDuration.size()) << " times";
        result += stream.str();

        return result;
    };

    dump << "digraph graphname {" << std::endl;
    for (const auto& node : nodes) {
        dump << "N" << node.first;
        dump << " [label=\"" << nodeSerialize(node.second);
        const float percent = static_cast<float>(node.second->getTotalTime()) / total * 100.0f;
        dump << std::fixed << std::setprecision(2) << ", " << percent << " %";
        dump << "\"";
        if (node.second->getMetainfo().at("node_type") != xptiUtils::COMMAND_NODE) {
            dump << ", shape=box";
        }
        for (const auto& p : painter) {
            if (percent >= p.first) {
                dump << ", style=filled, fillcolor=" << p.second;
                break;
            }
        }
        dump << "];" << std::endl;
    }

    for (const auto& edge : edges) {
        dump << "N" << edge->getParent() << " -> N" << edge->getChild()
        << " [label=\"" << edge->getName() << "_" << edge->getNumber() << "\"];" << std::endl;
    }

    dump << "}" << std::endl;

    dump.close();
}

dumpExecGraphTool::ExecGraph::~ExecGraph() {
    serialize();
}
