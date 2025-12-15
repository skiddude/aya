

#pragma once

// Qt Headers
#include <QObject>
#include <QMetaType>

// Roblox Headers
#include "DataModel/TeleportCallback.hpp"

#include "DataModel/FastLogSettings.hpp"


namespace Aya
{
class Game;
class FunctionMarshaller;
struct StandardOutMessage;
} // namespace Aya

class QTimer;

Q_DECLARE_METATYPE(Aya::MessageType);

class Roblox
    : public QObject
    , Aya::TeleportCallback
{
    Q_OBJECT

Q_SIGNALS:

    /**
     * Emitted when a log entry is made from the engine that should be displayed to the user.
     *
     * @param   message     note that this is a copy, not a reference to handle multi-threading
     * @param   type        type of log message
     */
    void newOutputMessage(const QString message, Aya::MessageType type);

    void marshallAppEvent(void* event, bool wait);

public:
    static Roblox& Instance();

    static void globalInit(const QString& urlArg, const QString& ticketArg);
    static void globalShutdown();

    static void sendAppEvent(void* pClosure);
    static void postAppEvent(void* pClosure);
    virtual bool isTeleportEnabled() const;

    void doTeleport(const std::string& url, const std::string& ticket, const std::string& script);

    void Teleport(const std::string& url, const std::string& ticket, const std::string& script);

    void startTimer();
    void stopTimer();

private Q_SLOTS:
    void onTimeOut();

private:
    Roblox(); // Private constructor
    ~Roblox();
    Roblox(const Roblox&);            // Prevent copy-construction
    Roblox& operator=(const Roblox&); // Prevent assignment

    static void onMessageOut(const Aya::StandardOutMessage& message);

    Aya::FunctionMarshaller* m_pMarshaller;
    QTimer* m_pTimer;

    static bool sInitialized;
};
