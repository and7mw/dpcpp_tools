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

std::string truncate(std::string Name) {
  size_t Pos = Name.find_last_of(":");
  if (Pos != std::string::npos) {
    return Name.substr(Pos + 1);
  } else {
    return Name;
  }
}

xptiUtils::taskInfo xptiUtils::extractTaskInfo(xpti::trace_event_data_t *event, const void *user_data) {
    const std::string nodeType = reinterpret_cast<const char *>(user_data);
    const xpti::metadata_t *metadata = xptiQueryMetadata(event);
    std::unordered_map<std::string, std::string> taskInfo;

    taskInfo["node_type"] = nodeType;
    if (attrForTask.count(nodeType)) {
        const auto& attrs = attrForTask.at(nodeType);
        for (const auto &item : *metadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            if (std::find(attrs.begin(), attrs.end(), typeInfo) != attrs.end()) {
                taskInfo[typeInfo] = xpti::readMetadata(item);
            }
        }

        if (nodeType == xptiUtils::COMMAND_NODE) {
            std::string short_name = "<unknown>";

            auto payload = xptiQueryPayload(event);
            if (payload->name_sid() != xpti::invalid_id) {
                short_name = truncate(payload->name);
            }

            taskInfo["short_kernel_name"] = short_name;
        }
    } else {
        for (const auto &item : *metadata) {
            const std::string typeInfo = xptiLookupString(item.first);
            taskInfo[typeInfo] = xpti::readMetadata(item);
        }
    }

    return taskInfo;
}
