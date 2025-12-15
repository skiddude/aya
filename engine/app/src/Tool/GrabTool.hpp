

#pragma once

#include "DataModel/MouseCommand.hpp"

namespace Aya
{

extern const char* const sGrabTool;
class GrabTool : public Named<MouseCommand, sGrabTool>
{
private:
    std::string cursor;

    /////////////////////////////////////////////////////////
    // MouseCommand
    //
    /*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
    /*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
    /*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
    /*override*/ const std::string getCursorName() const
    {
        return cursor;
    }

    /*override*/ bool drawConnectors() const
    {
        return true;
    } // default mouse command no draw connectors

public:
    GrabTool(Workspace* workspace);
    ~GrabTool();

    /*override*/ shared_ptr<MouseCommand> isSticky() const
    {
        return Creatable<MouseCommand>::create<GrabTool>(workspace);
    }
};

} // namespace Aya