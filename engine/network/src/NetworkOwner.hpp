

#pragma once

#include "Utility/SystemAddress.hpp"

#include "Utility/Color.hpp"


namespace Aya
{
namespace Network
{

class NetworkOwner
{
public:
    static const Aya::SystemAddress Server()
    {
        static Aya::SystemAddress s(1, 0);
        return s;
    }

    // created on server, have not be properly assigned via network owner job
    static const Aya::SystemAddress ServerUnassigned()
    {
        static Aya::SystemAddress s(1, 1);
        return s;
    }

    // default
    static const Aya::SystemAddress Unassigned()
    {
        static Aya::SystemAddress s;
        AYAASSERT(s == SystemAddress());
        AYAASSERT(s != NetworkOwner::Server());
        AYAASSERT(s != NetworkOwner::ServerUnassigned());
        AYAASSERT(s != NetworkOwner::AssignedOther());
        return s;
    }

    // generic value used on client indicating assigned to other clients or server (i.e. not self)
    static const Aya::SystemAddress AssignedOther()
    {
        static Aya::SystemAddress s(0, 1);
        return s;
    }

    static bool isClient(const Aya::SystemAddress& address)
    {
        return ((address != Server()) && (address != Unassigned()) && (address != ServerUnassigned()));
    }

    static bool isServer(const Aya::SystemAddress& address)
    {
        return address == Server() || address == ServerUnassigned();
    }

    static Color3 colorFromAddress(const Aya::SystemAddress& systemAddress)
    {
        if (systemAddress == Server())
        {
            return Color3::white();
        }
        else if (systemAddress == Unassigned() || systemAddress == ServerUnassigned())
        {
            return Color3::black();
        }
        else if (systemAddress == AssignedOther())
            return Color3::gray();
        else
        {
            unsigned int address = systemAddress.getAddress();
            unsigned int port = systemAddress.getPort();
            address += port;
            return Aya::Color::colorFromInt(address);
        }
    }
};

} // namespace Network
} // namespace Aya