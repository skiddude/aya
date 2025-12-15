

#include "DataModel/RobloxReplicatedStorage.hpp"

using namespace Aya;

const char* const Aya::sRobloxReplicatedStorage = "RobloxReplicatedStorage";

RobloxReplicatedStorage::RobloxReplicatedStorage(void)
{
    setName(sRobloxReplicatedStorage);
    setRobloxLocked(true);
}
