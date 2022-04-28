#pragma once

#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "xpti_trace_framework.h"

struct PerfEntry {
    std::chrono::high_resolution_clock::time_point start, end;
};

struct Edge;

class Node {
public:
    std::vector<PerfEntry> perfEntries;

public:
    std::vector<std::weak_ptr<Edge>> inEdges, outEdges;

public:
    Node() = default;
    virtual void serialize() const = 0;
    virtual ~Node() {};
};

class KernelNode : public Node {
public:
    KernelNode(const std::string &name, xpti::metadata_t *metadata) : name(name) {
        for (auto &it : *metadata) {
            if (xptiLookupString(it.first) == std::string("sycl_device")) {
                device = xptiLookupString(it.second);
                break;
            }
        }
    }

    void serialize() const override;

private:
    std::string name, device;
};

class MemoryNode : public Node {
public:
    // llvm/sycl/source/detail/scheduler/commands.cpp : commandToName(Command::CommandType Type)
    const static std::unordered_set<std::string> suppMemManagType;
    const static std::unordered_set<std::string> suppMemTransType;

    MemoryNode(const std::string &name, xpti::metadata_t *metadata) {
        type = name;

        const auto pos = name.find("[");
        if (pos != std::string::npos) {
            type = name.substr(0, pos);
            memObjPtr = name.substr(pos, name.size());
        }

        if (suppMemTransType.count(type)) {
            std::string from, to;
            for (auto &it : *metadata) {
                if (xptiLookupString(it.first) == std::string("copy_from")) {
                    from = xptiLookupString(it.second);
                }
                if (xptiLookupString(it.first) == std::string("copy_to")) {
                    to = xptiLookupString(it.second);
                }
                if (xptiLookupString(it.first) == std::string("sycl_device")) {
                    device = xptiLookupString(it.second);
                    break;
                }
            }

            if (!from.empty()) {
                addInfo += "FROM: " + from + " ";
            }
            if (!to.empty()) {
                addInfo += "TO: " + to;
            }
        }
    }

    void serialize() const override;

private:
    std::string type, memObjPtr, addInfo, device;
};

const std::unordered_set<std::string> MemoryNode::suppMemManagType = {
    "Memory Allocation",
    "Memory Deallocation"
};

const std::unordered_set<std::string> MemoryNode::suppMemTransType = {
    "Memory Transfer (Copy)",
    "Memory Transfer (Map)",
    "Memory Transfer (Unmap)"
};

class Edge {
public:
    Edge(std::weak_ptr<Node> parent, std::weak_ptr<Node> child) : parent(parent), child(child) {}

private:
    std::weak_ptr<Node> parent, child;
};

class ExecGraph {
    std::unordered_map<int64_t, std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Edge>> edges;

public:
    void modifyExecGraph(uint16_t eventType, xpti::trace_event_data_t *event, xpti::metadata_t *metadata) {
        switch (eventType) {
            case static_cast<uint16_t>(xpti::trace_point_type_t::node_create): {
                addNode(event, metadata);
                break;
            }
            case static_cast<uint16_t>(xpti::trace_point_type_t::edge_create): {
                addEdge(event);
                break;
            }
            case static_cast<uint16_t>(xpti::trace_point_type_t::task_begin): {
                nodes[event->unique_id]->perfEntries.push_back(PerfEntry());
                nodes[event->unique_id]->perfEntries.back().start = std::chrono::high_resolution_clock::now();
                break;
            }
            case static_cast<uint16_t>(xpti::trace_point_type_t::task_end): {
                nodes[event->unique_id]->perfEntries.back().end = std::chrono::high_resolution_clock::now();
                break;
            }
            default: {
                throw std::runtime_error("Can't handle event with id = " + std::to_string(eventType));
            }
        }
    }

private:
    void addNode(xpti::trace_event_data_t *event, xpti::metadata_t *metadata) {
        const xpti::payload_t *payload = xptiQueryPayload(event);
        std::string name;

        if (payload->name_sid != xpti::invalid_id) {
          name = payload->name;
        } else {
          throw std::runtime_error("Invalid event name!");
        }

        bool isKernel = false;
        for (auto &it : *metadata) {
            if (xptiLookupString(it.first) == std::string("kernel_name")) {
                isKernel = true;
                break;
            }
        }

        if (isKernel) {
            nodes[event->unique_id] = std::make_shared<KernelNode>(name, metadata);
        } else {
            const auto pos = name.find("[");
            std::string memType = name;
            if (pos != std::string::npos) {
                memType = name.substr(0, pos);
            }

            if (MemoryNode::suppMemManagType.count(memType) || MemoryNode::suppMemTransType.count(memType)) {
                nodes[event->unique_id] = std::make_shared<MemoryNode>(name, metadata);
            } else {
                throw std::runtime_error("Unknown node type!");
            }
        }
    }

    void addEdge(xpti::trace_event_data_t *event) {
        const auto parent = nodes.at(event->source_id);
        const auto child = nodes.at(event->target_id);

        auto edge = std::make_shared<Edge>(parent, child);
        parent->inEdges.push_back(edge);
        parent->outEdges.push_back(edge);
    }
};
