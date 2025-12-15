#pragma once

#include "Tree/Service.hpp"

namespace Aya
{

extern const char* const sReplicatedStorage;

class ReplicatedStorage
    : public DescribedCreatable<ReplicatedStorage, Instance, sReplicatedStorage, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
    , public Service
{
public:
    ReplicatedStorage();
};

} // namespace Aya
