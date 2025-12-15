

#include "DataModel/ServerStorage.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "Players.hpp"

using namespace Aya;

const char* const Aya::sServerStorage = "ServerStorage";

ServerStorage::ServerStorage(void)
{
    setName(sServerStorage);
}

bool ServerStorage::askAddChild(const Instance* instance) const
{
    return Aya::Network::Players::backendProcessing(this) || Aya::GameBasicSettings::singleton().inStudioMode();
}