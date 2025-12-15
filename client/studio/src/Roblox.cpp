


#include "Roblox.hpp"

// Qt Headers
#include <QDateTime>
#include <QTimer>
#include <QThread>

// 3rd Party Headers
#include "boost/function.hpp"

// Roblox Headers
#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/MouseCommand.hpp"

#include "DataModel/TeleportService.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/Light.hpp"

#include "DataModel/Lighting.hpp"

#include "DataModel/HttpService.hpp"

#include "TaskScheduler.hpp"


#include "Utility/StandardOut.hpp"

#include "Utility/Statistics.hpp"

#include "NetworkSettings.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "DataModel/Workspace.hpp"

#include "DataModel/CustomParticleEmitter.hpp"

#include "DataModel/ContentProvider.hpp"


// Roblox Studio Headers
#include "RobloxIDEDoc.hpp"
#include "RobloxMainWindow.hpp"
#include "UpdateUIManager.hpp"
#include "FunctionMarshaller.hpp"
#include "RobloxSettings.hpp"
#include "AuthoringSettings.hpp"
#include "ExternalHandlers.hpp"
#include "RobloxDocManager.hpp"
#include "StudioUtilities.hpp"
#include "RobloxUser.hpp"

#include "Profiler.hpp"


FASTFLAG(SurfaceLightEnabled)
DYNAMIC_FASTFLAG(CustomEmitterInstanceEnabled)
FASTFLAG(UsePGSSolver)

FASTFLAGVARIABLE(StudioABTestEnabled, false)


bool Roblox::sInitialized = false;
Aya::signals::scoped_connection messageOutConnection;


#ifdef Q_WS_MAC

extern "C"
{
    void writeFastLogDumpHelper(const char* fileName, int numEntries)
    {
        FLog::WriteFastLogDump(fileName, numEntries);
    }
};
#endif

// this function is being used in RBXGraphics.cpp also!
std::string macBundlePath()
{
#ifdef Q_WS_MAC
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);

    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    return std::string(path);
#endif
    return "";
}

Roblox& Roblox::Instance()
{
    static Roblox singleton;
    return singleton;
}

Roblox::Roblox()
    : m_pMarshaller(Aya::FunctionMarshaller::GetWindow())
    , m_pTimer(new QTimer(this))
{
    qRegisterMetaType<Aya::MessageType>("Aya::MessageType");
    qRegisterMetaType<boost::shared_ptr<Aya::Instance>>("shared_ptr<Aya::Instance>");
    qRegisterMetaType<const Aya::Reflection::PropertyDescriptor*>("const Aya::Reflection::PropertyDescriptor*");

    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
}

Roblox::~Roblox()
{
    Aya::FunctionMarshaller::ReleaseWindow(m_pMarshaller);
}

void Roblox::startTimer()
{
    m_pTimer->start(1000);
}

void Roblox::stopTimer()
{
    m_pTimer->stop();
}

void Roblox::onTimeOut() {}

