#pragma once

#include "DataModel/GuiObject.hpp"
#include "DataModel/GuiMixin.hpp"
#include "Utility/ContentId.hpp"

namespace Aya
{

extern const char* const sVideoFrame;

class VideoFrame
    : public DescribedCreatable<VideoFrame, GuiLabel, sVideoFrame>
    , public GuiImageMixin
{
public:
    VideoFrame();
    ~VideoFrame();

    void initialize();
    void uninitialize();
    void play();
    void pause();

    void setVolume(const float& volume);
    float getVolume() const
    {
        return playbackVolume;
    }

    void setVideo(const ContentId& video);
    ContentId getVideo() const
    {
        return video;
    }

    void setTimePosition(const double& timePosition);
    double getTimePosition() const
    {
        return currentTime;
    }

    void setPlaying(const bool& playing);
    bool getPlaying() const
    {
        return playing;
    }

    void setLooped(const bool& looped);
    bool getLooped() const
    {
        return false;
    }

    void setSpeed(const double& speed);
    double getSpeed() const
    {
        return playbackSpeed;
    }

    bool getIsLoaded() const
    {
        return initialized;
    }

    Vector2 getResolution() const
    {
        return sourceResolution;
    }

    double getTimeLength() const
    {
        return length;
    }

    Aya::remote_signal<void(ContentId)> pausedSignal;
    Aya::remote_signal<void(ContentId)> playedSignal;
    Aya::remote_signal<void(ContentId)> didLoopSignal;
    Aya::remote_signal<void(ContentId)> loadedSignal;
    Aya::remote_signal<void(ContentId)> endedSignal;

private:
    shared_ptr<Graphics::Texture> texture;

    bool initialized;
    bool playing;
    bool looped;
    Vector2 sourceResolution;
    double length;
    double currentTime;
    ContentId video;
    float playbackVolume;
    float playbackSpeed;
    /*override*/ void onAncestorChanged(const AncestorChanged& event);

    //
    // IAdornable
    //

    /*override*/ void render2d(Adorn* adorn);
    /*override*/ void renderBackground2d(Adorn* adorn);
};

} // namespace Aya