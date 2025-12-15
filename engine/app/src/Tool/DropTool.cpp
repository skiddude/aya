


#include "Tool/DropTool.hpp"
#include "DataModel/PartInstance.hpp"
#include "Tool/PartDropTool.hpp"
#include "Tool/GroupDropTool.hpp"

namespace Aya
{


shared_ptr<MouseCommand> DropTool::createDropTool(const Vector3& hitLocal, const std::vector<Instance*>& dragInstances, Workspace* workspace,
    shared_ptr<Instance> selectIfNoDrag, bool suppressPartsAlign)
{
    PartArray partArray;
    DragUtilities::instancesToParts(dragInstances, partArray);

    if (partArray.size() == 0)
    {
        return shared_ptr<MouseCommand>();
    }
    PartInstance* hitPart = partArray[0].lock().get();

    if (partArray.size() == 1)
    {
        return Creatable<MouseCommand>::create<PartDropTool>(hitPart, hitLocal, workspace, selectIfNoDrag);
    }
    else
    {
        return Creatable<MouseCommand>::create<GroupDropTool>(hitPart, partArray, workspace, suppressPartsAlign);
    }
}


} // namespace Aya
