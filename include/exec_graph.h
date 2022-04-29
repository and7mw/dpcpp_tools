#pragma once

#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <functional>
#include "xpti_trace_framework.h"

// #define DEBUG

#ifdef DEBUG
#define DBG_PRINT(x) x
#else
#define DBG_PRINT(x)
#endif

struct PerfEntry {
    std::chrono::high_resolution_clock::time_point start, end;
};

struct Edge;

class Node {
public:
    long id;
    std::vector<PerfEntry> perfEntries;

public:
    std::vector<std::weak_ptr<Edge>> inEdges, outEdges;

public:
    Node() = default;
    virtual std::string serialize() const = 0;
    virtual ~Node() {};

protected:
    std::string getPertCounters() const {
        uint64_t total = 0;

        for (size_t i = 0; i < perfEntries.size(); i++) {
            total += std::chrono::duration_cast<std::chrono::microseconds>(perfEntries[i].end - perfEntries[i].start).count();
        }

        std::string res = "Avg time: " + std::to_string(static_cast<float>(total) / perfEntries.size()) +
                          " ms, launched " + std::to_string(perfEntries.size()) + " time";
        return res;
    }
};

class KernelNode : public Node {
public:
    KernelNode(const std::string &name, xpti::metadata_t *metadata, int64_t id) : name(name) {
        this->id = id;
        for (auto &it : *metadata) {
            if (xptiLookupString(it.first) == std::string("sycl_device")) {
                device = xptiLookupString(it.second);
                break;
            }
        }
    }

    std::string serialize() const override {
        std::string res;
        res += "Kernel: " + name + "\n";
        res += "Device: " + device + "\n";
        res += getPertCounters();

        return res;
    }

private:
    std::string name, device;
};

class MemoryNode : public Node {
public:
    // llvm/sycl/source/detail/scheduler/commands.cpp : commandToName(Command::CommandType Type)
    const static std::unordered_set<std::string> suppMemManagType;
    const static std::unordered_set<std::string> suppMemTransType;

    MemoryNode(const std::string &name, xpti::metadata_t *metadata, int64_t id) {
        this->id = id;

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
                addInfo += "from: " + from + " ";
            }
            if (!to.empty()) {
                addInfo += "to: " + to;
            }
        }
    }

    std::string serialize() const override {
        std::string res;
        res += "Mem op type: " + type + "\n";
        res += "Mem ptr: " + memObjPtr + "\n";
        if (!device.empty()) {
            res += "Device: " + device + "\n";
        }
        if (!addInfo.empty()) {
            res += addInfo + "\n";
        }
        res += getPertCounters();

        return res;
    }

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
    Edge(std::weak_ptr<Node> parent, std::weak_ptr<Node> child, std::string name) : parent(parent), child(child) {
        this->name = name;
        const auto pos = name.find("[");
        if (pos != std::string::npos) {
            this->name = name.substr(0, pos);
        }
    }

public:
    std::weak_ptr<Node> parent, child;
    std::string name;
};

class ExecGraph {
public:
    std::unordered_map<int64_t, std::shared_ptr<Node>> nodes;

    struct edgeCompare {
        bool operator()(const std::shared_ptr<Edge>& lhs, const std::shared_ptr<Edge>& rhs) const {
            return lhs->parent.lock()->id == rhs->parent.lock()->id &&
                   lhs->child.lock()->id == rhs->child.lock()->id &&
                   lhs->name == rhs->name;
        }
    };

    struct hash {
        size_t operator()(const std::shared_ptr<Edge>& edge) const {
            return std::hash<int64_t>{}(edge->parent.lock()->id) ^
                   (std::hash<int64_t>{}(edge->child.lock()->id) << 1) ^
                   (std::hash<std::string>{}(edge->name) << 2);
        }
    };

    std::unordered_set<std::shared_ptr<Edge>, hash, edgeCompare> edges;

public:
    void modifyExecGraph(uint16_t eventType, xpti::trace_event_data_t *event) {
        xpti::metadata_t *metadata = xptiQueryMetadata(event);
        switch (eventType) {
            case static_cast<uint16_t>(xpti::trace_point_type_t::node_create): {
                DBG_PRINT(std::cout << "node created with id: " << event->unique_id << std::endl;)

                addNode(event, metadata);
                break;
            }
            case static_cast<uint16_t>(xpti::trace_point_type_t::edge_create): {
                DBG_PRINT(std::cout << "edge created"
                                    << " parent: " << event->source_id
                                    << " child: " << event->target_id
                                    << std::endl;)

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

    ~ExecGraph() {
        // printf("ExecGraph DTOR: %lu %lu\n", this->nodes.size(), this->edges.size());
        std::ofstream dump;
        dump.open("/home/maxim/master_science_work/dpcpp_tools/dump.dot");

        dump << "digraph graphname {" << std::endl;
        for (const auto& node : nodes) {
            dump << "N" << node.first << " [label=\"" << node.second->serialize() << "\"];" << std::endl;
        }

        for (const auto& edge : edges) {
            dump << "N" << edge->parent.lock()->id << " -> N" << edge->child.lock()->id
            << " [label=\"" << edge->name << "\"];" << std::endl;
        }

        dump << "}" << std::endl;

        dump.close();
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
            nodes[event->unique_id] = std::make_shared<KernelNode>(name, metadata, event->unique_id);
        } else {
            const auto pos = name.find("[");
            std::string memType = name;
            if (pos != std::string::npos) {
                memType = name.substr(0, pos);
            }

            if (MemoryNode::suppMemManagType.count(memType) || MemoryNode::suppMemTransType.count(memType)) {
                nodes[event->unique_id] = std::make_shared<MemoryNode>(name, metadata, event->unique_id);
            } else {
                throw std::runtime_error("Unknown node type!");
            }
        }
    }

    void addEdge(xpti::trace_event_data_t *event) {
        const auto parent = nodes.at(event->source_id);
        const auto child = nodes.at(event->target_id);

        const xpti::payload_t *payload = xptiQueryPayload(event);
        std::string name;
        if (payload->name_sid != xpti::invalid_id) {
          name = payload->name;
        } else {
          throw std::runtime_error("Invalid event name!");
        }
        auto edge = std::make_shared<Edge>(parent, child, name);
        edges.insert(edge);

        parent->inEdges.push_back(edge);
        parent->outEdges.push_back(edge);
    }
};
