#include "exec_graph.h"
#include "xpti_types.h"

#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

void dumpExecGraphTool::ExecGraph::addEdge(const size_t parent,
                                           const size_t child,
                                           const std::string& name) {
    edges.push_back(std::make_shared<Edge>(parent, child, name));
}

void dumpExecGraphTool::ExecGraph::serialize() const {
    // const std::vector<std::pair<float, std::string>> painter{
    //     {50.0f, "red"},
    //     {20.0f, "orange"},
    //     {10.0f, "yellow"},
    //     {0.0f, "green"},
    // };

    // std::ofstream dump;
    // // TODO: make path os agnostic
    // dump.open("/home/maxim/master_science_work/dump.dot");
    // if (dump.fail()) {
    //     throw std::runtime_error("Opening file for dumping failed!");
    // }

    // uint64_t total = 0;
    // for (const auto& Task : tasks) {
    //     total += Task.second->getTotalTime();
    // }

    // // TODO: make metadata serialization more pretty
    // auto taskserialize = [](const std::shared_ptr<xptiUtils::Task> Task) {
    //     const auto& metadata = Task->getMetainfo();
    //     std::string result;

    //     for (const auto& mi : metadata) {
    //         result += mi.first + " : " + mi.second + "\n";
    //     }

    //     const double avgTime = static_cast<double>(Task->getTotalTime()) / Task->execDuration.size();
    //     std::stringstream stream;
    //     stream << "Avg time: " << std::fixed << std::setprecision(2) << avgTime
    //            << " ms, launched " << std::to_string(Task->execDuration.size()) << " times";
    //     result += stream.str();

    //     return result;
    // };

    // dump << "digraph graphname {" << std::endl;
    // for (const auto& Task : tasks) {
    //     dump << "N" << Task.first;
    //     dump << " [label=\"" << taskserialize(Task.second);
    //     const float percent = static_cast<float>(Task.second->getTotalTime()) / total * 100.0f;
    //     dump << std::fixed << std::setprecision(2) << ", " << percent << " %";
    //     dump << "\"";
    //     if (Task.second->getMetainfo().at("Task_type") != xptiUtils::COMMAND_NODE) {
    //         dump << ", shape=box";
    //     }
    //     for (const auto& p : painter) {
    //         if (percent >= p.first) {
    //             dump << ", style=filled, fillcolor=" << p.second;
    //             break;
    //         }
    //     }
    //     dump << "];" << std::endl;
    // }

    // for (const auto& edge : edges) {
    //     dump << "N" << edge->getParent() << " -> N" << edge->getChild()
    //     << " [label=\"" << edge->getName() << "_" << edge->getNumber() << "\"];" << std::endl;
    // }

    // dump << "}" << std::endl;

    // dump.close();
}

dumpExecGraphTool::ExecGraph::~ExecGraph() {
    serialize();
}
