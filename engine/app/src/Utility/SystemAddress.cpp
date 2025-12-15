

#include "Utility/SystemAddress.hpp"

namespace Aya
{

bool SystemAddress::operator==(const Aya::SystemAddress& right) const
{
    return binaryAddress == right.binaryAddress && port == right.port;
}

bool SystemAddress::operator!=(const Aya::SystemAddress& right) const
{
    return binaryAddress != right.binaryAddress || port != right.port;
}

bool SystemAddress::operator>(const Aya::SystemAddress& right) const
{
    return ((binaryAddress > right.binaryAddress) || ((binaryAddress == right.binaryAddress) && (port > right.port)));
}

bool SystemAddress::operator<(const Aya::SystemAddress& right) const
{
    return ((binaryAddress < right.binaryAddress) || ((binaryAddress == right.binaryAddress) && (port < right.port)));
}

} // namespace Aya
