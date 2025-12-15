#include "server.hpp"

#include <boost/bind/bind.hpp>
#include <mutex>
#include "grid.hpp"
#include "Utility/StandardOut.hpp"

#ifdef _WIN32
// static boost::scoped_ptr<MainLogManager> mainLogManager(new MainLogManager("Aya Server", ".dmp", ".crashevent"));
#endif
static boost::scoped_ptr<StandardOutLogger> standardOutLog;

namespace Aya
{
class Explosion;
class Cofm;
class NormalBreakConnector;
class ContactConnector;
class RevoluteLink;
class SimBody;
class BallBallContact;
class BallBlockContact;
class BlockBlockContact;
} // namespace Aya

class StandardOutLogger
{
    Aya::signals::scoped_connection messageConnection;
    std::mutex mutex;

public:
    StandardOutLogger()
    {
        messageConnection =
            Aya::StandardOut::singleton()->messageOut.connect(boost::bind(&StandardOutLogger::onMessage, this, boost::placeholders::_1));
    }

protected:
    void onMessage(const Aya::StandardOutMessage& message)
    {
        std::lock_guard<std::mutex> lock(mutex);

        // ANSI escape codes for color output
        const char* colorCode = "\033[0m";
        switch (message.type)
        {
        case Aya::MESSAGE_OUTPUT:
            colorCode = "\033[34m";
            break;
        case Aya::MESSAGE_INFO:
            colorCode = "\033[37m";
            break;
        case Aya::MESSAGE_WARNING:
            colorCode = "\033[33m";
            break;
        case Aya::MESSAGE_ERROR:
            colorCode = "\033[31m";
            break;
        default:
            break;
        }

        std::cout << colorCode << message.message << "\033[0m" << std::endl;
    }
};

void start_aya_server(bool ui, bool isLAN, int gridPort, int localServerPort, const std::string& localServerPlace,
    const std::string& localServerPassword, const std::string& masterServerHost, const std::string& masterServerName,
    const std::string& masterServerDescription)
{
    if (gridPort != -1)
    {
        start_grid_server(gridPort);
    }
}