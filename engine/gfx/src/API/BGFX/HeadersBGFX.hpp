#pragma once

// BGFX headers
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

// Common macros
#ifndef AYAASSERT
#define AYAASSERT(x) assert(x)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif
