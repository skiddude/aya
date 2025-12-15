#ifdef ENABLE_VOICE_CHAT
#include "VoiceChatOutput.hpp"
#include "Tree/Service.hpp"
#include "Utility/VoiceChatOutput.hpp"
#include "Utility/SoundService.hpp"
#include "StandardOut.hpp"

#include "fmod.h"
#include "fmod.hpp"
#include "fmod_errors.h"

#include <algorithm>
#include <cstring>
#include <cmath>

namespace Aya
{
const char* const sVoiceChatOutput = "VoiceChatOutput";

static Reflection::PropDescriptor<VoiceChatOutput, float> prop_loudness(
    "Loudness", "Audio", &VoiceChatOutput::getLoudness, NULL, Reflection::PropertyDescriptor::NO_XML_WRITE);

// Configuration constants
static const size_t JITTER_BUFFER_TARGET = 3;                  // Target 3 packets (60ms) in jitter buffer
static const size_t JITTER_BUFFER_MAX = 10;                    // Max 10 packets (200ms) before dropping
static const size_t RING_BUFFER_SIZE = OPUS_VOICE_BUFSIZE * 8; // 160ms ring buffer

VoiceChatOutput::VoiceChatOutput()
    : fmodSound(nullptr)
    , fmodChannel(nullptr)
    , opusDecoder(nullptr)
    , ringBufferReadPos(0)
    , ringBufferWritePos(0)
    , ringBufferSize(RING_BUFFER_SIZE)
    , initialized(false)
    , loudness(0.0f)
    , lastDecodeTime(0)
    , consecutiveUnderflows(0)
    , DescribedCreatable<VoiceChatOutput, Instance, sVoiceChatOutput>()
{

    ringBuffer.resize(RING_BUFFER_SIZE, 0);
}

VoiceChatOutput::~VoiceChatOutput()
{
    if (opusDecoder)
    {
        opus_decoder_destroy(opusDecoder);
        opusDecoder = nullptr;
    }
    if (fmodSound)
    {
        fmodSound->release();
        fmodSound = nullptr;
    }
}

FMOD_RESULT F_CALLBACK __voiceChatCallback(FMOD_SOUND* _sound, void* _data, unsigned int datalen)
{
    VoiceChatOutput* src;
    FMOD::Sound* sound = (FMOD::Sound*)_sound;
    opus_int16* data = (opus_int16*)_data;
    sound->getUserData((void**)&src);

    int samplesNeeded = datalen / sizeof(opus_int16);

    bool filled = src->fillAudioBuffer(data, samplesNeeded);

    float loud = 0.0f;
    if (!filled)
    {
        // Buffer underrun - fill with silence
        memset(data, 0, datalen);
        loud = 0.0f;
    }
    else if (samplesNeeded > 0)
    {
        // Compute RMS loudness of the audio buffer and normalize to [0,1]
        unsigned long long sum = 0;
        for (int i = 0; i < samplesNeeded; ++i)
        {
            int s = data[i];
            sum += (unsigned long long)(s) * (unsigned long long)(s);
        }
        double mean = static_cast<double>(sum) / static_cast<double>(samplesNeeded);
        double rms = std::sqrt(mean);
        // 16-bit signed max is 32767
        loud = static_cast<float>(rms / 32767.0);
        if (loud < 0.0f)
            loud = 0.0f;
        else if (loud > 1.0f)
            loud = 1.0f;
    }

    // Update loudness so UI/indicators can read it
    src->setLoudness(loud);

    return FMOD_OK;
}

void VoiceChatOutput::load(const Aya::Instance* context)
{
    if (initialized)
        return;

    Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(context);
    if (!soundService)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Failed to get SoundService");
        return;
    }

    FMOD::System* system = soundService->getSystem();
    if (!system)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Failed to get FMOD System");
        return;
    }

    int err;
    opusDecoder = opus_decoder_create(OPUS_VOICE_SAMPLERATE, OPUS_VOICE_CHANNELS, &err);
    if (err != OPUS_OK)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Failed to create Opus decoder: %d", err);
        return;
    }

    // Configure decoder for better quality
    opus_decoder_ctl(opusDecoder, OPUS_SET_GAIN(0));

    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = OPUS_VOICE_CHANNELS;
    exinfo.format = FMOD_SOUND_FORMAT_PCM16;
    exinfo.defaultfrequency = OPUS_VOICE_SAMPLERATE;
    exinfo.length = OPUS_VOICE_BUFSIZE * 2 * sizeof(opus_int16); // Double buffer for smoother playback
    exinfo.pcmreadcallback = __voiceChatCallback;
    exinfo.userdata = this;

    FMOD_RESULT result = system->createSound(NULL, FMOD_LOOP_NORMAL | FMOD_CREATESTREAM | FMOD_OPENUSER, &exinfo, &fmodSound);
    if (result != FMOD_OK)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Failed to create FMOD sound: %d", result);
        opus_decoder_destroy(opusDecoder);
        opusDecoder = nullptr;
        return;
    }

    fmodSound->setUserData(this);

    result = system->playSound(fmodSound, NULL, false, &fmodChannel);
    if (result != FMOD_OK)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Failed to play sound: %d", result);
        fmodSound->release();
        fmodSound = nullptr;
        opus_decoder_destroy(opusDecoder);
        opusDecoder = nullptr;
        return;
    }

    initialized = true;
    StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "VoiceChatOutput: Initialized successfully");
}

