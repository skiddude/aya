


#include "RbxWorkspace.hpp"

// Qt Headers
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QDesktopServices>
#include <QStringList>
#include <QWebEnginePage>

// Roblox Headers
#include "DataModel/DataModel.hpp"

#include "DataModel/Selection.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/Commands.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/Tool.hpp"

#include "DataModel/Decal.hpp"

#include "DataModel/Visit.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/PartOperationAsset.hpp"

#include "DataModel/PlayerGui.hpp"

#include "DataModel/VehicleSeat.hpp"

#include "DataModel/KeyframeSequence.hpp"

#include "DataModel/KeyframeSequenceProvider.hpp"

#include "DataModel/NonReplicatedCSGDictionaryService.hpp"

#include "Tree/Instance.hpp"

#include "Tree/Service.hpp"

#include "Xml/XmlSerializer.hpp"
#include "Xml/Serializer.hpp"
#include "Xml/SerializerBinary.hpp"
#include "Xml/WebParser.hpp"
#include "Players.hpp"

#include "API.hpp"

#include "Script/script.hpp"
#include "Utility/FileSystem.hpp"

#include "Utility/HttpAsync.hpp"


// Roblox Studio Headers
#include "RbxContent.hpp"
#include "RobloxMainWindow.hpp"
#include "RobloxPluginHost.hpp"
#include "RobloxToolBox.hpp"
#include "AuthenticationHelper.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxGameExplorer.hpp"
#include "RobloxSettings.hpp"
#include "StudioUtilities.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxStudioVerbs.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxWebDoc.hpp"
#include "WebDialog.hpp"
#include "CommonInsertWidget.hpp"
#include "RobloxView.hpp"

#include "QtUtilities.hpp"

// Publish Selection to Roblox
bool RbxWorkspace::isScriptAssetUploadEnabled = false;
bool RbxWorkspace::isAnimationAssetUploadEnabled = false;
bool RbxWorkspace::isImageModelAssetUploadEnabled = false;

FASTFLAG(StudioCSGAssets)

FASTFLAGVARIABLE(StudioEnableGameAnimationsTab, false)
FASTFLAGVARIABLE(StudioRecordToolboxInsert, true)
FASTFLAGVARIABLE(StudioRemoveInsertEvent, false)
FASTFLAGVARIABLE(StudioExposeSessionID, true)

FASTINTVARIABLE(StudioPartSizeCameraDistRatio, 30)

RbxWorkspace::RbxWorkspace(QObject* parent, Aya::DataModel* dm)
    : QObject(parent)
    , m_pDataModel(dm)
    , m_pParent(parent)
{
    m_pContent = new RbxContent(this);
}

RbxWorkspace::~RbxWorkspace()
{
    delete m_pContent;
}

// Publish To Roblox
bool RbxWorkspace::SaveUrl(const QString& saveUrl)
{
    if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || !AuthenticationHelper::validateMachine())
        return false;

    try
    {
        std::string urlStr = saveUrl.toStdString();

        QString url(urlStr.c_str());
        url = url.mid(url.indexOf("assetid=") + 8);
        url = url.left(url.indexOf("&"));
        int placeId = url.toInt();

        Aya::HttpFuture universeFuture;
        if (placeId > 0)
        {
            universeFuture = Aya::HttpAsync::get(Aya::format("%s/universes/get-universe-containing-place?placeId=%d",
                Aya::ContentProvider::getUnsecureApiBaseUrl(RobloxSettings::getBaseURL().toStdString()).c_str(), placeId));
        }

        {
            Aya::DataModel::LegacyLock legacylock(m_pDataModel, Aya::DataModelJob::Write);
            Aya::Visit* visit = Aya::ServiceProvider::create<Aya::Visit>(m_pDataModel);
            visit->setUploadUrl(urlStr); // Set the upload URL so that future publishes go to here as well.

            if (FFlag::StudioCSGAssets)
                Aya::PartOperationAsset::publishAll(m_pDataModel);

            m_pDataModel->save(Aya::ContentId(urlStr));

            if (placeId > 0)
            {
                m_pDataModel->setPlaceID(placeId, false);

                try
                {
                    shared_ptr<const Aya::Reflection::ValueTable> v(new Aya::Reflection::ValueTable);
                    Aya::WebParser::parseJSONTable(universeFuture.get(), v);
                    int universeId = v->at("UniverseId").cast<int>();
                    m_pDataModel->setUniverseId(universeId);
                    if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
                    {
                        ideDoc->updateFromPlaceID(placeId);
                    }
                }
                catch (...)
                {
                }

                QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER), "openGameFromPlaceId",
                    Qt::QueuedConnection, Q_ARG(int, placeId));

                if (RobloxDocManager::Instance().getPlayDoc())
                    QMetaObject::invokeMethod(
                        RobloxDocManager::Instance().getPlayDoc(), "updateFromPlaceID", Qt::QueuedConnection, Q_ARG(int, placeId));
            }
        }

        if (!RobloxDocManager::Instance().getPlayDoc()->isLocalDoc())
            RobloxDocManager::Instance().getPlayDoc()->resetDirty(m_pDataModel);

        return true;
    }
    catch (std::exception&)
    {
        return false;
    }
}

