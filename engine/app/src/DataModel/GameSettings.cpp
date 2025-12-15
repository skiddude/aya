

#include "DataModel/GameSettings.hpp"
#include "DataModel/GameBasicSettings.hpp"

using namespace Aya;
const char* const Aya::sGameSettings = "GameSettings";


Reflection::BoundProp<int> prop_ChatHistory("ChatHistory", "Online", &GameSettings::chatHistory);
Reflection::BoundProp<int> prop_ReportAbuseChatHistory("ReportAbuseChatHistory", "Online", &GameSettings::reportAbuseChatHistory);
Reflection::BoundProp<int> prop_ChatScrollLength("ChatScrollLength", "Online", &GameSettings::chatScrollLength);
Reflection::BoundProp<bool> prop_SoundEnabled("SoundEnabled", "Sound", &GameSettings::soundEnabled);
Reflection::BoundProp<bool> prop_SoftwareSound("SoftwareSound", "Sound", &GameSettings::softwareSound);
Reflection::BoundProp<bool> prop_CollisionSoundEnabled(
    "CollisionSoundEnabled", "Sound", &GameSettings::collisionSoundEnabled, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<float> prop_CollisionSoundVolume(
    "CollisionSoundVolume", "Sound", &GameSettings::collisionSoundVolume, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<int> prop_MaxCollisionSounds(
    "MaxCollisionSounds", "Sound", &GameSettings::maxCollisionSounds, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<int> prop_bubbleChatMaxBubbles("BubbleChatMaxBubbles", "Online", &GameSettings::bubbleChatMaxBubbles);
Reflection::BoundProp<float> prop_bubbleChatLifetime("BubbleChatLifetime", "Online", &GameSettings::bubbleChatLifetime);

#if defined(AYA_PLATFORM_DURANGO)
Reflection::BoundProp<float> prop_overscanPX("OverscanPX", category_Video, &GameSettings::overscanPX);
Reflection::BoundProp<float> prop_overscanPY("OverscanPY", category_Video, &GameSettings::overscanPY);
#endif

Reflection::BoundProp<bool> prop_hardwareMouse("HardwareMouse", "Input", &GameSettings::hardwareMouse);

GameSettings::GameSettings(void)
    : chatHistory(100)
    , reportAbuseChatHistory(50)
    , chatScrollLength(5)
    , soundEnabled(true)
    , collisionSoundEnabled(true)
    , collisionSoundVolume(10)
    , maxCollisionSounds(-1)
    , softwareSound(false)
    , bubbleChatMaxBubbles(3)
    , bubbleChatLifetime(30.0f)
    , hardwareMouse(false)
    , overscanPX(-1)
    , overscanPY(-1)
{
    setName("Game Options");
}
