#include "GuidRegistryService.hpp"

const char* const Aya::Network::sGuidRegistryService = "GuidRegistryService";

Aya::Network::GuidRegistryService::GuidRegistryService(void)
    : registry(Registry::create())
{
}

Aya::Network::GuidRegistryService::~GuidRegistryService(void) {}
