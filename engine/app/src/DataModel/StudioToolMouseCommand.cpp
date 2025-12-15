


#include "DataModel/StudioToolMouseCommand.hpp"
#include "DataModel/StudioTool.hpp"
#include "DataModel/Mouse.hpp"
#include "DataModel/PartInstance.hpp"
#include "Humanoid/Humanoid.hpp"

#include "Utility/StandardOut.hpp"

namespace Aya
{

const char* const sStudioToolMouseCommand = "StudioToolMouseCommand";

StudioToolMouseCommand::StudioToolMouseCommand(Workspace* workspace, shared_ptr<StudioTool> studioTool)
    : Named<ScriptMouseCommand, sStudioToolMouseCommand>(workspace)
    , tool(studioTool)
{
    toolUnequipped = tool->unequippedSignal.connect(boost::bind(&StudioToolMouseCommand::onEvent_ToolUnequipped, this));
}
StudioToolMouseCommand::~StudioToolMouseCommand()
{
    if (toolUnequipped.connected())
    {
        tool->unequip();
        AYAASSERT(!toolUnequipped.connected());
    }
}

void StudioToolMouseCommand::onEvent_ToolUnequipped()
{
    toolUnequipped.disconnect();
}


shared_ptr<MouseCommand> StudioToolMouseCommand::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    tool->activate();

    if (toolUnequipped.connected())
    {
        return Super::onMouseDown(inputObject);
    }
    else
    {
        return shared_ptr<MouseCommand>(); // will reset to default
    }
}

shared_ptr<MouseCommand> StudioToolMouseCommand::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
    tool->deactivate();

    return Super::onMouseUp(inputObject);
}

} // namespace Aya
