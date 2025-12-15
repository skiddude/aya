// !!!WARNING!!!
// Do NOT call back into FASTLOG here, since FASTLOG heavily depends on this,
// and we could end up getting a SIGABRT from a lock being set multiple times.
#include "Utility/FileSystem.hpp"

#include <iostream>

#include <boost/filesystem.hpp>

#include "AyaFormat.hpp"
#include "FastLog.hpp"
#include "Debug.hpp"

namespace Aya
{
namespace JNI
{
std::string fileSystemFilesDir; // Set in JNIRobloxSettings.cpp.
} // namespace JNI
} // namespace Aya

using namespace Aya;
using namespace Aya::JNI;

namespace Aya
{
namespace FileSystem
{

boost::filesystem::path getUserDirectory(bool create, FileSystemDir dir, const char* subDirectory)
{
    boost::filesystem::path path;

    if (subDirectory)
    {
        path /= subDirectory;
    }

    switch (dir)
    {
    case DirAppData:
        path /= "appData";
        break;
    case DirExe:
        path /= "exe";
        break;
    case DirPicture:
        path /= "picture";
        break;
    case DirVideo:
        path /= "video";
        break;
    }
    return getCacheDirectory(true, path.native().c_str());
}

} // namespace FileSystem
} // namespace Aya
