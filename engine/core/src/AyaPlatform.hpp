// this file meant to include basic OS .h dependencies.
// obviously, it only supports windows at this time.
#pragma once

#ifdef WIN32
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#ifndef STRICT
#define STRICT 1
#endif


#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <Windows.h>
#undef NOMINMAX

#undef _G3D_INTERNAL_HIDE_WINSOCK_
#undef _WINSOCKAPI_

#include <intrin.h>

#endif

#include "AyaBase.hpp"