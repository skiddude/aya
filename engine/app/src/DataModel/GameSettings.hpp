#pragma once

#include "DataModel/GlobalSettings.hpp"

namespace Aya
{
extern const char* const sGameSettings;
class GameSettings : public GlobalAdvancedSettingsItem<GameSettings, sGameSettings>
{
public:
    enum ChatMode
    {
        CHAT_AUTO = 0,
        CHAT_CLASSIC = 1,
        CHAT_BUBBLE = 2,
        CHAT_BOTH = 3
    };

    GameSettings();
    int chatHistory;
    int reportAbuseChatHistory;
    int chatScrollLength;
    bool soundEnabled;
    bool softwareSound;

    bool collisionSoundEnabled;
    float collisionSoundVolume;
    int maxCollisionSounds;

    int bubbleChatMaxBubbles;
    float bubbleChatLifetime;

    float overscanPX;
    float overscanPY;


    bool hardwareMouse;
};
} // namespace Aya
