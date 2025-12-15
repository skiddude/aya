
#pragma once

#include "Tree/Instance.hpp"
#include "Utility/ContentId.hpp"
#include "DataModel/KeyframeSequence.hpp"

namespace Aya
{
extern const char* const sAnimationTrackState;

class KeyframeSequence;
class Animator;
class AnimationTrack;
struct PoseAccumulator;

class AnimationTrackState : public DescribedNonCreatable<AnimationTrackState, Instance, sAnimationTrackState>
{
protected:
    // Either set directly for solo/debugging or replicated by the AssetId
    shared_ptr<const KeyframeSequence> keyframeSequence;

    shared_ptr<AnimationTrack> animationTrack;

    int preKeyframe;

    // Keep a weak ptr to our parent
    weak_ptr<const Animator> animator;

    double startTime; // reset by the "play" function.
    double speed;
    double phase; // speed-independent adjustment to keyframetime.
    double fadeStartTime;
    double fadeStartWeight;
    double fadeEndTime;
    double fadeEndWeight;

    bool isPlaying;

    double getGameTime(); // helper

    void onPlay(float gameTime, float fadeTime, float weight, float speed);
    void onStop(float gameTime, float fadeTime);
    void onAdjustWeight(float gameTime, float weight, float fadeTime);
    void onAdjustSpeed(float gameTime, float speed);

    bool inReverse();
    void detectKeyframeReached(double animationTime, double lastAnimationTime);

    KeyframeSequence::Priority priority;
    bool priorityOverridden;

    // helper for step()
    void triggerKeyframeReachedSignal(const shared_ptr<Instance>& child, double minKeyframeTime, double maxKeyframeTime);

protected:
    double lastKeyframeTime; // keep track of last played frame for keyframe events.
public:
    AnimationTrackState(shared_ptr<const KeyframeSequence> keyframeSequence, weak_ptr<const Animator> animator);
    ~AnimationTrackState() {}
    void play(float fadeTime, float weight, float speed);
    void stop(float fadeTime);
    void adjustWeight(float weight, float fadeTime);
    void adjustSpeed(float speed);

    // Should be private
    Aya::remote_signal<void(float, float, float, float)> internalPlaySignal;
    Aya::remote_signal<void(float, float)> internalStopSignal;
    Aya::remote_signal<void(float, float, float)> internalAdjustWeightSignal;
    Aya::remote_signal<void(float, float)> internalAdjustSpeedSignal;
    Aya::remote_signal<void(std::string)> keyframeReachedSignal;
    Aya::remote_signal<void()> stoppedSignal;

    const KeyframeSequence* getKeyframeSequence() const
    {
        return keyframeSequence.get();
    };
    void setKeyframeSequence(shared_ptr<KeyframeSequence>);

    double getWeightAtTime(double time);
    double getKeyframeAtTime(double time);
    void setKeyframeAtTime(double gameTime, double keyframeTime); // adjust phase to get animation on a specific keyframe.

    double getSpeed()
    {
        return speed;
    }
    float getDuration();

    bool isStopped(double time);
    bool getIsPlaying() const
    {
        return isPlaying;
    };

    double getDurationClampedKeyframeTime(double keyframeTime);

    KeyframeSequence::Priority getPriority() const;
    void setPriority(KeyframeSequence::Priority priority);

    void resetKeyframeReachedDetection(double keyframeTime);

    void step(std::vector<PoseAccumulator>& jointposes, double time);

    void setAnimationTrack(shared_ptr<AnimationTrack> animationTrackIn);
    shared_ptr<AnimationTrack> getAnimationTrack() const
    {
        return animationTrack;
    };
};

} // namespace Aya
