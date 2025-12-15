
#include "Utility/FileSystem.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <ATLPath.h>

#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif // _WIN32

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "AyaFormat.hpp"
#include "Utility/StandardOut.hpp"
#include "atomic.hpp"

#include "Utility/Guid.hpp"

DYNAMIC_FASTFLAGVARIABLE(LogFileSystem, false)
DYNAMIC_FASTFLAGVARIABLE(FileSystemGetCacheDirectoryLikeAndroid, false)

using namespace Aya;

#ifdef __ANDROID__
namespace Aya
{
namespace JNI
{
std::string fileSystemCacheDir; // Set in JNIRobloxSettings.cpp
} // namespace JNI
} // namespace Aya
#endif // #ifdef __ANDROID__

namespace
{
void logFileError(const void* caller, const char* operation, bool isError, bool doThrow = true)
{
    if (isError)
    {
#ifdef _WIN32
        DWORD errnum = GetLastError();
#else
        int errnum = errno;
#endif
        boost::system::error_code ec(errnum, boost::system::system_category());

        if (doThrow)
        {
            throw Aya::runtime_error("%s(%p): %s", operation, caller, ec.message().c_str());
        }
        else
        {
            if (DFFlag::LogFileSystem)
            {
                StandardOut::singleton()->printf(MESSAGE_ERROR, "%s(%p): %s", operation, caller, ec.message().c_str());
            }
        }
    }
}


boost::filesystem::path getBaseCacheDirectory(bool create)
{
#ifdef __ANDROID__
    AYAASSERT(!JNI::fileSystemCacheDir.empty());
    return JNI::fileSystemCacheDir;
#else
    static Aya::atomic<int> caching;
    static boost::filesystem::path cachedResult;

    if (!cachedResult.empty())
    {
        return cachedResult;
    }

    boost::filesystem::path path = boost::filesystem::temp_directory_path();

#ifndef AYA_PLATFORM_IOS
    path /= AYA_PROJECT_NAME;
#endif

#if defined(_DEBUG) || defined(_NOOPT)
    // This appears in the logservice.  This means lua scripts can get access to the
    // user's OS User Name.  Because this is often the user's first name, it will
    // be concerning to users who will think they are being hacked.
    StandardOut::singleton()->printf(MESSAGE_INFO, "FileSystem temp directory: %s", path.string().c_str());
#endif

    if (!boost::filesystem::exists(path))
    {
        if (create)
        {
            boost::system::error_code ec;
            boost::filesystem::create_directories(path, ec);
        }
        else
        {
            return "";
        }
    }

    if (caching.compare_and_swap(1, 0) == 0)
    {
        cachedResult = path;
    }

    return path;
#endif // #ifdef __ANDROID__
}
} // namespace

namespace Aya
{

boost::filesystem::path FileSystem::getCacheDirectory(bool create, const char* subDirectory)
{
    boost::filesystem::path path = getBaseCacheDirectory(create);
    if (!subDirectory)
    {
        return path;
    }

#ifndef __ANDROID__
    if (!DFFlag::FileSystemGetCacheDirectoryLikeAndroid)
    {
        path /= subDirectory;

        if (!boost::filesystem::exists(path))
        {
            if (create)
            {
                boost::system::error_code ec;
                if (!boost::filesystem::create_directories(path, ec) && ec.value())
                {
                    StandardOut::singleton()->printf(MESSAGE_ERROR, "FileSystem unable to create %s, %s", path.c_str(), ec.message().c_str());
                    throw Aya::runtime_error("FileSystem unable to create %s, %s", path.c_str(), ec.message().c_str());
                }
            }
            else
            {
                return "";
            }
        }

        if (!boost::filesystem::exists(path))
        {
            return "";
        }
    }
    else
    {
#endif // #ifndef __ANDROID__
        path /= subDirectory;

        if (create && !boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(path);
        }

        if (!boost::filesystem::exists(path))
        {
            throw Aya::runtime_error("Path does not exist: %s", path.c_str());
        }

        if (!boost::filesystem::is_directory(path))
        {
            throw Aya::runtime_error("Path is not a directory: %s", path.c_str());
        }
#ifndef __ANDROID__
    } // if (!DFFlag::FileSystemGetCacheDirectoryLikeAndroid)
#endif // #ifndef __ANDROID__

    return path;
}

boost::filesystem::path FileSystem::getTempFilePath()
{
    std::string guid;
    Guid::generateRBXGUID(guid);
    return getCacheDirectory(true, "fs_tmp") / guid;
}

void FileSystem::clearCacheDirectory(const char* subDirectory)
{
    boost::filesystem::path cachePath = getCacheDirectory(false, subDirectory);
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    if (!cachePath.empty() && fs::exists(cachePath, ec) && !ec)
    {
        fs::directory_iterator end_iter;
        for (fs::directory_iterator iter(cachePath); end_iter != iter; ++iter)
        {
            if (!fs::is_directory(iter->status()))
            {
                boost::system::error_code ec;
                boost::filesystem::remove(*iter, ec);
            }
        }
    }
}

} // namespace Aya
