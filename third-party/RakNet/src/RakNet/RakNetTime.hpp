#ifndef __RAKNET_TIME_H
#define __RAKNET_TIME_H

#include "NativeTypes.hpp"
#include "RakNetDefines.hpp"
#include <cstdint>

// Thanks - justin5262
// "USE uint64 because LONG LONG isn't actually 64 bit on linux"

namespace RakNet {

// Aya CHANGE: ALWAYS 64-bit
typedef uint64_t Time;
typedef unsigned int TimeMS;
typedef uint64_t TimeUS;
// END Aya CHANGE

}; // namespace RakNet

#endif
