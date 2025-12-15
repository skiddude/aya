#ifdef ENABLE_VOICE_CHAT
#pragma once

#include "Tree/Instance.hpp"
#include <opus/opus.h>
#include <deque>
#include <mutex>
#include <vector>

namespace FMOD
{
class Channel;
class Sound;
} // namespace FMOD

namespace Aya
{

class PartInstance;

extern const char* const sVoiceChatOutput;

// received from Players
typedef std::pair<int, unsigned char*> VoiceChatOpusData;

// Jitter buffer entry
struct VoicePacket
{
    std::vector<opus_int16> samples;
    int sampleCount;

    VoicePacket()
        : sampleCount(0)
    {
    }
    VoicePacket(const opus_int16* data, int count)
        : samples(data, data + count)
        , sampleCount(count)
    {
    }
};

class VoiceChatOutput : public DescribedCreatable<VoiceChatOutput, Instance, sVoiceChatOutput>
{
    typedef DescribedCreatable<VoiceChatOutput, Instance, sVoiceChatOutput> Super;

    FMOD::Channel* fmodChannel;
    FMOD::Sound* fmodSound;
    OpusDecoder* opusDecoder;
    PartInstance* part;

    // Jitter buffer to smooth out network delivery
    std::deque<VoicePacket> jitterBuffer;
    std::mutex bufferMutex;

    // Continuous ring buffer for FMOD callback
    std::vector<opus_int16> ringBuffer;
    size_t ringBufferReadPos;
    size_t ringBufferWritePos;
    size_t ringBufferSize;

    bool initialized;
    unsigned int lastDecodeTime;
    int consecutiveUnderflows;

    float loudness;

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
    /*override*/ void onAncestorChanged(const AncestorChanged& event);

public:
    VoiceChatOutput();
    ~VoiceChatOutput();

    bool isInitialized()
    {
        return initialized;
    }
    void load(const Aya::Instance* context);

    void update3D();
    void setLoudness(float value);
    float getLoudness() const
    {
        return loudness;
    }

    int dataPending();
    bool fillAudioBuffer(opus_int16* output, int samplesNeeded);
    void addData(VoiceChatOpusData data);

private:
    void refillRingBuffer();
};
} // namespace Aya
#endif