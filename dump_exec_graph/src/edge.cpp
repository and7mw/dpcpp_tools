#include "edge.h"

size_t dumpExecGraphTool::Edge::counter = 0;

dumpExecGraphTool::Edge::Edge(size_t parent,
                              size_t child,
                              const std::string& origName) : parent(parent), child(child), origName(origName) {
    name = origName;
    size_t pos = name.find_last_of(":");
    if (pos != std::string::npos) {
        name = name.substr(pos + 1);
    }
    pos = name.find_last_of("[");
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }

    number = counter++;
}
