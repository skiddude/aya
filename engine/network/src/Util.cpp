#include "Util.hpp"
#include "World/ContactManagerSpatialHash.hpp"

#include <boost/static_assert.hpp>

namespace Aya
{
namespace Network
{

const Aya::SystemAddress RakNetToRbxAddress(const RakNet::SystemAddress& raknetAddress)
{
    return Aya::SystemAddress(raknetAddress.GetBinaryAddress(), raknetAddress.GetPort());
}

std::string RakNetAddressToString(const RakNet::SystemAddress& raknetAddress, bool writePort, char portDelineator)
{
    char buffer[30];
    raknetAddress.ToString(writePort, buffer, portDelineator);
    return buffer;
}

} // namespace Network
} // namespace Aya
