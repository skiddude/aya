#ifdef ENABLE_VOICE_CHAT
#include "MicrophoneInput.hpp"
#include "portaudio.h"
#include "StandardOut.hpp"
#include "time.hpp"

namespace Aya
{

PaStream* stream;
bool initialized = false;
OpusEncoder* encoder;

void MicrophoneInput::initialize()
{
    // Open microphone input using PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Error initializing PortAudio (%d): '%s'", err, Pa_GetErrorText(err));
        return;
    }

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "PortAudio initialized");

    // first see if we even have a default input device.
    PaDeviceIndex device = Pa_GetDefaultInputDevice();
    if (device == paNoDevice)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "No default input device found when initializing PortAudio.");
        Pa_Terminate();
        return;
    }

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Default input device: %d", device);

    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
    if (info != NULL)
    {
        Aya::StandardOut::singleton()->printf(
            MESSAGE_SENSITIVE, "Default input device: %s, max input channels: %d", info->name, info->maxInputChannels);
    }

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Opening default PortAudio stream");

    err = Pa_OpenDefaultStream(&stream, OPUS_VOICE_CHANNELS, 0, paInt16, OPUS_VOICE_SAMPLERATE, OPUS_VOICE_BUFSIZE, NULL, NULL);
    if (err != paNoError)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Error opening default PortAudio stream (%d): '%s'", err, Pa_GetErrorText(err));
        Pa_Terminate();
        return;
    }

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Starting default PortAudio stream");

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Error starting PortAudio stream (%d): '%s'", err, Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "PortAudio stream started");

    // Now, create our Opus encoder
    int err_opus;
    encoder = opus_encoder_create(OPUS_VOICE_SAMPLERATE, OPUS_VOICE_CHANNELS, OPUS_APPLICATION_VOIP, &err_opus);

    if (err_opus != OPUS_OK)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Error creating Opus encoder: %d", err_opus);
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }

    // Configure encoder for better voice quality
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(24000)); // 24kbps is good for voice
    opus_encoder_ctl(encoder, OPUS_SET_VBR(1));         // Enable variable bitrate
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));  // Medium complexity
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(encoder, OPUS_SET_DTX(0));        // Disable discontinuous transmission for now
    opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1)); // Enable forward error correction

    Aya::StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Opus encoder created and configured");

    initialized = true;
}

void MicrophoneInput::shutdown()
{
    if (initialized)
    {
        // shutdown mic input
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();

        // destroy opus encoder
        opus_encoder_destroy(encoder);

        initialized = false;
    }
}

std::tuple<unsigned char*, int> MicrophoneInput::getData()
{
    if (!initialized)
        return std::make_tuple(nullptr, 0);

    size_t streamAvailable = Pa_GetStreamReadAvailable(stream);
    if (streamAvailable < OPUS_VOICE_BUFSIZE) // Need at least one frame worth of samples
        return std::make_tuple(nullptr, 0);

    opus_int16* streamData = new opus_int16[OPUS_VOICE_BUFSIZE];
    PaError err = Pa_ReadStream(stream, streamData, OPUS_VOICE_BUFSIZE);

    if (err != paNoError)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Error reading from PortAudio stream (%d): '%s'", err, Pa_GetErrorText(err));
        delete[] streamData;
        return std::make_tuple(nullptr, 0);
    }

    // Allocate buffer for encoded data (max size for one frame)
    unsigned char* opusData = new unsigned char[4000];
    int bytes = opus_encode(encoder, streamData, OPUS_VOICE_BUFSIZE, opusData, 4000);

    delete[] streamData;

    if (bytes < 0)
    {
        Aya::StandardOut::singleton()->printf(MESSAGE_ERROR, "Unable to encode voice packet (%i) (error: '%s')", bytes, opus_strerror(bytes));
        delete[] opusData;
        return std::make_tuple(nullptr, 0);
    }

    if (bytes == 0)
    {
        delete[] opusData;
        return std::make_tuple(nullptr, 0);
    }

    // Reallocate to exact size to save bandwidth
    unsigned char* finalData = new unsigned char[bytes];
    memcpy(finalData, opusData, bytes);
    delete[] opusData;

    return std::make_tuple(finalData, bytes);
}

bool MicrophoneInput::isInitialized()
{
    return initialized;
}

} // namespace Aya
#endif