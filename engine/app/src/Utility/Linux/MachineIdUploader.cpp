#include "Utility/MachineIdUploader.hpp"
#include "Debug.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <algorithm>
#ifndef __ANDROID_API__
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#ifdef __APPLE__
#include <net/if_dl.h>
#else
#include <linux/if_packet.h>
#include <net/if_arp.h>
#endif
#endif
using namespace Aya;

bool MachineIdUploader::fillMachineId(MachineId* out)
{
#ifdef __ANDROID_API__
    // On Android, we cannot directly access IMEI due to privacy restrictions
    // Instead, use Android ID which is unique per device and app installation
    // System.secure.ANDROID_ID
    return false;
#else
    struct ifaddrs* ifaddr = nullptr;
    bool success = getifaddrs(&ifaddr) == 0;
    if (!success)
    {
        return false;
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

#ifdef __APPLE__
        // macOS uses AF_LINK for link-layer addresses
        if (ifa->ifa_addr->sa_family == AF_LINK)
        {
            struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;

            // Skip loopback and interfaces without MAC address
            if (sdl->sdl_alen == 0 || strcmp(ifa->ifa_name, "lo") == 0 || strcmp(ifa->ifa_name, "lo0") == 0)
                continue;

            AYAASSERT(MacAddress::kBytesInMacAddress == sdl->sdl_alen);
            out->macAddresses.resize(out->macAddresses.size() + 1);
            size_t bytesToCopy = std::min((size_t)MacAddress::kBytesInMacAddress, (size_t)sdl->sdl_alen);

            unsigned char* mac = (unsigned char*)LLADDR(sdl);
            for (size_t i = 0; i < bytesToCopy; ++i)
            {
                out->macAddresses.back().address[i] = mac[i];
            }
        }
#else
        // Check for AF_PACKET (Linux specific) to get hardware address
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;

            // Skip loopback and interfaces without MAC address
            if (s->sll_halen == 0 || strcmp(ifa->ifa_name, "lo") == 0)
                continue;

            // Only process Ethernet interfaces (similar to Windows IF_TYPE_ETHERNET_CSMACD check)
            if (s->sll_hatype != ARPHRD_ETHER)
                continue;

            AYAASSERT(MacAddress::kBytesInMacAddress == s->sll_halen);
            out->macAddresses.resize(out->macAddresses.size() + 1);
            size_t bytesToCopy = std::min((size_t)MacAddress::kBytesInMacAddress, (size_t)s->sll_halen);
            for (size_t i = 0; i < bytesToCopy; ++i)
            {
                out->macAddresses.back().address[i] = s->sll_addr[i];
            }
        }
#endif
    }

    freeifaddrs(ifaddr);
    return success && !out->macAddresses.empty();
#endif
}