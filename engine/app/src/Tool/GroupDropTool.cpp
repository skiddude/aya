


#include "Tool/GroupDropTool.hpp"
#include "Tool/MegaDragger.hpp"
#include "Tool/RunDragger.hpp"
#include "Tool/ToolsArrow.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Camera.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/ChangeHistory.hpp"
#include "SelectState.hpp"


namespace Aya
{

const char* const sGroupDropTool = "GroupDropTool";

// ToDo - should the parts all be shared pointers as part of a multiplayer dragging redo?

GroupDropTool::GroupDropTool(PartInstance* mousePart, const PartArray& partArray, Workspace* workspace, bool suppressAlign)
    : Named<GroupDragTool, sGroupDropTool>(mousePart, Vector3(), partArray, workspace)
{
    capture();

    dragging = true;
    megaDragger->startDragging();
    if (!suppressAlign)
        megaDragger->alignAndCleanParts();
    else
        megaDragger->cleanParts();
    lastHit = mousePart->getLocation().translation;
}

/*override*/ shared_ptr<MouseCommand> GroupDropTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(0);
    return shared_from(this);
}

shared_ptr<MouseCommand> GroupDropTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
    Super::onMouseUp(inputObject);
    releaseCapture();
    return shared_ptr<MouseCommand>();
}



shared_ptr<MouseCommand> GroupDropTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
    switch (inputObject->getKeyCode())
    {
    case AYA_SDLK_BACKSPACE:
    case AYA_SDLK_DELETE:
    case AYA_SDLK_ESCAPE:
    {
        return onCancelOperation();
    }
    default:
        break;
    }
    return Super::onKeyDown(inputObject);
}


shared_ptr<MouseCommand> GroupDropTool::onCancelOperation()
{
    megaDragger->removeParts();
    dragging = false;
    // Cancel the operation by removing the part from the workspace
    releaseCapture();
    return shared_ptr<MouseCommand>();
}

GroupDropTool::~GroupDropTool() {}


} // namespace Aya
