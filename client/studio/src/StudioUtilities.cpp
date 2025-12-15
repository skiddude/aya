


#include "StudioUtilities.hpp"

// Qt Headers
#include <QNetworkInterface>
#include <QFile>
#include <QFileInfo>
#include <QDir>

// 3rd Party Headers
#include "boost/iostreams/copy.hpp"

// Roblox Headers
#include "API.hpp"

#include "Script/script.hpp"
#include "Script/ScriptContext.hpp"
#include "Utility/FileSystem.hpp"

#include "Utility/Hash.hpp"

#include "Utility/RunStateOwner.hpp"

#include "Utility/Statistics.hpp"

#include "DataModel/Camera.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/ChangeHistory.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/PartInstance.hpp"

#include "DataModel/Tool.hpp"

#include "DataModel/Workspace.hpp"

#include "World/World.hpp"
#include "DataModel/Visit.hpp"

#include "Xml/Serializer.hpp"
#include "Xml/XmlSerializer.hpp"
#include "Reflection/Type.hpp"
#include "Client.hpp"
#include "ClientReplicator.hpp"
#include "Players.hpp"

#include "Marker.hpp"

// Roblox Studio Headers
#include "QtUtilities.hpp"
#include "RobloxMainWindow.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxUser.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxGameExplorer.hpp"
#include "RobloxSettings.hpp"

DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

namespace StudioUtilities
{
static bool sIsVideoUploading = false;
static std::string sVideoFileName("");
static bool IsAvatarMode = false;
static bool IsTestMode = false;
static bool IsFirstTimeOpeningStudio = false;

bool isFirstTimeOpeningStudio()
{
    return IsFirstTimeOpeningStudio;
}

void setIsFirstTimeOpeningStudio(bool value)
{
    IsFirstTimeOpeningStudio = value;
}

bool isConnectedToNetwork()
{
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    bool result = false;

    for (int i = 0; i < ifaces.count(); i++)
    {
        QNetworkInterface iface = ifaces.at(i);
        if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {

#ifdef _DEBUG
            // details of connection
            qDebug() << "name:" << iface.name() << Qt::endl << "ip addresses:" << Qt::endl << "mac:" << iface.hardwareAddress() << Qt::endl;
#endif

            // this loop is important
            for (int j = 0; j < iface.addressEntries().count(); j++)
            {
#ifdef _DEBUG
                qDebug() << iface.addressEntries().at(j).ip().toString() << " / " << iface.addressEntries().at(j).netmask().toString() << Qt::endl;
#endif

                // we have an interface that is up, and has an ip address
                // therefore the link is present

                // we will only enable this check on first positive,
                // all later results are incorrect
                if (result == false)
                    result = true;
            }
        }
    }
    return result;
}

bool isAvatarMode()
{
    return IsAvatarMode;
}

void setAvatarMode(bool isAvatarMode)
{
    IsAvatarMode = isAvatarMode;
}

bool isTestMode()
{
    return IsTestMode;
}

void setTestMode(bool isTestMode)
{
    IsTestMode = isTestMode;
}

bool containsEditScript(const QString& url)
{
    return url.contains("edit.ashx", Qt::CaseInsensitive);
}

bool containsJoinScript(const QString& url)
{
    return url.contains("join.ashx", Qt::CaseInsensitive);
}

bool containsVisitScript(const QString& url)
{
    return url.contains("visit.ashx", Qt::CaseInsensitive);
}

bool containsGameServerScript(const QString& url)
{
    return url.contains("gameserver.ashx", Qt::CaseInsensitive);
}

std::string authenticate(std::string& domain, std::string& url, std::string& ticket)
{
    if (domain.empty() || url.empty() || ticket.empty())
        return "";

    try
    {
        // Post our ticket back to Roblox to suggest a re-authentication
        std::string result;
        {
            std::string compound = url;
            compound += "?suggest=";
            compound += ticket;

            Aya::Http http(compound.c_str());
#ifdef Q_WS_MAC
            http.setAuthDomain(::GetBaseURL());
#else
            http.additionalHeaders["RBXAuthenticationNegotiation:"] = ::GetBaseURL();
#endif

            // http.additionalHeaders["RBXAuthenticationNegotiation:"] = domain;
            http.get(result);
        }

        // The http content is the new ticket
        return result;
    }
    catch (std::exception& e)
    {
        // Report Error!
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
        return "";
    }
}

Aya::ProtectedString fetchAndValidateScript(Aya::DataModel* pDataModel, const std::string& urlScript)
{
    if (!pDataModel)
        return Aya::ProtectedString();

    Aya::Security::Impersonator impersonate(Aya::Security::COM);

    std::ostringstream data;
    if (!Aya::ContentProvider::isUrl(urlScript))
        return Aya::ProtectedString();

    std::string dataString;
    {
        try
        {
            Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);
            std::auto_ptr<std::istream> stream(
                Aya::ServiceProvider::create<Aya::ContentProvider>(pDataModel)->getContent(Aya::ContentId(urlScript.c_str())));
            dataString = std::string(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
        }

        catch (std::exception& e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "ContentProvider while executing url script failed because %s", e.what());
        }
    }