void Roblox::globalInit(const QString& urlArg, const QString& ticketArg)
{
    if (sInitialized)
        return;


    messageOutConnection = Aya::StandardOut::singleton()->messageOut.connect(&onMessageOut);

    Aya::Http::SetUseStatistics(true);

    RobloxSettings::initWorkspaceSettings();

    Aya::MouseCommand::enableAdvArrowTool(true);

    // Force loading of settings classes
    Aya::GameBasicSettings::singleton();
    AuthoringSettings::singleton();
    Aya::NetworkSettings::singleton();
    CRenderSettingsItem::singleton();
    Aya::DebugSettings::singleton();

    Aya::GlobalAdvancedSettings::singleton()->loadState(std::string());
    {
        Aya::Security::Impersonator impersonate(Aya::Security::RobloxGameScript_);
        Aya::GlobalBasicSettings::singleton()->loadState(std::string());
    }

    Aya::GameBasicSettings::singleton().setStudioMode(true);

    // now that fast flags are loaded we can reconfigure our settings if necessary
    AuthoringSettings::singleton().configureBasedOnFastFlags();

    Aya::Profiler::onThreadCreate("Main");

    // Initialize the TaskScheduler (after loading configs)
    Aya::TaskScheduler::singleton().setThreadCount(Aya::TaskSchedulerSettings::singleton().getThreadPoolConfig());

    Roblox::Instance().startTimer();


    Aya::TeleportService::SetCallback(&Roblox::Instance());

    sInitialized = true;

    if (!FFlag::SurfaceLightEnabled)
    {
        Aya::Reflection::ClassDescriptor* classDesc = &Aya::SurfaceLight::classDescriptor();
        Aya::Reflection::Metadata::Class* pMetadataDynamic = Aya::Reflection::Metadata::Reflection::singleton()->get(*classDesc, false);
        if (pMetadataDynamic)
        {
            Aya::Reflection::Metadata::Class::prop_deprecated.setValue(pMetadataDynamic, true);
        }
    }
    if (!DFFlag::CustomEmitterInstanceEnabled)
    {
        Aya::Reflection::ClassDescriptor* classDesc = &Aya::CustomParticleEmitter::classDescriptor();
        Aya::Reflection::Metadata::Class* pMetadataDynamic = Aya::Reflection::Metadata::Reflection::singleton()->get(*classDesc, false);
        if (pMetadataDynamic)
        {
            Aya::Reflection::Metadata::Class::prop_deprecated.setValue(pMetadataDynamic, true);
        }
    }

    if (!FFlag::UsePGSSolver)
    {
        Aya::Reflection::PropertyDescriptor* propDesc = Aya::Workspace::classDescriptor().findPropertyDescriptor("PGSPhysicsSolverEnabled");
        if (propDesc)
        {
            Aya::Reflection::Metadata::Member* pMetadataDynamic = Aya::Reflection::Metadata::Reflection::singleton()->get(*propDesc);
            if (pMetadataDynamic)
                Aya::Reflection::Metadata::Item::prop_deprecated.setValue(pMetadataDynamic, true);
        }
    }
}

void Roblox::globalShutdown()
{
    RobloxSettings::saveAssets();
    Roblox::Instance().stopTimer();
    Aya::GlobalBasicSettings::singleton()->saveState();
    Aya::Game::globalExit();
    messageOutConnection.disconnect();
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through, wait till the event is processed
void Roblox::sendAppEvent(void* pClosure)
{
    Aya::CEvent* waitEvent = ((Aya::FunctionMarshaller::Closure*)pClosure)->waitEvent;
    bool waitFlag = (waitEvent == NULL);

    Q_EMIT Roblox::Instance().marshallAppEvent(pClosure, waitFlag);

    if (waitEvent)
        waitEvent->Wait();
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through, do not wait
void Roblox::postAppEvent(void* pClosure)
{
    Q_EMIT Roblox::Instance().marshallAppEvent(pClosure, false);
}

bool Roblox::isTeleportEnabled() const
{
    return true;
}

void Roblox::onMessageOut(const Aya::StandardOutMessage& message)
{
    Q_EMIT Roblox::Instance().newOutputMessage(message.message.c_str(), message.type);
}

void Roblox::Teleport(const std::string& url, const std::string& ticket, const std::string& script)
{
    // Teleporting in Studio is disabled.
    RobloxIDEDoc* pPlayDoc = RobloxDocManager::Instance().getPlayDoc();
    if (!pPlayDoc)
        return;

    pPlayDoc->teleportToURL(script.c_str(), true);
}

void Roblox::doTeleport(const std::string& url, const std::string& ticket, const std::string& script)
{
    AYAASSERT(isTeleportEnabled());
    boost::function<void()> callback = boost::bind(&Roblox::Teleport, this, url, ticket, script);
    m_pMarshaller->Submit(callback);
}
