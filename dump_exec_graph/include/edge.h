#pragma once

#include <cstddef>
#include <string>

namespace dumpExecGraphTool {

class Edge final {
    static size_t counter;

    size_t number;
    size_t parent, child;
    std::string origName, name;

  public:
    Edge(size_t parent, size_t child, const std::string& origName);

    size_t getParent() const { return parent; }
    size_t getChild() const { return child; }
    size_t getNumber() const { return number; }
    const std::string& getName() { return name; }
};

};
