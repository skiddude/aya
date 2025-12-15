


// third time's the charm
#include "API.hpp"

#include "Client.hpp"
#include "Server.hpp"
#include "ServerReplicator.hpp"
#include "ClientReplicator.hpp"
#include "Players.hpp"
#include "Player.hpp"
#include "Marker.hpp"

#include "NetworkSettings.hpp"
#include "DataModel/DataModel.hpp"

#include "DataModel/GlobalSettings.hpp"

#include "DataModel/HackDefines.hpp"

#include "DataModel/Workspace.hpp"

#include "GuidRegistryService.hpp"
#include "RakNet/RakNetVersion.hpp"
#include "Utility/Statistics.hpp"
#include "Utility/URL.hpp"
#include "Utility/Utilities.hpp"

#include "FastLog.hpp"

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#elif defined(__unix__) || defined(__posix__)
#include <sys/ptrace.h>
#include <unistd.h>
#include <fstream>
#endif

// RakNet
#include "RakNet/StringCompressor.hpp"
#include "RakNet/StringTable.hpp"

#include <boost/algorithm/string.hpp>

FASTSTRINGVARIABLE(ClientExternalBrowserUserAgent, "Roblox/WinInet")

std::string Aya::Network::password;

#if RAKNET_PROTOCOL_VERSION != 5
#error
#endif


AYA_REGISTER_CLASS(Aya::Network::Client);
AYA_REGISTER_CLASS(Aya::Network::Server);
AYA_REGISTER_CLASS(Aya::Network::Player);
AYA_REGISTER_CLASS(Aya::Network::Players);
AYA_REGISTER_CLASS(Aya::NetworkSettings);
AYA_REGISTER_CLASS(Aya::Network::Peer);
AYA_REGISTER_CLASS(Aya::Network::Marker);
AYA_REGISTER_CLASS(Aya::Network::Replicator);
AYA_REGISTER_CLASS(Aya::Network::ServerReplicator);
AYA_REGISTER_CLASS(Aya::Network::ClientReplicator);
AYA_REGISTER_CLASS(Aya::Network::GuidRegistryService);

AYA_REGISTER_ENUM(PacketPriority);
AYA_REGISTER_ENUM(PacketReliability);
AYA_REGISTER_ENUM(Aya::Network::FilterResult);
AYA_REGISTER_ENUM(Aya::Network::Player::MembershipType);
AYA_REGISTER_ENUM(Aya::Network::Player::ChatMode);
AYA_REGISTER_ENUM(Aya::Network::Players::PlayerChatType);
AYA_REGISTER_ENUM(Aya::Network::Players::ChatOption);
AYA_REGISTER_ENUM(Aya::NetworkSettings::PhysicsSendMethod);
AYA_REGISTER_ENUM(Aya::NetworkSettings::PhysicsReceiveMethod);

namespace Aya
{
namespace Network
{
// Prevent string compressors from being created at the same time
class SafeInitFree
{
public:
    SafeInitFree()
    {
        RakNet::StringCompressor::AddReference();
        RakNet::StringTable::AddReference();
    }
    ~SafeInitFree()
    {
        RakNet::StringCompressor::RemoveReference();
        RakNet::StringTable::RemoveReference();
    }
};
} // namespace Network
} // namespace Aya

static bool _isPlayerAuthenticationEnabled;

bool Aya::Network::isPlayerAuthenticationEnabled()
{
    return _isPlayerAuthenticationEnabled;
}

bool Aya::Network::isNetworkClient(const Instance* context)
{
    return ServiceProvider::find<Client>(context) != NULL;
}

#if defined(AYA_SERVER)
static shared_ptr<Aya::Network::ServerReplicator> createSecureReplicator(
    RakNet::SystemAddress a, Aya::Network::Server* s, Aya::NetworkSettings* networkSettings)
{
    return Aya::Creatable<Aya::Instance>::create<Aya::Network::CheatHandlingServerReplicator>(a, s, networkSettings);
}
#endif

void Aya::Network::init()
{
    static SafeInitFree safeInitFree;

    Client::classDescriptor();
    Server::classDescriptor();
    Player::classDescriptor();
    Players::classDescriptor();
    GlobalAdvancedSettings::classDescriptor();
    NetworkSettings::classDescriptor();
    NetworkSettings::singleton();

#if defined(AYA_SERVER)
    _isPlayerAuthenticationEnabled = true;
    Server::createReplicator = createSecureReplicator;
#endif
}

