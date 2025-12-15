

#pragma once

#include "DataModel/MouseCommand.hpp"

namespace Aya
{
class ICancelableTool
{

public:
    virtual shared_ptr<MouseCommand> onCancelOperation() = 0;
};


} // namespace Aya