int VoiceChatOutput::dataPending()
{
    std::lock_guard<std::mutex> lock(bufferMutex);
    return jitterBuffer.size();
}

void VoiceChatOutput::refillRingBuffer()
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    // Fill ring buffer from jitter buffer until full or jitter buffer empty
    while (!jitterBuffer.empty())
    {
        VoicePacket& packet = jitterBuffer.front();

        for (int i = 0; i < packet.sampleCount; i++)
        {
            ringBuffer[ringBufferWritePos] = packet.samples[i];
            ringBufferWritePos = (ringBufferWritePos + 1) % ringBufferSize;

            // Check if we're about to overwrite unread data
            if (ringBufferWritePos == ringBufferReadPos)
            {
                // Ring buffer full - stop filling
                return;
            }
        }

        jitterBuffer.pop_front();
    }
}

void VoiceChatOutput::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);

    if (event.child == this)
    {
        part = Instance::fastDynamicCast<PartInstance>(event.newParent);
    }
}

void VoiceChatOutput::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
    {
        Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(oldProvider);
        if (soundService)
            soundService->unregisterVoiceChatOutput(this);
    }

    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        Soundscape::SoundService* soundService = ServiceProvider::create<Soundscape::SoundService>(newProvider);
        soundService->registerVoiceChatOutput(this);
        part = Instance::fastDynamicCast<PartInstance>(this->getParent());
    }
}

bool VoiceChatOutput::fillAudioBuffer(opus_int16* output, int samplesNeeded)
{
    // Refill ring buffer from jitter buffer
    refillRingBuffer();

    // Calculate available samples in ring buffer
    size_t available;
    if (ringBufferWritePos >= ringBufferReadPos)
    {
        available = ringBufferWritePos - ringBufferReadPos;
    }
    else
    {
        available = ringBufferSize - ringBufferReadPos + ringBufferWritePos;
    }

    // If we don't have enough data, handle underflow
    if (available < static_cast<size_t>(samplesNeeded))
    {
        consecutiveUnderflows++;

        if (consecutiveUnderflows > 5)
        {
            // Multiple underflows - wait for buffer to refill
            return false;
        }

        // Do packet loss concealment with Opus decoder if possible
        if (opusDecoder)
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            opus_decode(opusDecoder, nullptr, 0, output, samplesNeeded, 0);
            return true;
        }

        return false;
    }

    consecutiveUnderflows = 0;

    // Read from ring buffer
    for (int i = 0; i < samplesNeeded; i++)
    {
        output[i] = ringBuffer[ringBufferReadPos];
        ringBufferReadPos = (ringBufferReadPos + 1) % ringBufferSize;
    }

    return true;
}

void VoiceChatOutput::addData(VoiceChatOpusData od)
{
    if (!opusDecoder)
    {
        delete[] od.second;
        return;
    }

    // Decode the Opus packet
    opus_int16 decodedSamples[OPUS_VOICE_BUFSIZE * 2]; // Extra space just in case
    int samplesDecoded = opus_decode(opusDecoder, od.second, od.first, decodedSamples, OPUS_VOICE_BUFSIZE, 0);

    delete[] od.second; // Free the input data immediately

    if (samplesDecoded < 0)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "VoiceChatOutput: Opus decode error: %d", samplesDecoded);
        return;
    }

    if (samplesDecoded == 0)
    {
        return;
    }

    // Add to jitter buffer
    {
        std::lock_guard<std::mutex> lock(bufferMutex);

        // Drop old packets if buffer is too full
        while (jitterBuffer.size() >= JITTER_BUFFER_MAX)
        {
            jitterBuffer.pop_front();
        }

        jitterBuffer.emplace_back(decodedSamples, samplesDecoded);
    }
}

void VoiceChatOutput::update3D()
{
    if (fmodChannel && part)
    {
        FMOD_VECTOR pos;
        if (Soundscape::SoundService::convert(part->getCoordinateFrame().translation, pos))
        {
            FMOD_VECTOR vel;

            if (Soundscape::SoundService::convert(part->getLinearVelocity(), vel))
                fmodChannel->set3DAttributes(&pos, &vel);
        }
    }
}

void VoiceChatOutput::setLoudness(float value)
{
    if (loudness != value)
    {
        loudness = value;
        raisePropertyChanged(prop_loudness);
    }
}

} // namespace Aya
#endif