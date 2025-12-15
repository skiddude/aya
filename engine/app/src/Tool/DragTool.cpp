


#include "Tool/DragTool.hpp"
#include "DataModel/PartInstance.hpp"
#include "Tool/PartDragTool.hpp"
#include "Tool/GroupDragTool.hpp"
#include "Tool/LuaDragTool.hpp"

namespace Aya
{


shared_ptr<MouseCommand> DragTool::onMouseDown(PartInstance* hitPart, const Vector3& hitWorld, const std::vector<Instance*>& dragInstances,
    const shared_ptr<InputObject>& inputObject, Workspace* workspace, shared_ptr<Instance> selectIfNoDrag)
{
    AYAASSERT(hitPart);

    PartArray partArray;
    DragUtilities::instancesToParts(dragInstances, partArray);

#if 1
    shared_ptr<LuaDragTool> luaDragTool = Creatable<MouseCommand>::create<LuaDragTool>(hitPart, hitWorld, partArray, workspace, selectIfNoDrag);
    return luaDragTool->onMouseDown(inputObject);
#else
    if (partArray.size() == 1)
    {
        AYAASSERT(partArray[0].lock().get() == hitPart);

        shared_ptr<MouseCommand> partDragTool = Creatable<MouseCommand>::create<PartDragTool>(hitPart, hitWorld, workspace, selectIfNoDrag);
        return partDragTool->onMouseDown(InputObject);
    }
    else
    {
        shared_ptr<MouseCommand> groupDragTool = Creatable<MouseCommand>::create<GroupDragTool>(hitPart, hitWorld, partArray, workspace);
        return groupDragTool->onMouseDown(InputObject);
    }
#endif
}


} // namespace Aya
