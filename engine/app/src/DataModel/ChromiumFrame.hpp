#pragma once

#include <deque>
#include <vector>
#include <mutex>

#include "DataModel/GuiObject.hpp"
#include "DataModel/GuiMixin.hpp"
#include "Utility/Sound.hpp"
#include "Utility/SoundChannel.hpp"
#include "fmod_common.h"

#ifdef ENABLE_CHROMIUM_FRAMES
#include <include/cef_app.h>
#include <include/cef_base.h>
#endif

namespace Aya
{

class PartInstance;

enum ChromiumConsoleMessageSeverity
{
    Default,
    Verbose,
    Info,
    Warning,
    Error,
    Fatal
};

enum ChromiumLoadState
{
    Stopped,
    Loaded,
    Loading,
    LoadError
};

enum ChromiumMouseCursor
{
    Pointer,
    Cross,
    Hand,
    IBeam,
    Wait,
    Help,
    EastResize,
    NorthResize,
    NortheastResize,
    NorthwestResize,
    SouthResize,
    SoutheastResize,
    SouthwestResize,
    WestResize,
    NorthSouthResize,
    EastWestResize,
    NortheastSouthwestResize,
    NorthwestSoutheastResize,
    ColumnResize,
    RowResize,
    MiddlePanning,
    EastPanning,
    NorthPanning,
    NortheastPanning,
    NorthwestPanning,
    SouthPanning,
    SoutheastPanning,
    SouthwestPanning,
    WestPanning,
    Move,
    VerticalText,
    Cell,
    ContextMenu,
    Alias,
    Progress,
    NoDrop,
    Copy,
    None,
    NotAllowed,
    ZoomIn,
    ZoomOut,
    Grab,
    Grabbing,
    MiddlePanningVertical,
    MiddlePanningHorizontal,
    Custom,
    DndNone,
    DndMove,
    DndCopy,
    DndLink
};

// fwd decl
class AyaCefRenderer;
class AyaCefClient;

extern const char* const sChromiumFrame;

class ChromiumFrame
    : public DescribedCreatable<ChromiumFrame, GuiLabel, sChromiumFrame>
    , public GuiImageMixin
{
    typedef DescribedCreatable<ChromiumFrame, GuiLabel, sChromiumFrame> Super;

public:
    ChromiumFrame();
    ~ChromiumFrame();

    void initialize();
    void uninitialize();

    void mouseLeftClick(int x, int y, bool isDown = true);
    void mouseMiddleClick(int x, int y, bool isDown = true);
    void mouseRightClick(int x, int y, bool isDown = true);
    void mouseWheel(int x, int y, int deltaX, int deltaY);
    void mouseMove(int x, int y);
    void keyEvent(int keyCode, int modifiers, bool isKeyDown = true);

    std::string runJS(std::string code);

    void refresh();
    void refreshNoCache();
    void backward();
    void forward();
    void cancel();
    void focus();
    void unfocus();
    Color3 getColorAtPixel(int x, int y);

    bool getIsLoaded() const
    {
        return (loadState == ChromiumLoadState::Loaded);
    }
    bool getCanPageBackward() const
    {
        return canPageBackward;
    }
    void setCanPageBackward(bool value);
    bool getCanPageForward() const
    {
        return canPageForward;
    }
    void setCanPageForward(bool value);

    bool getInitialized() const
    {
        return initialized;
    }
    void setInitializedInternal(bool value);

    std::string getPageTitle() const
    {
        return pageTitle;
    }
    void setPageTitleInternal(std::string value);

    int getStatusCode() const
    {
        return statusCode;
    }
    void setStatusCodeInternal(int value);

    ChromiumLoadState getLoadState() const
    {
        return loadState;
    }
    void setLoadStateInternal(ChromiumLoadState value);

    Vector2 getResolution() const
    {
        return this->resolution;
    }
    void setResolution(Vector2 resolution);

    Vector2 getMousePosition() const
    {
        return this->mousePosition;
    }
    void setMousePosition(Vector2 position);

    std::string getURL() const
    {
        return this->url;
    }
    void setURL(std::string url);
    void setURLInternal(std::string url); // called by AyaCefClient::OnLoadStart

    ChromiumMouseCursor getMouseCursor() const
    {
        return this->mouseCursor;
    }
    void setMouseCursorInternal(ChromiumMouseCursor value);

    Vector2 getScrollOffset() const
    {
        return this->scrollOffset;
    }
    void setScrollOffsetInternal(Vector2 value);

    float getZoom() const
    {
        return this->zoom;
    }
    void setZoom(float value);
    void setZoomInternal(float value);

    float getVolume() const
    {
        return this->volume;
    }
    void setVolume(float value);

    float getPitch() const
    {
        return this->pitch;
    }
    void setPitch(float value);

    bool isPlaying() const
    {
        return this->playing;
    }
    void setIsPlayingInternal(bool value);

    void setWorldAudioEnabled(bool value);
    bool getWorldAudioEnabled() const
    {
        return this->worldAudioEnabled;
    }

    void updateWorldAudio();

    float getWorldAudioMinDistance() const
    {
        return this->worldAudioMinDistance;
    }
    void setWorldAudioMinDistance(float value);

    float getWorldAudioMaxDistance() const
    {
        return this->worldAudioMaxDistance;
    }
    void setWorldAudioMaxDistance(float value);

    Soundscape::RollOffMode getWorldAudioRollOff() const
    {
        return this->worldAudioRollOff;
    }
    void setWorldAudioRollOff(Soundscape::RollOffMode value);

    typedef std::pair<int, float*> ChromiumFrameAudioData;

    std::deque<ChromiumFrameAudioData> audioData;
    std::mutex audioMutex;

    void startSound(int channels, int sampleRate, int framesPerBuffer);
    void pushSoundData(const float** data, int frames, int64_t pts);
    ChromiumFrameAudioData popSoundData();
    int pendingSoundData()
    {
        return audioData.size();
    };
    void stopSound();

    // These are public so that AyaCefRenderer::OnPaint may modify them
    bool shouldReuploadTexture = false;
    std::vector<char> buffer;

    Aya::remote_signal<void(bool)> isLoadedSignal;
    Aya::remote_signal<void(ChromiumMouseCursor)> mouseCursorChangedSignal;
    Aya::remote_signal<void(std::string, ChromiumConsoleMessageSeverity, std::string, int)> onConsoleMessageSignal;
    Aya::remote_signal<void(Vector2)> scrollOffsetChangedSignal;
    static Reflection::RemoteEventDesc<ChromiumFrame, void(std::string, ChromiumConsoleMessageSeverity, std::string, int)>
        event_ChromiumFrameConsoleMessage;

#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    // These are public so that the cefLoop may set them upon initialization
    CefRefPtr<CefBrowser> cefInstance;
    CefRefPtr<AyaCefRenderer> cefRenderer;
    CefRefPtr<AyaCefClient> cefClient;
#endif

    int lastPaintWidth;
    int lastPaintHeight;

private:
#if defined(ENABLE_CHROMIUM_FRAMES) && !defined(AYA_SERVER)
    void mouseClick(int x, int y, cef_mouse_button_type_t button, bool isDown = true);
#endif

    static FMOD_RESULT F_CALLBACK AudioCallback(FMOD_SOUND* _sound, void* _data, unsigned int datalen);

    std::string url;
    std::string pageTitle;

    bool initialized;
    bool canPageBackward = false;
    bool canPageForward = false;
    bool worldAudioEnabled;
    bool playing = false;

    int statusCode;
    float volume;
    float pitch;
    float worldAudioMinDistance;
    float worldAudioMaxDistance;
    Soundscape::RollOffMode worldAudioRollOff;
    float zoom = 0.0f;

    Vector2 resolution;
    Vector2 mousePosition;
    Vector2 scrollOffset;

    FMOD::Sound* fmod_sound;
    FMOD::Channel* fmod_channel; // the latest channel
    PartInstance* part;          // The Part (if any) that this sound is attached to

    ChromiumLoadState loadState;
    ChromiumMouseCursor mouseCursor;

    shared_ptr<Graphics::Texture> texture;

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
    /*override*/ void onAncestorChanged(const AncestorChanged& event);

    //
    // IAdornable
    //

    /*override*/ void render2d(Adorn* adorn);
    /*override*/ void renderBackground2d(Adorn* adorn);
};

} // namespace Aya