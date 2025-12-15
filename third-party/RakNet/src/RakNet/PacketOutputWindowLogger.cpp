#include "NativeFeatureIncludes.hpp"
#if _RAKNET_SUPPORT_PacketLogger == 1

#if defined(UNICODE)
#include "RakWString.hpp"
#endif

#include "PacketOutputWindowLogger.hpp"
#include "RakString.hpp"
#if defined(_WIN32) && !defined(X360__)
#include "WindowsIncludes.hpp"
#endif

using namespace RakNet;

PacketOutputWindowLogger::PacketOutputWindowLogger() {}
PacketOutputWindowLogger::~PacketOutputWindowLogger() {}
void PacketOutputWindowLogger::WriteLog(const char *str) {}

#endif // _RAKNET_SUPPORT_*