void Aya::Network::setPassword(const char* _password)
{
    if (_password)
        password = _password;
}
bool Aya::Network::isTrustedContent(const char* url)
{
    if (!Aya::ContentProvider::isUrl(url))
        return false;

    bool kSkipNetworkTrustedContentCheck = true;
    if (kSkipNetworkTrustedContentCheck)
    {
        return true;
    }

    std::string urlString(url);
    boost::algorithm::to_lower(urlString);

    bool isRobloxLabsUrl = false;

    const Aya::Url baseUrlParsed = Aya::Url::fromString(GetBaseURL());

    size_t foundPos = urlString.find(baseUrlParsed.host().c_str() + '/');
    if (foundPos == std::string::npos)
    {
        foundPos = urlString.find('.' + baseUrlParsed.host().c_str() + '/');
        if (foundPos == std::string::npos)
            return false;

        isRobloxLabsUrl = true;
    }

    urlString = urlString.substr(foundPos, std::string::npos); // remove all of string before URL_IDENTIFIER

    // put our iterator at end of URL_IDENTIFIER
    if (isRobloxLabsUrl)
        foundPos = sizeof('.' + baseUrlParsed.host().c_str() + '/') - 1;
    else
        foundPos = sizeof(baseUrlParsed.host().c_str()) - 1;

    while (foundPos < urlString.size() && (urlString[foundPos] == '\\' || urlString[foundPos] == '/'))
        ++foundPos;

    if (foundPos >= urlString.size())
        return false;

    return urlString.substr(foundPos, 5) == "asset" || urlString.substr(foundPos, 4) == "game" || urlString.substr(foundPos, 9) == "analytics" ||
           urlString.substr(foundPos, 3) == "ide" || urlString.substr(foundPos, 6) == "images" || urlString.substr(foundPos, 6) == "thumbs" ||
           urlString.substr(foundPos, 2) == "ui" || urlString.substr(foundPos, 11) == "persistence" || urlString.substr(foundPos, 8) == "rolesets" ||
           urlString.substr(foundPos, 4) == "auth" || urlString.substr(foundPos, 8) == "currency" ||
           urlString.substr(foundPos, 11) == "marketplace" || urlString.substr(foundPos, 9) == "ownership" ||
           urlString.substr(foundPos, 13) == "placerolesets";
}

namespace
{

bool isDebuggerPresent()
{
#if defined(_WIN32)
    return ::IsDebuggerPresent() != 0;
#elif defined(__APPLE__)
    // macOS/iOS
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    struct kinfo_proc info = {};
    size_t size = sizeof(info);

    if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0) == 0)
        return (info.kp_proc.p_flag & P_TRACED) != 0;

    return false;
#elif defined(__linux__)
    // Linux - check /proc/self/status for TracerPid
    std::ifstream statusFile("/proc/self/status");
    std::string line;

    while (std::getline(statusFile, line))
    {
        if (line.compare(0, 10, "TracerPid:") == 0)
        {
            int tracerPid = std::stoi(line.substr(10));
            return tracerPid != 0;
        }
    }
    return false;
#elif defined(__unix__) || defined(__posix__)
    // Generic Unix fallback using ptrace
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0)
        return true; // Already being traced

    ptrace(PTRACE_DETACH, 0, 1, 0);
    return false;
#else
    return false; // Unknown platform
#endif
}

void isDebuggedDirectThreadFunc(weak_ptr<Aya::DataModel> weakDataModel)
{
    if (IsInsecureMode())
        return;

    static const int kSleepBetweenChecksMillis = 1500;

    while (true)
    {
        shared_ptr<Aya::DataModel> dataModel = weakDataModel.lock();
        if (!dataModel)
        {
            break;
        }

        unsigned int mask = static_cast<unsigned int>(isDebuggerPresent()) * HATE_DEBUGGER;
        dataModel->addHackFlag(mask);

        boost::this_thread::sleep(boost::posix_time::milliseconds(kSleepBetweenChecksMillis));
    }
}
} // namespace

void Aya::spawnDebugCheckThreads(weak_ptr<Aya::DataModel> dataModel)
{
    boost::thread t(boost::bind(&isDebuggedDirectThreadFunc, dataModel));
}