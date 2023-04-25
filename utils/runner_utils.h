#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#endif

#include <iostream>
#include <string>
#include <vector>

inline std::string getPathToRunner() {
    std::string runnerPath;
#ifdef __linux__
    char buffer[1024] = { 0 };
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        throw std::runtime_error("readlink failed!");
    } else {
        buffer[len] = '\0';
        runnerPath = std::string(buffer);
    }
#else
    throw std::runtime_error("Can't get path to runner on this OS!");
#endif
    return runnerPath;
}
