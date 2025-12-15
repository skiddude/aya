

#pragma once

#include "Tree/Instance.hpp"

namespace Aya
{

extern const char* const sConfiguration;
class Configuration : public DescribedCreatable<Configuration, Instance, sConfiguration>
{
private:
    typedef DescribedCreatable<Configuration, Instance, sConfiguration> Super;

public:
    Configuration();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Instance
    /*override*/ bool askForbidChild(const Instance* instance) const;
    /*override*/ bool askSetParent(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};
} // namespace Aya