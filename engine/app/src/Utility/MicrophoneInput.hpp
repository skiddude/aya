#ifdef ENABLE_VOICE_CHAT
#pragma once

#include <portaudio.h>
#include <opus/opus.h>
#include "Utility/SoundService.hpp"

namespace Aya
{
class MicrophoneInput
{
    static double nextSample;
public:
    static void initialize();
    static void shutdown();
    static bool isInitialized();
    static std::tuple<unsigned char*, int> getData();
};
} // namespace Aya
#endif