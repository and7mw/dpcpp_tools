# Performance analysis tools for DPC++

## Overview

This repository contains following tools for performance analysis DPC++ applications:
1. hotspots. This tool shows hottest PI API calls and devices activities (e.g. kernels execution time).
2. dump_exec_graph. This tool dumps DPC++ execution graph for target application and shows hottest nodes.

## Requirements

+ cmake >= 3.24
+ Compiler with С++ 17 support
+ DPC++ built with XPTI support

## How to build

0. Since XPTI has several issue on master branch, at first you must apply patches and rebuild DPC++ 
1. mkdir build && cd build
2. cmake -DXPTI_INCLUDE=<path_to_xpti_include> -DXPTIFW_DISPATHER=<path_to_libxptifw.so> -DSYCL_INCLUDE=<path_to_sycl_include> ..
> e.g. cmake -DXPTI_INCLUDE=<path_to_llvm>/build/install/include -DXPTIFW_DISPATHER=<path_to_llvm>/build/install/lib/libxptifw.so -DSYCL_INCLUDE=<path_to_llvm>/build/install/include ..
3. cmake --build .

## How to use

You can use runners:
> ./hotspots <target_app> <target_app_args>

> ./dump_exec_graph <target_app> <target_app_args>

or manually set following environment variables:
+ XPTI_TRACE_ENABLE=1
+ XPTI_FRAMEWORK_DISPATCHER=<path_to_libxptifw.so>
+ XPTI_SUBSCRIBERS=<path_to_subscriber>

and launch target application.

> Note: for dump_exec_graph tool you can specify path (set GRAPH_DUMP_PATH environment variable) to directory where execution graph will be saved.

## Validated
+ OS: Ubuntu 18.04
+ DPC++ hash commit: 4043dda356af59d3d88607037955a0728dc0f466
+ Intel UHD Graphics 630, Intel Graphics Compute Runtime version: 20.52.18783
+ NVIDIA GeForce GTX 1650, CUDA version: 11.0
