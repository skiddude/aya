#include "AyaDbgInfo.hpp"

#include <string.h>

using namespace Aya;

AyaDbgInfo AyaDbgInfo::s_instance;

AyaDbgInfo::AyaDbgInfo()
{
    memset(this, 0, sizeof(AyaDbgInfo));
}

void AyaDbgInfo::AddPlace(long ID)
{
    // Shift all places to upper indices
    for (int i = PLACE_HISTORY - 1; i > 0; i--)
    {
        s_instance.PlaceIDs[i] = s_instance.PlaceIDs[i - 1];
    }
    s_instance.PlaceIDs[0] = ID;
    s_instance.PlaceCounter++;
}

void AyaDbgInfo::RemovePlace(long ID)
{
    s_instance.PlaceCounter--;
    for (int i = 0; i < PLACE_HISTORY; i++)
    {
        if (s_instance.PlaceIDs[i] == ID)
        {
            // Shift all places after it to lower indices
            for (int j = i; j < PLACE_HISTORY - 1; j++)
            {
                s_instance.PlaceIDs[j] = s_instance.PlaceIDs[j + 1];
            }
            s_instance.PlaceIDs[PLACE_HISTORY - 1] = 0;
            return;
        }
    }
}

#pragma warning(push)
#pragma warning(disable : 4996)
void AyaDbgInfo::SetGfxCardName(const char* s)
{
    strncpy(s_instance.GfxCardName, s, DBG_STRING_MAX - 1);
    s_instance.GfxCardName[DBG_STRING_MAX - 1] = '\0';
}

void AyaDbgInfo::SetGfxCardDriverVersion(const char* s)
{
    strncpy(s_instance.GfxCardDriverVersion, s, DBG_STRING_MAX - 1);
    s_instance.GfxCardDriverVersion[DBG_STRING_MAX - 1] = '\0';
}

void AyaDbgInfo::SetGfxCardVendor(const char* s)
{
    strncpy(s_instance.GfxCardVendorName, s, DBG_STRING_MAX - 1);
    s_instance.GfxCardVendorName[DBG_STRING_MAX - 1] = '\0';
}

void AyaDbgInfo::SetCPUName(const char* s)
{
    strncpy(s_instance.CPUName, s, DBG_STRING_MAX - 1);
    s_instance.CPUName[DBG_STRING_MAX - 1] = '\0';
}

void AyaDbgInfo::SetServerIP(const char* s)
{
    strncpy(s_instance.ServerIP, s, DBG_STRING_MAX - 1);
    s_instance.ServerIP[DBG_STRING_MAX - 1] = '\0';
}
#pragma warning(pop)
