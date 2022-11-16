#include "xpti_utils.h"
#include "xpti_types.h"

#include <vector>
#include <string>
#include <algorithm>

namespace {
    const std::unordered_map<std::string, std::vector<std::string>> attrForTask = {
        {xptiUtils::COMMAND_NODE, std::vector<std::string>{xptiUtils::DEVICE_TYPE,
                                                           xptiUtils::DEVICE_ID,
                                                           xptiUtils::DEVICE_NAME,
                                                           xptiUtils::KERNEL_NAME}},
        // TODO: fix copy_from / copy_to
        {xptiUtils::MEM_TRANSF_NODE, std::vector<std::string>{xptiUtils::COPY_FROM,
                                                             xptiUtils::DEVICE_ID,
                                                             xptiUtils::COPY_TO,
                                                             xptiUtils::MEM_OBJ}},
        {xptiUtils::MEM_ALLOC_NODE, std::vector<std::string>{xptiUtils::MEM_OBJ,
                                                             xptiUtils::DEVICE_NAME,
                                                             xptiUtils::DEVICE_ID,
                                                             xptiUtils::DEVICE_TYPE}},
        {xptiUtils::MEM_DEALLOC_NODE, std::vector<std::string>{xptiUtils::DEVICE_NAME,
                                                               xptiUtils::DEVICE_ID,
                                                               xptiUtils::DEVICE_TYPE}}
    };
};

std::unordered_map<std::string, std::string>
xptiUtils::extractMetadata(xpti::trace_event_data_t *event,const void *userData) {
    const std::string nodeType = reinterpret_cast<const char *>(userData);
    std::unordered_map<std::string, std::string> metadata;

    const xpti::metadata_t *xptiMetadata = xptiQueryMetadata(event);

    metadata["node_type"] = nodeType;
    if (attrForTask.count(nodeType)) {
        const auto& attrs = attrForTask.at(nodeType);
        for (const auto &item : *xptiMetadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            if (std::find(attrs.begin(), attrs.end(), typeInfo) != attrs.end()) {
                metadata[typeInfo] = xpti::readMetadata(item);
            }
        }
    } else {
        for (const auto &item : *xptiMetadata) {
            metadata[xptiLookupString(item.first)] = xpti::readMetadata(item);
        }
    }

    return metadata;
}
