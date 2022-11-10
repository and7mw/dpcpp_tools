#include "xpti_utils.h"

#include <vector>
#include <string>
#include <algorithm>

namespace {
    const std::unordered_map<std::string, std::vector<std::string>> attrForTask = {
        {"command_group_node", std::vector<std::string>{"sycl_device_type",
                                                        "sycl_device",
                                                        "sycl_device_name",
                                                        "kernel_name"}},
        // TODO: fix copy_from / copy_to
        {"memory_transfer_node", std::vector<std::string>{"copy_from_id",
                                                          "sycl_device",
                                                          "copy_to_id",
                                                          "memory_object"}},
        {"memory_allocation_node", std::vector<std::string>{"memory_object",
                                                            "sycl_device_name",
                                                            "sycl_device",
                                                            "sycl_device_type"}},
        {"memory_deallocation_node", std::vector<std::string>{"sycl_device_name",
                                                              "sycl_device",
                                                              "sycl_device_type"}}
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

        if (nodeType == "command_group_node") {
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
