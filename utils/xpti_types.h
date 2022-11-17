#pragma once

namespace xptiUtils {
    // xpti streams
    inline const char* SYCL_STREAM = "sycl";
    inline const char* SYCL_PI_STREAM = "sycl.pi";

    // node types
    inline const char* COMMAND_NODE = "command_group_node";
    inline const char* MEM_TRANSF_NODE = "memory_transfer_node";
    inline const char* MEM_ALLOC_NODE = "memory_allocation_node";
    inline const char* MEM_DEALLOC_NODE = "memory_deallocation_node";

    // metadata keys
    inline const char* DEVICE_TYPE = "sycl_device_type";
    inline const char* DEVICE_ID = "sycl_device";
    inline const char* DEVICE_NAME = "sycl_device_name";
    inline const char* KERNEL_NAME = "kernel_name";
    inline const char* COPY_FROM = "copy_from_id";
    inline const char* COPY_TO = "copy_to_id";
    inline const char* MEM_OBJ = "memory_object";

    // env var
    inline const char* GRAPH_DUMP_PATH = "GRAPH_DUMP_PATH";

    // aux
    inline const char* NODE_TYPE = "node_type";
};