    Aya::ProtectedString verifiedSource;

    try
    {
        verifiedSource = Aya::ProtectedString::fromTrustedSource(dataString);
        Aya::ContentProvider::verifyScriptSignature(verifiedSource, true);
    }
    catch (std::bad_alloc&)
    {
        throw;
    }
    catch (std::exception& e)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Script signature verification failed -- %s", e.what());
        return Aya::ProtectedString();
    }

    return verifiedSource;
}

void executeURLJoinScript(boost::shared_ptr<Aya::Game> pGame, std::string urlScript)
{
    AYAASSERT(containsJoinScript(QString::fromStdString(urlScript)));

    boost::shared_ptr<Aya::DataModel> pDataModel = pGame->getDataModel();
    if (!pDataModel)
        return;

    Aya::ProtectedString verifiedSource = fetchAndValidateScript(pDataModel.get(), urlScript);
    if (verifiedSource.empty())
        return;

    try
    {
        Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);
        if (pDataModel->isClosed())
            return;

        std::string dataString = verifiedSource.getSource();
        int firstNewLineIndex = dataString.find("\r\n");
        if (dataString[firstNewLineIndex + 2] == '{')
        {
            pGame->configurePlayer(Aya::Security::COM, dataString.substr(firstNewLineIndex + 2));
            if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
            {
                QMetaObject::invokeMethod(ideDoc, "refreshDisplayName", Qt::QueuedConnection);
            }

            return;
        }

        boost::shared_ptr<Aya::ScriptContext> pContext = Aya::shared_from(pDataModel->create<Aya::ScriptContext>());
        if (pContext)
            pContext->executeInNewThread(Aya::Security::COM, verifiedSource, "Start Game");
    }
    catch (std::exception& exp)
    {
        std::string s(verifiedSource.getSource());
        std::string msg = Aya::format("executeURLJoinScript() failed because %s.  %s", exp.what(), s.substr(0, 64).c_str());
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, msg.c_str());
    }
}

void executeURLScript(boost::shared_ptr<Aya::DataModel> pDataModel, std::string urlScript)
{
    if (!pDataModel)
        return;

    Aya::ProtectedString verifiedSource = fetchAndValidateScript(pDataModel.get(), urlScript);
    if (verifiedSource.empty())
        return;

    try
    {
        Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);
        if (pDataModel->isClosed())
            return;

        boost::shared_ptr<Aya::ScriptContext> pContext = Aya::shared_from(pDataModel->create<Aya::ScriptContext>());
        if (pContext)
            pContext->executeInNewThread(Aya::Security::COM, verifiedSource, "Start Game");
    }
    catch (std::exception& exp)
    {
        std::string s(verifiedSource.getSource());
        std::string msg = Aya::format("executeURLScript() failed because %s.  %s", exp.what(), s.substr(0, 64).c_str());
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, msg.c_str());
    }
}

void convertPhysicalPropertiesIfNeeded(std::vector<boost::shared_ptr<Aya::Instance>> instances, Aya::Workspace* workspace)
{
    if (workspace->getUsingNewPhysicalProperties())
    {
        for (size_t i = 0; i < instances.size(); ++i)
        {
            Aya::PartInstance::convertToNewPhysicalPropRecursive(instances[i].get());
        }
    }
    else
    {
        for (size_t i = 0; i < instances.size(); i++)
        {
            if (Aya::PartInstance::instanceOrChildrenContainsNewCustomPhysics(instances[i].get()))
            {
                if (RobloxIDEDoc::displayAskConvertPlaceToNewMaterialsIfInsertNewModel())
                {
                    workspace->setPhysicalPropertiesMode(Aya::PhysicalPropertiesMode_NewPartProperties);
                    Aya::PartInstance::convertToNewPhysicalPropRecursive(Aya::DataModel::get(workspace));
                }
                break;
            }
        }
    }
}

