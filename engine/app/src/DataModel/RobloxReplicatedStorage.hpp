#pragma once

#include "Tree/Service.hpp"

namespace Aya
{

extern const char* const sRobloxReplicatedStorage;

class RobloxReplicatedStorage
    : public DescribedCreatable<RobloxReplicatedStorage, Instance, sRobloxReplicatedStorage, Reflection::ClassDescriptor::INTERNAL,
          Security::RobloxScript>
    , public Service
{
public:
    RobloxReplicatedStorage();
};

} // namespace Aya
