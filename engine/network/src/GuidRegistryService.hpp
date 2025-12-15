#pragma once

#include "Tree/Service.hpp"


namespace Aya
{

namespace Network
{

extern const char* const sGuidRegistryService;
class GuidRegistryService
    : public DescribedNonCreatable<GuidRegistryService, Instance, sGuidRegistryService>
    , public Service
{
public:
    boost::intrusive_ptr<GuidItem<Instance>::Registry> const registry;
    GuidRegistryService(void);
    ~GuidRegistryService(void);
};
} // namespace Network
} // namespace Aya
