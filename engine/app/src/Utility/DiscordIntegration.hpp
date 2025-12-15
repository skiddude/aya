#pragma once

#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/DataModel.hpp"

#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <discord_rpc.h>
#include <discord_register.h>
#include <ctime>
#include <rapidjson/document.h>

namespace Aya
{
class DiscordIntegration
{
private:
    static std::int64_t discordAppId;
    static bool initialized;
    static bool detailsSet;
    static bool partyIdSet;
    static bool dataModelSet;

    static DataModel* dm;
    static std::string details;
    static const char* partyId;
    static std::int64_t partySize;
    static std::int64_t partyMax;
    static std::time_t start;

    void updateActivity();

public:
    void initialize();
    void shutdown();
    void setDetails(std::string details);
    void setPartySize(std::int64_t currentSize, std::int64_t maxSize);
    void setDataModel(DataModel* dm);

    bool hasSetDetails()
    {
        return detailsSet;
    }
    bool hasSetDataModel()
    {
        return dataModelSet;
    }
};
} // namespace Aya