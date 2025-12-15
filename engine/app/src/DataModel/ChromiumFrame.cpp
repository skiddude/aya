#include "Utility/CefIntegration.hpp"

#include "DataModel/ChromiumFrame.hpp"
#include "DataModel/GuiLayerCollector.hpp"

#include "Render/AdornRender.hpp"

#include "Utility/SoundChannel.hpp"
#include "Utility/SoundService.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/URL.hpp"

#include "Profiler.hpp"
#include "Players.hpp"
#include "fmod.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/thread/once.hpp>
#include <filesystem>

AYA_REGISTER_ENUM(ChromiumConsoleMessageSeverity);
AYA_REGISTER_ENUM(ChromiumLoadState);
AYA_REGISTER_ENUM(ChromiumMouseCursor);

DYNAMIC_FASTINTVARIABLE(ChromiumFrameMaximum, 10);
DYNAMIC_FASTSTRINGVARIABLE(ChromiumFramePlaceIdWhitelist, ""); // comma separated
DYNAMIC_FASTSTRINGVARIABLE(ChromiumFrameDomainWhitelist, "");  // comma separated
DYNAMIC_FASTSTRINGVARIABLE(ChromiumFrameURLWhitelist, "");     // comma separated

namespace Aya
{

const char* const sChromiumFrame = "ChromiumFrame";

int nChromiumFrames = 0;

bool whitelisted(std::string haystack, std::string needle)
{
    std::vector<std::string> results;
    boost::split(results, haystack, boost::is_any_of(","));
    return std::find(results.begin(), results.end(), needle) != results.end();
}

bool whitelisted(std::string haystack, int needle)
{
    return whitelisted(haystack, std::to_string(needle));
}

static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_refresh(&ChromiumFrame::refresh, "PageRefresh", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_backward(&ChromiumFrame::backward, "PageBackward", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_forward(&ChromiumFrame::forward, "PageForward", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_cancel(&ChromiumFrame::cancel, "CancelLoading", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_refreshNoCache(&ChromiumFrame::refreshNoCache, "PageRefreshNoCache", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_focus(&ChromiumFrame::focus, "Focus", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_unfocus(&ChromiumFrame::unfocus, "Unfocus", Security::None);

static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_initialize(&ChromiumFrame::initialize, "Initialize", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void()> func_uninitialize(&ChromiumFrame::uninitialize, "Uninitialize", Security::None);

static Reflection::BoundFuncDesc<ChromiumFrame, std::string(std::string)> func_runJS(&ChromiumFrame::runJS, "RunJS", "code", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int, bool)> func_mouseLeftClick(
    &ChromiumFrame::mouseLeftClick, "MouseLeftClick", "x", "y", "isDown", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int, bool)> func_mouseMiddleClick(
    &ChromiumFrame::mouseMiddleClick, "MouseMiddleClick", "x", "y", "isDown", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int, bool)> func_mouseRightClick(
    &ChromiumFrame::mouseRightClick, "MouseRightClick", "x", "y", "isDown", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int, int, int)> func_mouseWheel(
    &ChromiumFrame::mouseWheel, "MouseWheel", "x", "y", "deltaX", "deltaY", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int)> func_mouseMove(&ChromiumFrame::mouseMove, "MouseMove", "x", "y", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, void(int, int, bool)> func_keyEvent(
    &ChromiumFrame::keyEvent, "KeyEvent", "keyCode", "modifiers", "isKeyDown", Security::None);
static Reflection::BoundFuncDesc<ChromiumFrame, Color3(int, int)> func_getColorAtPixel(
    &ChromiumFrame::getColorAtPixel, "GetColorAtPixel", "x", "y", Security::None);

static Reflection::EnumPropDescriptor<ChromiumFrame, ChromiumLoadState> prop_loadState(
    "LoadState", "Browser", &ChromiumFrame::getLoadState, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_initialized(
    "Initialized", "Browser", &ChromiumFrame::getInitialized, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_canPageBackward(
    "CanPageBackward", "Browser", &ChromiumFrame::getCanPageBackward, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_canPageForward(
    "CanPageForward", "Browser", &ChromiumFrame::getCanPageForward, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_isLoaded(
    "IsLoaded", "Browser", &ChromiumFrame::getIsLoaded, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, Vector2> prop_resolution(
    "Resolution", "Browser", &ChromiumFrame::getResolution, &ChromiumFrame::setResolution, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ChromiumFrame, Vector2> prop_mousePosition(
    "MousePosition", "Browser", &ChromiumFrame::getMousePosition, &ChromiumFrame::setMousePosition, Reflection::PropertyDescriptor::STANDARD);
static Reflection::EnumPropDescriptor<ChromiumFrame, ChromiumMouseCursor> prop_mouseCursor(
    "MouseCursor", "Browser", &ChromiumFrame::getMouseCursor, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, Vector2> prop_scrollOffset(
    "ScrollOffset", "Browser", &ChromiumFrame::getScrollOffset, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, int> prop_statusCode(
    "StatusCode", "Browser", &ChromiumFrame::getStatusCode, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, std::string> prop_url(
    "URL", "Browser", &ChromiumFrame::getURL, &ChromiumFrame::setURL, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ChromiumFrame, std::string> prop_pageTitle(
    "PageTitle", "Browser", &ChromiumFrame::getPageTitle, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, float> prop_zoom(
    "Zoom", "Browser", &ChromiumFrame::getZoom, &ChromiumFrame::setZoom, Reflection::PropertyDescriptor::STANDARD);

static Reflection::PropDescriptor<ChromiumFrame, float> prop_volume(
    "Volume", "Audio", &ChromiumFrame::getVolume, &ChromiumFrame::setVolume, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ChromiumFrame, float> prop_pitch(
    "Pitch", "Audio", &ChromiumFrame::getPitch, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_isPlaying(
    "IsPlaying", category_Data, &ChromiumFrame::isPlaying, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<ChromiumFrame, bool> prop_worldAudioEnabled("WorldAudioEnabled", "Audio", &ChromiumFrame::getWorldAudioEnabled,
    &ChromiumFrame::setWorldAudioEnabled, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ChromiumFrame, float> prop_worldAudioMinDistance("WorldAudioMinDistance", "Audio",
    &ChromiumFrame::getWorldAudioMinDistance, &ChromiumFrame::setWorldAudioMinDistance, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ChromiumFrame, float> prop_worldAudioMaxDistance("WorldAudioMaxDistance", "Audio",
    &ChromiumFrame::getWorldAudioMaxDistance, &ChromiumFrame::setWorldAudioMaxDistance, Reflection::PropertyDescriptor::STANDARD);
static Reflection::EnumPropDescriptor<ChromiumFrame, Soundscape::RollOffMode> prop_worldAudioRollOff("WorldAudioRollOff", "Audio",
    &ChromiumFrame::getWorldAudioRollOff, &ChromiumFrame::setWorldAudioRollOff, Reflection::PropertyDescriptor::STANDARD);

static Reflection::RemoteEventDesc<ChromiumFrame, void(bool)> event_ChromiumFrameLoaded(&ChromiumFrame::isLoadedSignal, "Loaded", "isLoaded",
    Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<ChromiumFrame, void(ChromiumMouseCursor)> event_ChromiumFrameMouseCursorChanged(
    &ChromiumFrame::mouseCursorChangedSignal, "MouseCursorChanged", "cursor", Security::None, Reflection::RemoteEventCommon::SCRIPTING,
    Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<ChromiumFrame, void(Vector2)> event_scrollOffsetChanged(&ChromiumFrame::scrollOffsetChangedSignal,
    "ScrollOffsetChanged", "offset", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);

Reflection::RemoteEventDesc<ChromiumFrame, void(std::string, ChromiumConsoleMessageSeverity, std::string, int)>
    ChromiumFrame::event_ChromiumFrameConsoleMessage(&ChromiumFrame::onConsoleMessageSignal, "ConsoleMessage", "message", "severity", "source",
        "line", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);

} // namespace Aya

namespace Aya
{
namespace Reflection
{

template<>
EnumDesc<ChromiumConsoleMessageSeverity>::EnumDesc()
    : EnumDescriptor("ChromiumConsoleMessageSeverity")
{
    addPair(ChromiumConsoleMessageSeverity::Default, "Default");
    addPair(ChromiumConsoleMessageSeverity::Verbose, "Verbose");
    addPair(ChromiumConsoleMessageSeverity::Info, "Info");
    addPair(ChromiumConsoleMessageSeverity::Warning, "Warning");
    addPair(ChromiumConsoleMessageSeverity::Error, "Error");
    addPair(ChromiumConsoleMessageSeverity::Fatal, "Fatal");
}

template<>
EnumDesc<ChromiumLoadState>::EnumDesc()
    : EnumDescriptor("ChromiumLoadState")
{
    addPair(ChromiumLoadState::Stopped, "Stopped");
    addPair(ChromiumLoadState::Loaded, "Loaded");
    addPair(ChromiumLoadState::Loading, "Loading");
    addPair(ChromiumLoadState::LoadError, "LoadError");
}

template<>
EnumDesc<ChromiumMouseCursor>::EnumDesc()
    : EnumDescriptor("ChromiumMouseCursor")
{
    addPair(ChromiumMouseCursor::Pointer, "Pointer");
    addPair(ChromiumMouseCursor::Cross, "Cross");
    addPair(ChromiumMouseCursor::Hand, "Hand");
    addPair(ChromiumMouseCursor::IBeam, "IBeam");
    addPair(ChromiumMouseCursor::Wait, "Wait");
    addPair(ChromiumMouseCursor::Help, "Help");
    addPair(ChromiumMouseCursor::EastResize, "EastResize");
    addPair(ChromiumMouseCursor::NorthResize, "NorthResize");
    addPair(ChromiumMouseCursor::NortheastResize, "NortheastResize");
    addPair(ChromiumMouseCursor::NorthwestResize, "NorthwestResize");
    addPair(ChromiumMouseCursor::SouthResize, "SouthResize");
    addPair(ChromiumMouseCursor::SoutheastResize, "SoutheastResize");
    addPair(ChromiumMouseCursor::SouthwestResize, "SouthwestResize");
    addPair(ChromiumMouseCursor::WestResize, "WestResize");
    addPair(ChromiumMouseCursor::NorthSouthResize, "NorthSouthResize");
    addPair(ChromiumMouseCursor::EastWestResize, "EastWestResize");
    addPair(ChromiumMouseCursor::NortheastSouthwestResize, "NortheastSouthwestResize");
    addPair(ChromiumMouseCursor::NorthwestSoutheastResize, "NorthwestSoutheastResize");
    addPair(ChromiumMouseCursor::ColumnResize, "ColumnResize");
    addPair(ChromiumMouseCursor::RowResize, "RowResize");
    addPair(ChromiumMouseCursor::MiddlePanning, "MiddlePanning");
    addPair(ChromiumMouseCursor::EastPanning, "EastPanning");
    addPair(ChromiumMouseCursor::NorthPanning, "NorthPanning");
    addPair(ChromiumMouseCursor::NortheastPanning, "NortheastPanning");
    addPair(ChromiumMouseCursor::NorthwestPanning, "NorthwestPanning");
    addPair(ChromiumMouseCursor::SouthPanning, "SouthPanning");
    addPair(ChromiumMouseCursor::SoutheastPanning, "SoutheastPanning");
    addPair(ChromiumMouseCursor::SouthwestPanning, "SouthwestPanning");
    addPair(ChromiumMouseCursor::WestPanning, "WestPanning");
    addPair(ChromiumMouseCursor::Move, "Move");
    addPair(ChromiumMouseCursor::VerticalText, "VerticalText");
    addPair(ChromiumMouseCursor::Cell, "Cell");
    addPair(ChromiumMouseCursor::ContextMenu, "ContextMenu");
    addPair(ChromiumMouseCursor::Alias, "Alias");
    addPair(ChromiumMouseCursor::Progress, "Progress");
    addPair(ChromiumMouseCursor::NoDrop, "NoDrop");
    addPair(ChromiumMouseCursor::Copy, "Copy");
    addPair(ChromiumMouseCursor::None, "None");
    addPair(ChromiumMouseCursor::NotAllowed, "NotAllowed");
    addPair(ChromiumMouseCursor::ZoomIn, "ZoomIn");
    addPair(ChromiumMouseCursor::ZoomOut, "ZoomOut");
    addPair(ChromiumMouseCursor::Grab, "Grab");
    addPair(ChromiumMouseCursor::Grabbing, "Grabbing");
    addPair(ChromiumMouseCursor::MiddlePanningVertical, "MiddlePanningVertical");
    addPair(ChromiumMouseCursor::MiddlePanningHorizontal, "MiddlePanningHorizontal");
    addPair(ChromiumMouseCursor::Custom, "Custom");
    addPair(ChromiumMouseCursor::DndNone, "DndNone");
    addPair(ChromiumMouseCursor::DndMove, "DndMove");
    addPair(ChromiumMouseCursor::DndCopy, "DndCopy");
    addPair(ChromiumMouseCursor::DndLink, "DndLink");
}

} // namespace Reflection
} // namespace Aya

namespace Aya
{

REFLECTION_END();

ChromiumFrame::ChromiumFrame()
    : DescribedCreatable<ChromiumFrame, GuiLabel, sChromiumFrame>("ChromiumFrame")
    , initialized(false)
    , resolution(Vector2(1920, 1080))
    , mousePosition(Vector2(0, 0))
    , url("")
    , texture(nullptr)
    , volume(0.5)
    , pitch(1)
    , worldAudioEnabled(false)
    , worldAudioMinDistance(10)
    , worldAudioMaxDistance(100000)
    , worldAudioRollOff(Soundscape::Inverse)
    , fmod_channel(nullptr)
    , part(nullptr)
{
#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    this->buffer.resize(this->resolution.x * this->resolution.y * 4);
    this->lastPaintWidth = this->resolution.x;
    this->lastPaintHeight = this->resolution.y;
#endif
}

ChromiumFrame::~ChromiumFrame()
{
    if (this->initialized)
        this->uninitialize();
}

void ChromiumFrame::render2d(Adorn* adorn)
{
    AYAPROFILER_SCOPE("CefIntegration", "ChromiumFrame::render2d");

#ifndef AYA_SERVER
#ifdef ENABLE_CHROMIUM_FRAMES
    if (this->initialized && this->shouldReuploadTexture)
    {
        if (this->texture == nullptr || this->texture->getWidth() != this->lastPaintWidth || this->texture->getHeight() != this->lastPaintHeight)
        {
            Aya::StandardOut::singleton()->print(MESSAGE_INFO, "Creating texture for ChromiumFrame");
            TextureProxyBaseRef ref = adorn->createTextureProxy(this->lastPaintWidth, this->lastPaintHeight);
            guiImageDraw.setImageFromRef(ref);
            this->texture = boost::dynamic_pointer_cast<Aya::Graphics::TextureProxy>(ref)->getTexture();
        }

        this->texture->upload(0, 0, Graphics::TextureRegion(0, 0, 0, this->texture->getWidth(), this->texture->getHeight(), 1), this->buffer.data(),
            this->buffer.size());
        this->shouldReuploadTexture = false;
    }
#endif

    GuiObject* clippingObject = firstAncestorClipping();

    if (clippingObject == NULL || !absoluteRotation.empty())
        guiImageDraw.render2d(adorn, true, getRect2D(), Vector2(0, 0), Vector2(1, 1), Color4(1, 1, 1, 1), absoluteRotation, Gui::NOTHING, false);
    else
        guiImageDraw.render2d(
            adorn, true, getRect2D(), Vector2(0, 0), Vector2(1, 1), Color4(1, 1, 1, 1), clippingObject->getClippedRect(), Gui::NOTHING, false);
#endif
}

void ChromiumFrame::renderBackground2d(Adorn* adorn)
{
    AYAPROFILER_SCOPE("CefIntegration", "ChromiumFrame::renderBackground2d");

    if (this->getBackgroundTransparency() < 1.0)
        GuiObject::render2dImpl(adorn, getRenderBackgroundColor4());
}

void ChromiumFrame::initialize()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("CEF integration has not been compiled into this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::Initialize can only be called from the client");

    if (this->initialized)
        throw std::runtime_error("Cannot initialize an already initialized ChromiumFrame");

    if (nChromiumFrames >= DFInt::ChromiumFrameMaximum)
        throw std::runtime_error("Maximum number of ChromiumFrames reached");

    if (!DFString::ChromiumFramePlaceIdWhitelist.empty())
        if (!whitelisted(DFString::ChromiumFramePlaceIdWhitelist, DataModel::get(this)->getPlaceID()))
            throw std::runtime_error("ChromiumFrame initialization blocked: this place is not whitelisted");

#ifndef AYA_SERVER
    if (!CefIntegration::isAvailable() || !CefIntegration::isInitialized())
        throw std::runtime_error("CEF integration is not available because it is not installed or it failed during startup");

    if (this->url.empty())
    {
        std::filesystem::path path = getModuleParentPath() / "content" / "ChromiumFrame.html";
        this->url = "file:///" + path.string();
    }

    this->cefRenderer = new AyaCefRenderer(this);
    this->cefClient = new AyaCefClient(this);

    CefBrowserSettings browser_settings;
    CefWindowInfo window_info;

    browser_settings.background_color = CefColorSetARGB(255, 255, 255, 255);
    browser_settings.windowless_frame_rate = Aya::GameBasicSettings::singleton().getMaxFramerate();
    browser_settings.javascript_access_clipboard = STATE_DISABLED;
    // browser_settings.local_storage = STATE_DISABLED;

    window_info.SetAsWindowless(0);
    CefRefPtr<CefDictionaryValue> emptyDictionary = CefDictionaryValue::Create();
    CefRefPtr<CefRequestContext> globalContext = CefRequestContext::GetGlobalContext();

    CefBrowserHost::CreateBrowser(window_info, this->cefClient.get(), this->getURL(), browser_settings, emptyDictionary, globalContext);

    nChromiumFrames++;
#endif
#endif
}

void ChromiumFrame::uninitialize()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::Uninitalize can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot uninitalized a non-initialized ChromiumFrame");

    Aya::StandardOut::singleton()->printf(MESSAGE_INFO, "Uninitializing ChromiumFrame");

#ifndef AYA_SERVER
    this->stopSound();
    this->cefInstance->GetHost()->CloseBrowser(true);
    this->setInitializedInternal(false);
    this->cefInstance = nullptr;
    this->cefRenderer = nullptr;
    this->texture = nullptr;
    guiImageDraw.setImageFromRef(nullptr);

    nChromiumFrames--;
#endif
#endif
}

void ChromiumFrame::refresh()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::Refresh can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot refresh on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->cefInstance->GetMainFrame()->GetBrowser()->Reload();
#endif
#endif
}

void ChromiumFrame::refreshNoCache()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::RefreshNoCache can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot refresh on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->cefInstance->GetMainFrame()->GetBrowser()->ReloadIgnoreCache();
#endif
#endif
}

void ChromiumFrame::backward()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::PageBackward can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot go backwards on a non-initialized ChromiumFrame");
#ifndef AYA_SERVER
    if (this->cefInstance->GetMainFrame()->GetBrowser()->CanGoBack())
        this->cefInstance->GetMainFrame()->GetBrowser()->GoBack();
    else
        throw std::runtime_error("Cannot go backwards anymore");
#endif
#endif
}

void ChromiumFrame::forward()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::PageForward can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot go forwards on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    if (this->cefInstance->GetMainFrame()->GetBrowser()->CanGoForward())
        this->cefInstance->GetMainFrame()->GetBrowser()->GoForward();
    else
        throw std::runtime_error("Cannot go forwards anymore");
#endif
#endif
}

void ChromiumFrame::cancel()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::CancelLoading can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot go forwards on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->cefInstance->GetMainFrame()->GetBrowser()->StopLoad();
#endif
#endif
}

void ChromiumFrame::onAncestorChanged(const AncestorChanged& event)
{
    GuiObject::onAncestorChanged(event);

    if (event.child == this)
    {
        part = Instance::fastDynamicCast<PartInstance>(event.newParent);
    }

#if !defined(AYA_SERVER) || !defined(ENABLE_CHROMIUM_FRAMES)
    // we uninit if it is null and/or if it is not a gui object of importance to us
    // why? because it doesn't make sense to keep playing if it is not being rendered...
    if (this->initialized &&
        !(Aya::Instance::fastDynamicCast<Aya::GuiObject>(event.newParent) || Aya::Instance::fastDynamicCast<Aya::GuiLayerCollector>(event.newParent)))
    {
        this->uninitialize();
    }
#endif
}

void ChromiumFrame::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
    {
        Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(oldProvider);
        if (soundService)
            soundService->unregisterChromiumFrame(this);

        if (this->initialized)
            this->uninitialize();
    }

    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(newProvider);
        soundService->registerChromiumFrame(this);
        part = Instance::fastDynamicCast<PartInstance>(this->getParent());
    }
}

void ChromiumFrame::setLoadStateInternal(ChromiumLoadState value)
{
    if (this->loadState == value)
        return;

    loadState = value;

    raisePropertyChanged(prop_loadState);
    raisePropertyChanged(prop_isLoaded);

    if (value == ChromiumLoadState::Loaded)
        event_ChromiumFrameLoaded.fireEvent(this, this->getIsLoaded());
}

void ChromiumFrame::setStatusCodeInternal(int value)
{
    if (this->statusCode == value)
        return;

    statusCode = value;
    raisePropertyChanged(prop_statusCode);
}

void ChromiumFrame::setPageTitleInternal(std::string value)
{
    if (this->pageTitle == value)
        return;

    pageTitle = value;
    raisePropertyChanged(prop_pageTitle);
}

void ChromiumFrame::setInitializedInternal(bool value)
{
    if (this->initialized == value)
        return;

    initialized = value;
    raisePropertyChanged(prop_initialized);
}

void ChromiumFrame::setURL(std::string value)
{
    if (this->url == value)
        return;

    if (!DFString::ChromiumFrameURLWhitelist.empty())
    {
        if (!whitelisted(DFString::ChromiumFrameURLWhitelist, value))
            throw std::runtime_error("ChromiumFrame navigation blocked: URL is not whitelisted");
    }

    if (!DFString::ChromiumFrameDomainWhitelist.empty())
    {
        std::string domain;
        try
        {
            domain = Aya::Url::fromString(value).host();
        }
        catch (...)
        {
            throw std::runtime_error("Failed to parse URL for domain whitelist check");
        }

        if (!whitelisted(DFString::ChromiumFrameDomainWhitelist, domain))
            throw std::runtime_error("ChromiumFrame navigation blocked: domain is not whitelisted");
    }

    this->url = value;
    raisePropertyChanged(prop_url);

#if !defined(AYA_SERVER) && defined(ENABLE_CHROMIUM_FRAMES)
    if (this->initialized)
        this->cefInstance->GetMainFrame()->LoadURL(this->url);
#endif
}

void ChromiumFrame::setURLInternal(std::string value)
{
    if (this->url == value)
        return;

    this->url = value;
    raisePropertyChanged(prop_url);
}

void ChromiumFrame::setCanPageForward(bool value)
{
    if (this->canPageForward == value)
        return;

    this->canPageForward = value;
    raisePropertyChanged(prop_canPageForward);
}

void ChromiumFrame::setCanPageBackward(bool value)
{
    if (this->canPageBackward == value)
        return;

    this->canPageBackward = value;
    raisePropertyChanged(prop_canPageBackward);
}

void ChromiumFrame::setResolution(Vector2 value)
{
    if (this->resolution == value)
        return;

    if (value.x <= 0 || value.y <= 0)
        throw std::runtime_error("The x/y resolution of a ChromiumFrame must be a positive integer");

    if (value.x * value.y > (8192 * 4320))
        throw std::runtime_error("ChromiumFrame resolution cannot currently exceed 8K Full Format (8192x4320)");

    this->resolution = value;
    raisePropertyChanged(prop_resolution);

#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    if (this->initialized)
    {
        this->cefRenderer->UpdateResolution(resolution.x, resolution.y);
        this->cefInstance->GetHost()->WasResized();
    }
#endif
}

void ChromiumFrame::setMousePosition(Vector2 value)
{
    if (this->mousePosition == value)
        return;

    this->mousePosition = value;
    raisePropertyChanged(prop_mousePosition);

#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    if (this->initialized)
    {
        CefMouseEvent mouseEvent;
        mouseEvent.x = this->mousePosition.x;
        mouseEvent.y = this->mousePosition.y;

        this->cefInstance->GetHost()->SendMouseMoveEvent(mouseEvent, false);
    }
#endif
}

void ChromiumFrame::setMouseCursorInternal(ChromiumMouseCursor value)
{
    if (this->mouseCursor == value)
        return;

    this->mouseCursor = value;
    raisePropertyChanged(prop_mouseCursor);

    event_ChromiumFrameMouseCursorChanged.fireEvent(this, value);
}

void ChromiumFrame::setScrollOffsetInternal(Vector2 value)
{
    if (this->scrollOffset == value)
        return;

    this->scrollOffset = value;
    raisePropertyChanged(prop_scrollOffset);

    event_scrollOffsetChanged.fireEvent(this, value);
}

std::string ChromiumFrame::runJS(std::string code)
{
    AYAPROFILER_SCOPE("CefIntegration", "ChromiumFrame::runJS");

#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::RunJS can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot evaluate JavaScript on a non-initialized ChromiumFrame");
#ifndef AYA_SERVER
    std::promise<std::string> prom;
    std::future<std::string> fut = prom.get_future();

    this->cefClient.get()->SetPromise(&prom);

    std::string wrappedCode = "sendResultToEngine((function() { try { " + code + "; } catch(e) { return 'Error: ' + e.message; }})());";
    this->cefInstance->GetMainFrame()->ExecuteJavaScript(wrappedCode, "", 0);

    // bye bye after 5 secs
    std::future_status status = fut.wait_for(std::chrono::seconds(5));
    if (status == std::future_status::timeout)
    {
        throw std::runtime_error("JavaScript execution timed out.");
    }
    else if (status == std::future_status::ready)
    {
        return fut.get();
    }
    else
    {
        throw std::runtime_error("JavaScript execution failed.");
    }
#endif
#endif
}

#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
void ChromiumFrame::mouseClick(int x, int y, cef_mouse_button_type_t button, bool isDown)
{
    CefMouseEvent mouseEvent;
    mouseEvent.x = x;
    mouseEvent.y = y;

    this->cefInstance->GetHost()->SendMouseClickEvent(mouseEvent, button, isDown, 1);
}
#endif

void ChromiumFrame::mouseLeftClick(int x, int y, bool isDown)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::MouseLeftClick can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->mouseClick(x, y, MBT_LEFT, isDown);
#endif
#endif
}

void ChromiumFrame::mouseMiddleClick(int x, int y, bool isDown)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::MouseMiddleClick can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->mouseClick(x, y, MBT_MIDDLE, isDown);
#endif
#endif
}

void ChromiumFrame::mouseRightClick(int x, int y, bool isDown)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::MouseRightClick can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->mouseClick(x, y, MBT_RIGHT, isDown);
#endif
#endif
}

void ChromiumFrame::mouseWheel(int x, int y, int deltaX, int deltaY)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::MouseWheel can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    CefMouseEvent mouseEvent;
    mouseEvent.x = x;
    mouseEvent.y = y;

    this->cefInstance->GetHost()->SendMouseWheelEvent(mouseEvent, deltaX, deltaY);
#endif
#endif
}

void ChromiumFrame::mouseMove(int x, int y)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::MouseMove can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    CefMouseEvent mouseEvent;
    mouseEvent.x = x;
    mouseEvent.y = y;

    this->cefInstance->GetHost()->SendMouseMoveEvent(mouseEvent, false);
#endif
#endif
}

void ChromiumFrame::keyEvent(int keyCode, int modifiers, bool isKeyDown)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::KeyEvent can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot run inputs on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    CefKeyEvent keyEvent;
    keyEvent.windows_key_code = keyCode;
    keyEvent.modifiers = modifiers;
    keyEvent.type = isKeyDown ? KEYEVENT_RAWKEYDOWN : KEYEVENT_KEYUP;

    this->cefInstance->GetHost()->SendKeyEvent(keyEvent);
#endif
#endif
}

Color3 ChromiumFrame::getColorAtPixel(int x, int y)
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::GetColorAtPixel can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot get color at pixel on a non-initialized ChromiumFrame");

    if (x < 0 || x >= this->resolution.x || y < 0 || y >= this->resolution.y)
        throw std::runtime_error("Pixel out of bounds");

#ifndef AYA_SERVER
    int offset = (y * this->resolution.x + x) * 4;
    return Color3(this->buffer[offset + 0] / 255.0f, this->buffer[offset + 1] / 255.0f, this->buffer[offset + 2] / 255.0f);
#endif
#endif
}

void ChromiumFrame::focus()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::Focus can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot focus on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->cefInstance->GetHost()->SetFocus(true);
#endif
#endif
}

void ChromiumFrame::unfocus()
{
#ifndef ENABLE_CHROMIUM_FRAMES
    throw std::runtime_error("ChromiumFrames are not enabled in this build of Aya");
#else
    if (Network::Players::serverIsPresent(this))
        throw std::runtime_error("ChromiumFrame::Unfocus can only be called from the client");

    if (!this->initialized)
        throw std::runtime_error("Cannot unfocus on a non-initialized ChromiumFrame");

#ifndef AYA_SERVER
    this->cefInstance->GetHost()->SetFocus(false);
#endif
#endif
}

void ChromiumFrame::setZoom(float value)
{
    if (this->zoom == value)
        return;

    if (value < -10 || value > 10)
        throw std::runtime_error("Zoom level must be between -10 and 10");

    this->zoom = value;
    raisePropertyChanged(prop_zoom);

#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    if (this->initialized)
        this->cefInstance->GetHost()->SetZoomLevel(value);
#endif
}

void ChromiumFrame::setZoomInternal(float value)
{
    if (this->zoom == value)
        return;

    this->zoom = value;
    raisePropertyChanged(prop_zoom);
}

void ChromiumFrame::setVolume(float value)
{
    value = G3D::clamp(value, 0.0f, 1.0f);

    if (this->volume == value)
        return;

    this->volume = value;
    if (this->fmod_channel)
        this->fmod_channel->setVolume(this->volume);

    raisePropertyChanged(prop_volume);
}

void ChromiumFrame::setPitch(float value)
{
    if (value < 0.0f)
    {
        value = 0.0f;
    }

    if (this->pitch == value)
        return;

    this->pitch = value;
    if (this->fmod_channel)
        this->fmod_channel->setPitch(this->pitch);

    raisePropertyChanged(prop_pitch);
}

void ChromiumFrame::setIsPlayingInternal(bool value)
{
    if (this->playing == value)
        return;

    this->playing = value;
    raisePropertyChanged(prop_isPlaying);
}

void ChromiumFrame::setWorldAudioEnabled(bool value)
{
    if (this->worldAudioEnabled == value)
        return;

    this->worldAudioEnabled = value;
    raisePropertyChanged(prop_worldAudioEnabled);

    this->updateWorldAudio();
}

void ChromiumFrame::updateWorldAudio()
{
    if (!this->worldAudioEnabled || !this->initialized || !this->fmod_channel || !this->part)
        return;

    fmod_channel->set3DMinMaxDistance(this->worldAudioMinDistance, this->worldAudioMaxDistance);
    if (this->worldAudioRollOff == Aya::Soundscape::RollOffMode::Inverse)
        fmod_channel->setMode(FMOD_3D_INVERSEROLLOFF);
    else if (this->worldAudioRollOff == Aya::Soundscape::RollOffMode::Linear)
        fmod_channel->setMode(FMOD_3D_LINEARROLLOFF);
    else if (this->worldAudioRollOff == Aya::Soundscape::RollOffMode::LinearSquare)
        fmod_channel->setMode(FMOD_3D_LINEARSQUAREROLLOFF);
    else if (this->worldAudioRollOff == Aya::Soundscape::RollOffMode::InverseTapered)
        fmod_channel->setMode(FMOD_3D_INVERSETAPEREDROLLOFF);

    FMOD_VECTOR pos;
    if (Soundscape::SoundService::convert(part->getCoordinateFrame().translation, pos))
    {
        FMOD_VECTOR vel;

        if (Soundscape::SoundService::convert(part->getLinearVelocity(), vel))
            fmod_channel->set3DAttributes(&pos, &vel);
    }
}

void ChromiumFrame::setWorldAudioMinDistance(float value)
{
    if (this->worldAudioMinDistance == value)
        return;

    if (value < 0.0f)
        value = 0.0f;

    this->worldAudioMinDistance = value;
    raisePropertyChanged(prop_worldAudioMinDistance);
}

void ChromiumFrame::setWorldAudioMaxDistance(float value)
{
    if (this->worldAudioMaxDistance == value)
        return;

    if (value < 0.0f)
        value = 0.0f;

    this->worldAudioMaxDistance = value;
    raisePropertyChanged(prop_worldAudioMaxDistance);
}

void ChromiumFrame::setWorldAudioRollOff(Soundscape::RollOffMode value)
{
    if (this->worldAudioRollOff == value)
        return;

    this->worldAudioRollOff = value;
    raisePropertyChanged(prop_worldAudioRollOff);
}

FMOD_RESULT F_CALLBACK ChromiumFrame::AudioCallback(FMOD_SOUND* _sound, void* _data, unsigned int datalen)
{
    AYAPROFILER_SCOPE("CefIntegration", "ChromiumFrame::AudioCallback");

    ChromiumFrame* frame;
    FMOD::Sound* sound = (FMOD::Sound*)_sound;
    float* data = (float*)_data;
    sound->getUserData((void**)&frame);

    int bytesToAdd = datalen;
    int bytesRead = 0;

    while (bytesToAdd > 0)
    {
        if (frame->pendingSoundData() > 0)
        {
            ChromiumFrame::ChromiumFrameAudioData audioPacket = frame->popSoundData();
            if (audioPacket.second && audioPacket.first > 0)
            {
                int bytesToCopy = std::min(audioPacket.first, bytesToAdd);
                memcpy((char*)data + bytesRead, audioPacket.second, bytesToCopy);
                bytesToAdd -= bytesToCopy;
                bytesRead += bytesToCopy;

                // Clean up the allocated memory
                delete[] audioPacket.second;
            }
        }
        else
        {
            // No more data available, fill the rest with silence
            memset((char*)data + bytesRead, 0, bytesToAdd);
            break;
        }
    }

    return FMOD_OK;
}

void ChromiumFrame::startSound(int channels, int sampleRate, int framesPerBuffer)
{
    Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(this);
    FMOD::System* system = soundService->getSystem();

    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1; // could set channels but might mess up world audio
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = sampleRate;
    exinfo.length = framesPerBuffer * sizeof(float) * 10; // Buffer size for PCM data
    exinfo.pcmreadcallback = ChromiumFrame::AudioCallback;
    exinfo.decodebuffersize = framesPerBuffer;
    exinfo.userdata = this;

    system->createSound(NULL, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM | FMOD_OPENUSER, &exinfo, &fmod_sound);
    system->playSound(fmod_sound, NULL, false, &fmod_channel);

    fmod_sound->setUserData(this);
    fmod_channel->setUserData(this);
    fmod_channel->setVolume(this->volume);
    fmod_channel->setPitch(this->pitch);
    fmod_channel->setChannelGroup(soundService->getMasterChannel());

    updateWorldAudio();

    // ready to play
    fmod_channel->setPaused(false);
    setIsPlayingInternal(true);
}

void ChromiumFrame::pushSoundData(const float** data, int frames, int64_t pts)
{
    // Convert from CEF's planar float format to interleaved float format
    // data is an array of channel pointers (e.g., data[0] = left channel, data[1] = right channel)
    // We need to allocate memory for the PCM data and store it

    int bytesPerSample = sizeof(float);
    int totalBytes = frames * bytesPerSample;

    float* pcmData = new float[frames];

    // For now, assume mono or take first channel if multi-channel
    // CEF provides planar audio, so data[0] is the first channel
    memcpy(pcmData, data[0], totalBytes);

    std::lock_guard<std::mutex> lock(audioMutex);
    audioData.push_back(ChromiumFrameAudioData(totalBytes, pcmData));
}

ChromiumFrame::ChromiumFrameAudioData ChromiumFrame::popSoundData()
{
    std::lock_guard<std::mutex> lock(audioMutex);
    if (audioData.empty())
        return ChromiumFrameAudioData(0, nullptr);

    ChromiumFrameAudioData data = audioData.front();
    audioData.pop_front();
    return data;
}


void ChromiumFrame::stopSound()
{
    if (fmod_sound)
    {
        fmod_sound->release();
    }

    if (fmod_channel)
    {
        fmod_channel->stop();
        fmod_channel->setUserData(nullptr);
    }

    // Clean up any remaining audio data
    std::lock_guard<std::mutex> lock(audioMutex);
    while (!audioData.empty())
    {
        ChromiumFrameAudioData data = audioData.front();
        audioData.pop_front();
        if (data.second)
            delete[] data.second;
    }

    setIsPlayingInternal(false);
}

} // namespace Aya