bool RbxWorkspace::Save()
{
    if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || !AuthenticationHelper::validateMachine())
        return false;

    try
    {
        RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();
        if (!pIDEDoc)
            return false;

        Aya::Verb* pVerb = pIDEDoc->getVerb("PublishToRobloxVerb");
        PublishToRobloxVerb* pPublishVerb = dynamic_cast<PublishToRobloxVerb*>(pVerb);

        if (!pPublishVerb)
            return false;

        pPublishVerb->doIt(pIDEDoc->getDataModel());

        return true;
    }
    catch (std::exception&)
    {
        return false;
    }
}

#ifdef ENABLE_EDIT_PLACE_IN_STUDIO
// To start edit of a place
bool RbxWorkspace::StartGame(const QString& ticket, const QString& url, const QString& script)
{
    static int errorCount = 0;
    Hide();
    bool result = UpdateUIManager::Instance().getMainWindow().handleFileOpen("", IRobloxDoc::IDE, script);
    if (result)
    {
        errorCount = 0;
        Close();
    }
    else
    {
        Show();
        errorCount++;
        QMessageBox::critical(NULL, tr(AYA_PROJECT_NAME " Studio"),
            errorCount <= 2 ? tr("There was a problem opening your place.  Please try again.")
                            : tr("Oh no!  It seems we're still unable to open this place.  Please restart your application and try again.  If you "
                                 "continue to encounter this error, please contact customer service."));
    }
    return result;
}
#endif

void RbxWorkspace::InstallPlugin(int assetId, int assetVersion)
{
    QString url = QString("%1/asset/?id=%2").arg(RobloxSettings::getBaseURL()).arg(assetId);
    Aya::Http http(qPrintable(url));
    boost::function<void(bool)> completionCallback =
        boost::bind(&RbxWorkspace::pluginInstallCompleteHelper, boost::weak_ptr<RbxWorkspace>(shared_from_this()), _1, assetId);
    http.get(boost::bind(RobloxPluginHost::processInstallPluginFromWebsite, assetId, assetVersion, _1, _2, completionCallback));
}

QString RbxWorkspace::GetInstalledPlugins()
{
    return RobloxPluginHost::getPluginMetadataJson();
}

void RbxWorkspace::pluginInstallCompleteHelper(weak_ptr<RbxWorkspace> weakWorkspace, bool succeeded, int assetId)
{
    if (shared_ptr<RbxWorkspace> workspace = weakWorkspace.lock())
    {
        Q_EMIT workspace->PluginInstallComplete(succeeded, assetId);
    }
}

void RbxWorkspace::SetPluginEnabled(int assetId, bool enabled)
{
    RobloxPluginHost::setPluginEnabled(assetId, enabled);
}

void RbxWorkspace::UninstallPlugin(int assetId)
{
    RobloxPluginHost::uninstallPlugin(assetId);
}

