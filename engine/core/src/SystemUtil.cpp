#include "SystemUtil.hpp"
#include "StreamHelpers.hpp"
#include "FastLog.hpp"
#include "AyaFormat.hpp"
#ifdef __ANDROID_API__
#include <android/api-level.h>
#endif

#include <fstream>
#include <string>

#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <glad/gl.h>

#ifdef _WIN32
#include <Windows.h>
#include <setupapi.h>

#include <initguid.h>
#include <d3d9.h>
#include "ddraw.h"
#include <d3d11.h>
#include <dxgi1_4.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

using namespace Aya;

namespace Aya
{
namespace SystemUtil
{
std::string mOSVersion; // Set in JNIGLActivity.cpp
std::string mDeviceName;

std::string getCPUMake()
{
#ifdef __arm_
    return "ARM";
#elif __aarch64__
    return "AArch64";
#elif __i386__ || __amd64__
    return "Intel";
#else
#error Unsupported platform.
#endif
}

uint64_t getCPUSpeed()
{
    return 0;
}

uint64_t getCPULogicalCount()
{
    return getCPUCoreCount();
}

uint64_t getCPUCoreCount()
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_CONF);
#endif
}

uint64_t getCPUPhysicalCount()
{
    return getCPUCoreCount();
}

bool isCPU64Bit()
{
#ifdef __arm64__ || __amd64__ || _WIN64
    return true;
#else
#ifdef _WIN32
    BOOL result = false;
    return IsWow64Process(GetCurrentProcess(), &result) && result;
#else
    return false;
#endif
#endif
}

uint64_t getMBSysRAM()
{
    return 16 * 1024;
}

uint64_t getMBSysAvailableRAM()
{
    return getMBSysRAM();
}

uint64_t getVideoMemory()
{
    GLint total_mem_kb = 0;
    // return 8gb
    return 8 * 1024 * 1024;
}

std::string osPlatform()
{
#ifdef __ANDROID_API__
    return "Android";
#elif _WIN32
    return "Win32";
#elif __APPLE__
    return "Apple";
#else
    return "Linux";
#endif
}

int osPlatformId()
{
#ifdef __ANDROID_API__
    return __ANDROID_API__;
#elif _WIN32
    return VER_PLATFORM_WIN32_NT;
#else
    return 0x7999999999;
#endif
}

std::string osVer()
{
#ifdef _WIN32
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#pragma warning(disable : 4996 28159)
    GetVersionEx(&osvi);
    return Aya::format("%d.%d.%d.%d", osvi.dwOSVersionInfoSize, osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
#else
    return mOSVersion;
#endif
}

std::string deviceName()
{
#ifndef __ANDROID_API__
    return "PC";
#else
    return Aya::SystemUtil::mDeviceName;
#endif
}

std::string getGPUMake()
{
#ifdef __ANDROID_API__
    return "Android";
#elif _WIN32

    DISPLAY_DEVICE displayDevice;
    ZeroMemory(&displayDevice, sizeof(displayDevice));
    displayDevice.cb = sizeof(displayDevice);
    EnumDisplayDevices(NULL, 0, &displayDevice, 0);

    return std::string(displayDevice.DeviceString);
#else
    return "Some Linux GPU";
#endif
}

std::string getMaxRes()
{
    return "";
}

std::string getExecutablePath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().string();
#else
    char buffer[PATH_MAX] = {0};
    readlink("/proc/self/exe", buffer, sizeof(buffer));
    return std::filesystem::path(buffer).parent_path();
#endif
}
} // namespace SystemUtil
} // namespace Aya