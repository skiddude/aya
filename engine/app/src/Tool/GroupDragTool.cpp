


#include "Tool/GroupDragTool.hpp"
#include "Tool/MegaDragger.hpp"
#include "Tool/Dragger.hpp"
#include "Tool/ToolsArrow.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/ChangeHistory.hpp"

namespace Aya
{

const char* const sGroupDragTool = "GroupDragTool";

GroupDragTool::GroupDragTool(PartInstance* mousePart, const Vector3& hitPointWorld, const PartArray& partArray, Workspace* workspace)
    : Named<MouseCommand, sGroupDragTool>(workspace)
    , dragging(false)
{
    FASTLOG1(FLog::MouseCommandLifetime, "GroupDragTool created: %p", this);
    AYAASSERT(mousePart);
    megaDragger.reset(new MegaDragger(mousePart, partArray, workspace));
}


shared_ptr<MouseCommand> GroupDragTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    downPoint = inputObject->get2DPosition();
    capture();
    return shared_from(this);
}

void GroupDragTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(this->captured());

    if (!dragging)
    {
        if ((inputObject->get2DPosition() - downPoint).length() > 4)
        {
            dragging = true;
            megaDragger->startDragging();
            megaDragger->alignAndCleanParts();
            lastHit = megaDragger->hitObjectOrPlane(inputObject);
        }
    }

    onMouseIdle(inputObject);
}


void GroupDragTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(captured());

    Vector3 hit;

    if (captured() && dragging && megaDragger->mousePartAlive() &&
        Math::intersectRayPlane(getSearchRay(inputObject), Plane(Vector3::unitY(), Vector3::zero()), hit))
    {
        hit = megaDragger->hitObjectOrPlane(inputObject);

        megaDragger->continueDragging(); // do this every time - multiplayer

        Vector3 hitSnapped = DragUtilities::toGrid(hit);

        Vector3 delta = DragUtilities::toGrid(hitSnapped - lastHit);

        lastHit = hitSnapped;

        if (delta != Vector3::zero())
        {
            megaDragger->safeMoveYDrop(delta);
            megaDragger->safeMoveToMinimumHeight(hit.y);
        }
    }
}

shared_ptr<MouseCommand> GroupDragTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(this->captured());

    if (dragging)
    {
        megaDragger->finishDragging();
        dragging = false;
        ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
    }
    releaseCapture();
    return shared_ptr<MouseCommand>();
}


shared_ptr<MouseCommand> GroupDragTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
    if (dragging)
    {
        switch (inputObject->getKeyCode())
        {
        case AYA_SDLK_r:
        {
            megaDragger->safeRotate(Math::matrixRotateY());
            break;
        }
        case AYA_SDLK_t:
        {
            megaDragger->safeRotate(Math::matrixTiltZ());
            break;
        }
        default:
            break;
        }
        return shared_from(this);
    }
    else
    {
        AYAASSERT(0); // when did this happen
        return shared_ptr<MouseCommand>();
    }
}


GroupDragTool::~GroupDragTool()
{
    if (dragging)
    {
        AYAASSERT(0); // shouldn't ever get here....
    }
    FASTLOG1(FLog::MouseCommandLifetime, "GroupDragTool destroyed: %p", this);
}

} // namespace Aya