// Publish Selection To Roblox
QObject* RbxWorkspace::WriteSelection()
{
    if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || !AuthenticationHelper::validateMachine())
    {
        return NULL;
    }

    delete m_pContent;
    m_pContent = new RbxContent(this);

    try
    {
        shared_ptr<Aya::Script> script;
        if (RbxWorkspace::isScriptAssetUploadEnabled)
        {
            Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Read);
            Aya::Selection* sel = Aya::ServiceProvider::find<Aya::Selection>(m_pDataModel);
            if (sel && sel->size() == 1)
                script = Aya::Instance::fastSharedDynamicCast<Aya::Script>(sel->front());
        }

        if (script)
        {
            Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Read);
            if (script->isCodeEmbedded())
                m_pContent->data << script->getEmbeddedCode().get().getSource();
            else
            {
                Aya::BaseScript::Code code = script->requestCode();
                if (code.loaded)
                    m_pContent->data << code.script.get().getSource();
            }
        }
        else
        {
            Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

            if (FFlag::StudioCSGAssets)
                Aya::PartOperationAsset::publishSelection(m_pDataModel);

            Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(getWorkspace());
            Aya::CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<Aya::CSGDictionaryService>(m_pDataModel);
            Aya::NonReplicatedCSGDictionaryService* nrDictionaryService =
                Aya::ServiceProvider::create<Aya::NonReplicatedCSGDictionaryService>(m_pDataModel);

            if (dictionaryService && nrDictionaryService)
                for (Aya::Instances::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
                {
                    dictionaryService->retrieveAllDescendants(*iter);
                    nrDictionaryService->retrieveAllDescendants(*iter);
                }

            Aya::Instances instances(sel->begin(), sel->end());
            Aya::SerializerBinary::serialize(m_pContent->data, instances);

            if (dictionaryService && nrDictionaryService)
                for (Aya::Instances::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
                {
                    dictionaryService->storeAllDescendants(*iter);
                    nrDictionaryService->storeAllDescendants(*iter);
                }
        }
    }
    catch (std::exception& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
        return NULL;
    }

    return m_pContent;
}

void RbxWorkspace::writeToStream(const XmlElement* root, std::ostream& stream)
{
    TextXmlWriter machine(stream);
    machine.serialize(root);
}

// Do not confuse the DataModel Workspace with the RbxWorkspace. We are using RbxWorkspace as a Javascript Bridge Object
Aya::Workspace* RbxWorkspace::getWorkspace() const
{
    return m_pDataModel ? m_pDataModel->getWorkspace() : NULL;
}

std::auto_ptr<XmlElement> RbxWorkspace::writeSelectionToXml()
{
    std::auto_ptr<XmlElement> root(Serializer::newRootElement());
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(getWorkspace());
        Aya::AddSelectionToRoot(root.get(), sel, Aya::SerializationCreator);
        if (sel && sel->size() == 1 && dynamic_cast<Aya::KeyframeSequence*>(sel->front().get()))
        {
            root->addAttribute(tag_assettype, "animation");
        }
    }
    return root;
}

QString RbxWorkspace::GetStudioSessionID()
{
    return QString("");
}

bool RbxWorkspace::Insert(const QString& urlQStr)
{
    try
    {
        if (!m_pDataModel)
            throw std::runtime_error("Can't insert at this time");

        QByteArray ba = urlQStr.toLocal8Bit();
        const char* url = ba.data();

        bool success = Aya::Http::trustCheck(url);
        std::string urlString = urlQStr.toStdString();

        insert(Aya::ContentId(urlString), false);
    }
    catch (std::exception const& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
        return false;
    }
    return true;
}

// InsertInto:	TREE_VIEW, SUPPRESS_PROMPTS
// Insert:      3D_VIEW, ALLOW_PROMPTS

void RbxWorkspace::insert(std::istream& stream, bool insertInto)
{
    if (!m_pDataModel)
        throw std::runtime_error("Can't insert at this time");

    // The following code happens BEFORE we do a lock because we can't display a message box within a lock
    Aya::Instances instances;

    Serializer().loadInstances(stream, instances);

    insert(instances, insertInto);
}

