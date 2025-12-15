// d9mz - might be something wrong with my sdk, but this is needed to compile shadercompiler - it sucks
#ifndef PTRDIFF_MAX
#include "StringConv.hpp"
#else
#include "StringConv.hpp"
#endif

#if !defined(__linux) && !defined(__APPLE__)
#include <Windows.h>
#endif

namespace Aya
{

#if !defined(__linux) && !defined(__APPLE__)
// convert wstring to UTF-8 string
std::string utf8_encode(const std::wstring& path)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), NULL, 0, NULL, NULL);
    std::string tgt(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), &tgt[0], size_needed, NULL, NULL);
    return tgt;
}

// convert UTF-8 string to wstring
std::wstring utf8_decode(const std::string& path)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &path[0], (int)path.size(), NULL, 0);
    std::wstring tgt(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &path[0], (int)path.size(), &tgt[0], size_needed);
    return tgt;
}
#else
std::string utf8_encode(const std::string& path)
{
    return path;
}
std::string utf8_decode(const std::string& path)
{
    return path;
}
#endif

} // namespace Aya