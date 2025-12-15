


#include "Tool/PartDropTool.hpp"
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

const char* const sPartDropTool = "PartDropTool";

// ToDo - should the parts all be shared pointers as part of a multiplayer dragging redo?

PartDropTool::PartDropTool(PartInstance* mousePart, const Vector3& hitLocal, Workspace* workspace, shared_ptr<Instance> selectWhenDone)
    : Named<PartDragTool, sPartDropTool>(mousePart, Vector3(0, 0, 0), workspace, selectWhenDone)
    , hitLocal(hitLocal)
{
    capture();
    dragging = true;
    megaDragger->startDragging();
    runDragger->initLocal(workspace, megaDragger->getMousePart(), hitLocal);
}

/*override*/ shared_ptr<MouseCommand> PartDropTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(0);
    return shared_from(this);
}
// MouseCommand* PartDropTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
//{
//	Super::onMouseUp(inputObject);
//	releaseCapture();
//	return NULL;
// }

void PartDropTool::onMouseDelta(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(this->captured());

    if ((!dragging) && megaDragger->mousePartAlive())
    {
        dragging = true;
        megaDragger->startDragging();
        runDragger->initLocal(workspace, megaDragger->getMousePart(), hitLocal);
    }

    onMouseIdle(inputObject);
}


shared_ptr<MouseCommand> PartDropTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
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


shared_ptr<MouseCommand> PartDropTool::onCancelOperation()
{
    megaDragger->removeParts();
    // Cancel the operation by removing the part from the workspace
    releaseCapture();
    return shared_ptr<MouseCommand>();
}

PartDropTool::~PartDropTool() {}


} // namespace Aya