void RbxWorkspace::insert(Aya::Instances& instances, bool insertInto)
{
    {
        // Lock the data model temporarily to make surRBX::e insertion is legal
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

        if (Aya::ContentProvider* cp = m_pDataModel->create<Aya::ContentProvider>())
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
            if (Aya::Instance::fastDynamicCast<Aya::Tool>(single))
            {
                if (QMessageBox::question(&UpdateUIManager::Instance().getMainWindow(), "Insert Tool",
                        "Put this tool into the starter pack (otherwise drop into the 3d view)?", QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::Yes)
                    promptMode = Aya::PUT_TOOL_IN_STARTERPACK;
            }
        }
    }

    // Now lock the datamodel (after MessageBox)
    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    shared_ptr<Aya::Workspace> spWorkspace = shared_from(getWorkspace());
    if (!spWorkspace)
        throw std::runtime_error("Can't insert at this time");

    if (DFFlag::MaterialPropertiesEnabled)
    {
        StudioUtilities::convertPhysicalPropertiesIfNeeded(instances, spWorkspace.get());
    }

    Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(spWorkspace.get());

    Aya::Instance* requestedParent = 0;

    requestedParent = spWorkspace.get();

    Aya::InsertMode insertMode = Aya::INSERT_TO_3D_VIEW;

    // if we have a selection make sure we can invoke insert decal tool
    if ((instances.size() == 1) && Aya::Instance::fastDynamicCast<Aya::Decal>(instances[0].get()) && (sel->size() == 1))
    {
        requestedParent = sel->front().get();
        insertMode = Aya::Instance::fastDynamicCast<Aya::PartInstance>(requestedParent) ? Aya::INSERT_TO_TREE : Aya::INSERT_TO_3D_VIEW;
    }

    try
    {
        // get position to insert at
        bool isOnPart = false;
        Aya::Vector3 pos = InsertObjectWidget::getInsertLocation(shared_from(m_pDataModel), NULL, &isOnPart);
        spWorkspace->insertInstances(instances, requestedParent, insertMode, promptMode, isOnPart ? &pos : NULL);

        if (promptMode != Aya::PUT_TOOL_IN_STARTERPACK)
        {
            // zoom camera at the inserted instances
            Aya::PartArray parts;
            for (size_t i = 0; i < instances.size(); ++i)
                Aya::PartInstance::findParts(instances[i].get(), parts);
            if (parts.size() > 0)
            {
                Aya::Extents ext = Aya::DragUtilities::computeExtents(parts);
                if (!ext.isNanInf() && !ext.isNull())
                {
                    Aya::Camera* camera = spWorkspace->getCamera();
                    float cameraDistance = fabsf((camera->getCameraCoordinateFrame().translation - ext.center()).magnitude());
                    // if extents are out of camera frustum or size of the part is small, we need to zoom in/out
                    if (!camera->frustum().containsAABB(ext) ||
                        ((cameraDistance > 0) && ext.longestSide() / cameraDistance < FInt::StudioPartSizeCameraDistRatio / 100.0f))
                    {
                        ext.expand(ext.longestSide());
                        camera->zoomExtents(ext, Aya::Camera::ZOOM_IN_OR_OUT);
                    }
                }
            }
        }

        boost::shared_ptr<Aya::RunService> runService = shared_from(m_pDataModel->create<Aya::RunService>());
        // If we're not playing a game then select the content
        if (runService->getRunState() == Aya::RS_STOPPED)
        {
            sel->setSelection(instances.begin(), instances.end());
            RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
            if (playDoc)
            {
                UpdateUIManager::Instance().updateToolBars();
            }
        }
    }
    catch (std::exception&)
    {
        std::for_each(instances.begin(), instances.end(), boost::bind(&Aya::Instance::setParent, _1, (Aya::Instance*)NULL));
        throw;
    }
}


