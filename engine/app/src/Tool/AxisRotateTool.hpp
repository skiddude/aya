

#pragma once

#include "Tool/AxisMoveTool.hpp"

namespace Aya
{

extern const char* const sAxisRotateTool;
class AxisRotateTool : public Named<AxisToolBase, sAxisRotateTool>
{
private:
    /*override*/ Color3 getHandleColor() const
    {
        return Color3::green();
    }
    /*override*/ HandleType getDragType() const
    {
        return HANDLE_ROTATE;
    }

public:
    AxisRotateTool(Workspace* workspace)
        : Named<AxisToolBase, sAxisRotateTool>(workspace)
    {
    }

    /*override*/ shared_ptr<MouseCommand> isSticky() const
    {
        return Creatable<MouseCommand>::create<AxisRotateTool>(workspace);
    }
};



} // namespace Aya