


#include "Tool/AdvDragTool.hpp"
#include "DataModel/PartInstance.hpp"
#include "Tool/PartDragTool.hpp"
#include "Tool/GroupDragTool.hpp"
#include "Tool/AdvLuaDragTool.hpp"

namespace Aya
{


shared_ptr<MouseCommand> AdvDragTool::onMouseDown(PartInstance* hitPart, const Vector3& hitWorld, const std::vector<Instance*>& dragInstances,
    const shared_ptr<InputObject>& inputObject, Workspace* workspace, shared_ptr<Instance> selectIfNoDrag)
{
    AYAASSERT(hitPart);

    PartArray partArray;
    DragUtilities::instancesToParts(dragInstances, partArray);

    shared_ptr<MouseCommand> advLuaDragTool =
        Creatable<MouseCommand>::create<AdvLuaDragTool>(hitPart, hitWorld, partArray, workspace, selectIfNoDrag);
    return advLuaDragTool->onMouseDown(inputObject);
}


} // namespace Aya
