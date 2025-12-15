

#pragma once

#include "DataModel/Hopper.hpp"
#include "Script/IScriptFilter.hpp"

namespace Aya
{

extern const char* const sBackpack;
class Backpack
    : public DescribedCreatable<Backpack, Hopper, sBackpack>
    , public IScriptFilter
{
private:
    // IScriptOwner
    /*override*/ bool scriptShouldRun(BaseScript* script);

public:
    Backpack();
};


} // namespace Aya