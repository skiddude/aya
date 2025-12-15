#pragma once

#include "Debug.hpp"

// Engine assertions often cause the game to stop running.
// Until we can fix these, turn them off.
// #define AYA_DEBUGENGINE

#ifdef AYA_DEBUGENGINE
#define AYA_ENGINE_ASSERT(expr) AYAASSERT(expr)
#else
#define AYA_ENGINE_ASSERT(expr) ((void)0)
#endif