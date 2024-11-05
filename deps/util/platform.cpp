#include <chrono>
#include <util/platform.h>

uint64_t os_gettime_ns()
{
    auto d = std::chrono::system_clock::now().time_since_epoch();

    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}