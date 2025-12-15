

#include "Utility/DiscordIntegration.hpp"

namespace Aya
{
#ifdef AYA_TEST_BUILD
std::int64_t DiscordIntegration::discordAppId = 1212279497542606858;
#else
std::int64_t DiscordIntegration::discordAppId = 1212279202544484392;
#endif
bool DiscordIntegration::initialized = false;
bool DiscordIntegration::detailsSet = false;
bool DiscordIntegration::partyIdSet = false;
bool DiscordIntegration::dataModelSet = false;

std::string DiscordIntegration::details;
const char* DiscordIntegration::partyId;
std::int64_t DiscordIntegration::partySize;
std::int64_t DiscordIntegration::partyMax;
DataModel* DiscordIntegration::dm;
std::time_t DiscordIntegration::start = std::time(0);

void DiscordIntegration::initialize()
{
    if (initialized)
        return;

    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(std::to_string(discordAppId).c_str(), &handlers, 0, NULL);

    // This is really unsafe but works anyway
    initialized = true;
    updateActivity();
}

void DiscordIntegration::updateActivity()
{
    if (!initialized)
        return;

    DiscordRichPresence presence;
    memset(&presence, 0, sizeof(presence));

    presence.state = "In game";
    presence.largeImageKey = "logo";
    presence.largeImageText = AYA_PROJECT_NAME;
    presence.startTimestamp = start;
    presence.instance = 1;

    if (detailsSet)
        presence.details = details.c_str();

    if (partyIdSet)
    {
        presence.partyId = partyId;
        presence.partySize = partySize;
        presence.partyMax = partyMax;
    }

    Discord_UpdatePresence(&presence);
}

void DiscordIntegration::setDetails(std::string details)
{
    if (!detailsSet)
        detailsSet = true;
    else
        return; // Only set the details once, since this is called by MarketplaceService:LoadAssets() when place info is first loaded during
                // LoadingScript

    initialize();

    DiscordIntegration::details = details;

    updateActivity();
}

void DiscordIntegration::setDataModel(DataModel* dm)
{
    if (!dataModelSet)
        dataModelSet = true;
    else
        return; // Only set the data model once, since this is called by MarketplaceService:LoadAssets() when place info is first loaded during
                // LoadingScript

    DiscordIntegration::dm = dm;

    // Run new thread that refreshes partySize with the number of players in the game + the max players every minute
    boost::thread(
        [&]()
        {
            while (true)
            {
                boost::this_thread::sleep_for(boost::chrono::minutes(1));

                if (partyIdSet && initialized && dataModelSet)
                {
                    setPartySize(dm->getNumPlayers(), dm->getMaxPlayers());
                }
            }
        });
}

void DiscordIntegration::setPartySize(std::int64_t currentSize, std::int64_t maxSize)
{
    if (!partyIdSet)
    {
        partyId = (std::string("Aya-" + boost::uuids::to_string(boost::uuids::random_generator()())).c_str());
        partyIdSet = true;
    }

    partySize = currentSize;
    partyMax = maxSize;

    updateActivity();
}

void DiscordIntegration::shutdown()
{
    if (!initialized)
        return;

    Discord_ClearPresence();
    Discord_Shutdown();

    initialized = false;
    dm = nullptr;
}
} // namespace Aya