void insertModel(boost::shared_ptr<Aya::DataModel> pDataModel, QString fileName, bool insertInto)
{
    if (fileName.isEmpty())
        return;

    if (!pDataModel)
        throw std::runtime_error("Can't insert at this time");

    QByteArray ba = fileName.toUtf8();
    const char* c_str = ba.constData();

    std::ifstream stream(c_str, std::ios_base::in | std::ios_base::binary);

    // The following code happens BEFORE we do a lock because we can't display a message box within a lock
    Aya::Instances instances;
    Serializer().loadInstances(stream, instances);

    {
        // Lock the data model temporarily to make sure insertion is legal
        Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);

        if (DFFlag::MaterialPropertiesEnabled)
        {
            convertPhysicalPropertiesIfNeeded(instances, pDataModel->getWorkspace());
        }

        if (Aya::ContentProvider* cp = pDataModel->create<Aya::ContentProvider>())
        {
            Aya::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, instances);
        }
    }

    Aya::PromptMode promptMode = Aya::SUPPRESS_PROMPTS;

    if (!insertInto)
    {
        if (instances.size() == 1)
        {
            Aya::Instance* single = instances[0].get();
            if (dynamic_cast<Aya::Tool*>(single))
            {
                if (QtUtilities::RBXMessageBox::question(&UpdateUIManager::Instance().getMainWindow(), "Insert Model",
                        "Put this tool into the starter pack (otherwise drop into the 3d view)?", QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::Yes)
                    promptMode = Aya::PUT_TOOL_IN_STARTERPACK;
            }
        }
    }

    // Now lock the datamodel (after MessageBox)
    Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);

    shared_ptr<Aya::Workspace> spWorkspace = shared_from(pDataModel ? pDataModel->getWorkspace() : NULL);
    if (!spWorkspace)
        throw std::runtime_error("Can't insert at this time");

    Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(spWorkspace.get());

    Aya::Instance* requestedParent = (insertInto && (sel->size() == 1)) ? sel->front().get() : spWorkspace.get();

    Aya::InsertMode insertMode = insertInto ? Aya::INSERT_TO_TREE : Aya::INSERT_TO_3D_VIEW;

    try
    {
        spWorkspace->insertInstances(instances, requestedParent, insertMode, promptMode);

        boost::shared_ptr<Aya::RunService> runService = shared_from(pDataModel->create<Aya::RunService>());
        // If we're not playing a game then select the content
        if (runService->getRunState() == Aya::RS_STOPPED)
        {
            sel->setSelection(instances.begin(), instances.end());
        }
    }
    catch (std::exception&)
    {
        std::for_each(instances.begin(), instances.end(), boost::bind(&Aya::Instance::setParent, _1, (Aya::Instance*)NULL));
        throw;
    }
}

void insertScript(boost::shared_ptr<Aya::DataModel> pDataModel, const QString& fileName)
{
    shared_ptr<Aya::Workspace> spWorkspace = shared_from(pDataModel ? pDataModel->getWorkspace() : NULL);
    if (!pDataModel || !spWorkspace)
        throw std::runtime_error("Script cannot be inserted at this time");

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        throw std::runtime_error("Script file cannot be opened");

    Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);

    shared_ptr<Aya::Script> pScriptInstance = Aya::Creatable<Aya::Instance>::create<Aya::Script>();
    if (!pScriptInstance)
        throw std::runtime_error("Script instance cannot be created");

    // set file name as script instance name
    pScriptInstance->setName(QFileInfo(fileName).completeBaseName().toStdString());

    // read and set code from file into script instance
    QTextStream in(&file);
    pScriptInstance->setEmbeddedCode(Aya::ProtectedString::fromTrustedSource(in.readAll().toUtf8().data()));

    // set appropriate parent
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(spWorkspace.get());
    Aya::Instance* pRequestedParent = (pSelection && pSelection->size() == 1) ? pSelection->front().get() : spWorkspace.get();
    pScriptInstance->setParent(pRequestedParent);

    // select the created script instance
    if (pSelection)
        pSelection->setSelection(pScriptInstance.get());
}

QString getDebugInfoFile(const QString& fileName, const QString& debuggerFileExt)
{
    QString debugInfoFile;
    if (!fileName.isEmpty())
    {
        static QString filePath = QtUtilities::qstringFromStdString(Aya::FileSystem::getUserDirectory(true, Aya::DirAppData, "debugger").native());
        // get hash of the file path
        unsigned int hash = Aya::Hash::hash(QFileInfo(fileName).absoluteFilePath().toStdString());
        // create debug file path with generated hash
        debugInfoFile = filePath;
        debugInfoFile.append(QString::number(hash));
        if (!debuggerFileExt.isEmpty())
            debugInfoFile.append(debuggerFileExt);
        debugInfoFile.append(".rdbg");
    }
    return debugInfoFile;
}

bool checkNetworkAndUserAuthentication(bool openStartPage)
{
    if (!StudioUtilities::isConnectedToNetwork())
    {
        // we don't have network connect
        UpdateUIManager::Instance().showErrorMessage(QObject::tr("Publish to Roblox"),
            QObject::tr("Feature not available in offline mode.\nPlease check internet connection and restart Roblox."));
        return false;
    }

    if (RobloxUser::singleton().getUserId() <= 0)
    {
        UpdateUIManager::Instance().showErrorMessage(QObject::tr("Log in required"), QObject::tr("You must log in to perform this action!"));
        if (openStartPage)
        {
            UpdateUIManager::Instance().getMainWindow().openStartPage(true, "showlogin=True");
            UpdateUIManager::Instance().getMainWindow().setFocus();
        }
        return false;
    }

    return true;
}

int translateKeyModifiers(Qt::KeyboardModifiers state, const QString& text)
{
    // Convert modifier combos to a single integer.
    int result = 0;
    if ((state & Qt::ShiftModifier) && (text.size() == 0 || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}
} // namespace StudioUtilities
