#ifndef V8XML_SERIALIZER_H
#define V8XML_SERIALIZER_H

#pragma once

#include "SerializerV2.hpp"

#include "Utility/SoundService.hpp"

#include "DataModel/Workspace.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/ServerStorage.hpp"
#include "DataModel/ReplicatedStorage.hpp"
#include "DataModel/ReplicatedFirst.hpp"
#include "DataModel/PlayerGui.hpp"
#include "DataModel/Hopper.hpp"
#include "DataModel/StarterPlayerService.hpp"
#include "DataModel/ServerScriptService.hpp"
#include "DataModel/CSGDictionaryService.hpp"

class Serializer : public SerializerV2
{
public:
    static bool canWriteChild(const shared_ptr<Aya::Instance> instance, Aya::Instance::SaveFilter saveFilter)
    {
        if (!instance->getIsArchivable())
            return false;

        switch (saveFilter)
        {
        case Aya::Instance::SAVE_ALL:
            return true;

        case Aya::Instance::SAVE_WORLD:
            if (Aya::Instance::fastDynamicCast<Aya::Workspace>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::Lighting>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::Soundscape::SoundService>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::ServerStorage>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::ReplicatedStorage>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::CSGDictionaryService>(instance.get()))
                return true;

            return false;

        case Aya::Instance::SAVE_GAME:
            if (Aya::Instance::fastDynamicCast<Aya::StarterGuiService>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::StarterPackService>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::StarterPlayerService>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::ServerScriptService>(instance.get()))
                return true;
            if (Aya::Instance::fastDynamicCast<Aya::ReplicatedFirst>(instance.get()))
                return true;

            return false;

        default:
            return true;
        }
    }
};



#endif