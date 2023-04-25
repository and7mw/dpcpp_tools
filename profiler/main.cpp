#include "../utils/runner_utils.h"

int main(int argc, char **argv, char **env) {
#ifdef __linux__
    if (argc <= 1) {
        std::cout << "Provide path to app for analysis!" << std::endl;
        return 1;
    }

    std::vector<char *> new_env;
    size_t i = 0;
    while (env[i] != nullptr) {
        new_env.push_back(env[i++]);
    }

    std::string selfPath = getPathToRunner();
    const auto pos = selfPath.find_last_of("/");
    selfPath = selfPath.substr(0, pos);

    std::string trace_enable_env = "XPTI_TRACE_ENABLE=1";
    std::string xptifw_env = "XPTI_FRAMEWORK_DISPATCHER=libxptifw.so";
    std::string subscr_env = "XPTI_SUBSCRIBERS=" + selfPath + "/libprofiler_lib.so";
    new_env.push_back(const_cast<char *>(trace_enable_env.data()));
    new_env.push_back(const_cast<char *>(xptifw_env.data()));
    new_env.push_back(const_cast<char *>(subscr_env.data()));
    new_env.push_back(nullptr);

    if (execve(argv[1], argv + 1, new_env.data())) {
        std::cout << "Can't launch target app!" << std::endl;
        return 1;
    }
#else
    throw std::runtime_error("Runner doesn't work for this OS, set"
                             " XPTI_TRACE_ENABLE, XPTI_FRAMEWORK_DISPATCHER, XPTI_SUBSCRIBERS manually!");
#endif
    return 0;
}
