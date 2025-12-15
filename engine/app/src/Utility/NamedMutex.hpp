#pragma once

#ifdef _WIN32
#include <Windows.h>

namespace Aya
{
class ScopedNamedMutex
{
    HANDLE hMutex;

public:
    ScopedNamedMutex(const char* name);
    ~ScopedNamedMutex();
};
} // namespace Aya
#endif // #ifdef _WIN32