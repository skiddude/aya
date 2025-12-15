

#pragma once

#include "Tool/ToolsArrow.hpp"
#include "DataModel/MouseCommand.hpp"
#include "Tool/AdvLuaDragger.hpp"
#include "Utility/Math.hpp"
#include <boost/shared_ptr.hpp>

namespace Aya
{

class PartInstance;

extern const char* const sAdvLuaDragTool;

class AdvLuaDragTool : public Named<AdvArrowToolBase, sAdvLuaDragTool>
{
private:
    typedef Named<AdvArrowToolBase, sAdvLuaDragTool> Super;
    boost::shared_ptr<AdvLuaDragger> advLuaDragger;
    boost::weak_ptr<Instance> selectIfNoDrag;
    std::string cursor;
    bool dragging;
    G3D::Vector2 downPoint2d;

    bool canDrag(const shared_ptr<InputObject>& inputObject) const;

    /////////////////////////////////////////////////////////
    // MouseCommand
    //
    /*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
    /*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
    /*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
    /*override*/ const std::string getCursorName() const
    {
        return cursor;
    }
    /*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
    /*override*/ void setCursor(std::string newCursor)
    {
        cursor = newCursor;
    }

public:
    /*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

    AdvLuaDragTool(PartInstance* mousePart, const Vector3& hitPointWorld, const std::vector<weak_ptr<PartInstance>>& partArray, Workspace* workspace,
        shared_ptr<Instance> selectIfNoDrag);

    ~AdvLuaDragTool();
};

} // namespace Aya