void RbxWorkspace::insert(Aya::ContentId contentId, bool insertInto)
{
    if (!m_pDataModel)
        throw std::runtime_error("Can't insert at this time");

    // load instances in a different thread
    bool hasError = false;
    Aya::Instances instances;
    UpdateUIManager::Instance().waitForLongProcess(
        "Inserting...", boost::bind(&RbxWorkspace::loadContent, this, contentId, boost::ref(instances), boost::ref(hasError)));

    if (hasError || instances.size() < 1)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, "Selected object cannot be inserted at this time.");
        return;
    }

    // now insert instances
    insert(instances, insertInto);
}

void RbxWorkspace::loadContent(Aya::ContentId contentId, Aya::Instances& instances, bool& hasError)
{
    try
    {
        std::auto_ptr<std::istream> stream(Aya::ServiceProvider::create<Aya::ContentProvider>(m_pDataModel)->getContent(contentId));
        Serializer().loadInstances(*stream, instances);
        hasError = false;
    }
    catch (...)
    {
        hasError = true;
    }
}

#ifdef ENABLE_DRAG_DROP_TOOLBOX
bool RbxWorkspace::StartDrag(const QString& urlQStr)
{
    try
    {
        if (!m_pDataModel)
            throw std::runtime_error("Can't insert at this time");

        bool success = Aya::Http::trustCheck(urlQStr.toLocal8Bit().data());
        if (success)
        {
            QDrag* pDrag = new QDrag(qobject_cast<QWidget*>(m_pParent));
            QMimeData* pMimeData = new QMimeData;

            QList<QUrl> listUrl;
            listUrl << urlQStr;

            pMimeData->setUrls(listUrl);
            pDrag->setMimeData(pMimeData);

            QPixmap pixMap(1, 1);
            pDrag->setPixmap(pixMap);

#ifdef Q_WS_MAC
            pDrag->exec();
#else
            pDrag->exec(Qt::CopyAction);
#endif
            pDrag->deleteLater();
        }
    }
    catch (std::exception const& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
        return false;
    }

    return true;
}
#endif

bool RbxWorkspace::OpenRecentFile(const QString& recentFileName)
{
    return UpdateUIManager::Instance().getMainWindow().openRecentFile(recentFileName);
}



bool RbxWorkspace::OpenPicFolder()
{
    QString userPictureFolder = QtUtilities::qstringFromStdString(Aya::FileSystem::getUserDirectory(true, Aya::DirPicture).native());
    QDir pictureDir(userPictureFolder);

    if (!pictureDir.exists())
        pictureDir.mkpath(userPictureFolder);

    QDesktopServices::openUrl(QUrl::fromLocalFile(userPictureFolder));

    return true;
}


void RbxWorkspace::OpenUniverse(int universeId)
{
    QAction* action = UpdateUIManager::Instance().getDockAction(eDW_GAME_EXPLORER);
    if (!action->isChecked())
    {
        action->activate(QAction::Trigger);
    }
    QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER), "openGameFromGameId",
        Qt::QueuedConnection, Q_ARG(int, universeId));
}

void RbxWorkspace::PublishAsNewUniverse()
{
    QMetaObject::invokeMethod(
        &UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER), "publishGameToNewSlot", Qt::QueuedConnection);
}

void RbxWorkspace::PublishAsNewGroupUniverse(int groupId)
{
    QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER), "publishGameToNewGroupSlot",
        Qt::QueuedConnection, Q_ARG(int, groupId));
}

void RbxWorkspace::PublishToUniverse(int universeId)
{
    QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER), "publishGameToExistingSlot",
        Qt::QueuedConnection, Q_ARG(int, universeId));
}

void RbxWorkspace::ImportAsset(int assetId)
{
    if (!FFlag::StudioEnableGameAnimationsTab)
        return;

    ImportAssetDialog* pImportDlg = dynamic_cast<ImportAssetDialog*>(m_pParent);
    if (pImportDlg)
    {
        pImportDlg->setAssetId(assetId);
        pImportDlg->close();
    }
}

QString RbxWorkspace::GetGameAnimations(int universeId)
{
    if (!FFlag::StudioEnableGameAnimationsTab)
        return QString();

    RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
    return gameExplorer.getAnimationsDataJson();
}