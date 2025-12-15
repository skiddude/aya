#include "DataModel/VideoFrame.hpp"
#include "DataModel/GuiLayerCollector.hpp"
#include "Utility/ContentId.hpp"

namespace Aya
{

const char* const sVideoFrame = "VideoFrame";

static Reflection::BoundFuncDesc<VideoFrame, void()> func_pause(&VideoFrame::pause, "Pause", Security::None);
static Reflection::BoundFuncDesc<VideoFrame, void()> func_play(&VideoFrame::play, "Play", Security::None);

static Reflection::PropDescriptor<VideoFrame, float> prop_volume(
    "Volume", "Video", &VideoFrame::getVolume, &VideoFrame::setVolume, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<VideoFrame, ContentId> prop_video(
    "Video", "Video", &VideoFrame::getVideo, &VideoFrame::setVideo, Reflection::PropertyDescriptor::STANDARD);
static static Reflection::PropDescriptor<VideoFrame, double> prop_timePosition(
    "TimePosition", "Video", &VideoFrame::getTimePosition, &VideoFrame::setTimePosition, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<VideoFrame, bool> prop_playing(
    "Playing", "Video", &VideoFrame::getPlaying, &VideoFrame::setPlaying, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<VideoFrame, bool> prop_looped(
    "Looped", "Video", &VideoFrame::getLooped, &VideoFrame::setLooped, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<VideoFrame, bool> prop_isLoaded(
    "IsLoaded", "Video", &VideoFrame::getIsLoaded, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<VideoFrame, Vector2> prop_resolution(
    "Resolution", "Video", &VideoFrame::getResolution, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<VideoFrame, double> prop_timeLength(
    "TimeLength", "Video", &VideoFrame::getTimeLength, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);
static Reflection::PropDescriptor<VideoFrame, double> prop_speed(
    "Speed", "Video", &VideoFrame::getSpeed, &VideoFrame::setSpeed, Reflection::PropertyDescriptor::STANDARD);

static Reflection::RemoteEventDesc<VideoFrame, void(ContentId)> event_paused(
    &VideoFrame::pausedSignal, "Paused", "video", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<VideoFrame, void(ContentId)> event_played(
    &VideoFrame::playedSignal, "Played", "video", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<VideoFrame, void(ContentId)> event_didLoop(&VideoFrame::didLoopSignal, "DidLoop", "video", Security::None,
    Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<VideoFrame, void(ContentId)> event_loaded(
    &VideoFrame::loadedSignal, "Loaded", "video", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<VideoFrame, void(ContentId)> event_ended(
    &VideoFrame::endedSignal, "Ended", "video", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);

VideoFrame::VideoFrame()
    : DescribedCreatable<VideoFrame, GuiLabel, sVideoFrame>("VideoFrame")
    , initialized(false)
    , playing(false)
    , looped(false)
    , sourceResolution(Vector2(0, 0))
    , length(0.0f)
    , currentTime(0.0f)
    , video(ContentId())
    , playbackVolume(1.0f)
    , playbackSpeed(1.0f)
{
}

VideoFrame::~VideoFrame()
{
    if (this->initialized)
        this->uninitialize();
}

void VideoFrame::onAncestorChanged(const AncestorChanged& event)
{
    GuiObject::onAncestorChanged(event);

    // we uninit if it is null and/or if it is not a gui object of importance to us
    // why? because it doesn't make sense to keep playing if it is not being rendered...
    if (this->initialized &&
        !(Aya::Instance::fastDynamicCast<Aya::GuiObject>(event.newParent) || Aya::Instance::fastDynamicCast<Aya::GuiLayerCollector>(event.newParent)))
    {
        this->uninitialize();
    }
}

void VideoFrame::initialize()
{
    if (this->initialized)
        return;

    if (this->playing)
        this->uninitialize();

    if (this->video.isNull())
        return;

    this->initialized = true;
    event_loaded.fireEvent(this, this->video);
}

void VideoFrame::uninitialize()
{
    if (!this->initialized)
        return;

    this->initialized = false;
    event_ended.fireEvent(this, this->video);
}

void VideoFrame::play()
{
    if (!this->initialized)
        return;

    this->playing = true;
    event_played.fireEvent(this, this->video);
}

void VideoFrame::pause()
{
    if (!this->initialized)
        return;

    this->playing = false;
    event_paused.fireEvent(this, this->video);
}

void VideoFrame::setVolume(const float& volume)
{
    this->playbackVolume = volume;
}

void VideoFrame::setVideo(const ContentId& video)
{
    this->video = video;
    this->initialize();
}

void VideoFrame::setLooped(const bool& looped)
{
    this->looped = looped;
}

void VideoFrame::setPlaying(const bool& playing)
{
    if (playing)
        this->play();
    else
        this->pause();
}

void VideoFrame::setSpeed(const double& speed)
{
    this->playbackSpeed = speed;
}

void VideoFrame::setTimePosition(const double& timePosition)
{
    this->currentTime = timePosition;
}

void VideoFrame::render2d(Adorn* adorn) {}

void VideoFrame::renderBackground2d(Adorn* adorn)
{
    if (this->getBackgroundTransparency() < 1.0)
        GuiObject::render2dImpl(adorn, getRenderBackgroundColor4());
}


} // namespace Aya