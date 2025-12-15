#include "CPUCount.hpp"

#include <thread>

unsigned int RbxTotalUsableCoreCount(unsigned int defaultValue)
{
    unsigned n = std::thread::hardware_concurrency();
    return n ? n : defaultValue;
}
