


#include "DataModel/PluginMouse.hpp"
#include "DataModel/PluginManager.hpp"
#include "World/ContactManager.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/Workspace.hpp"
#include "World/World.hpp"

namespace Aya
{

const char* const sPluginMouse = "PluginMouse";


static Reflection::EventDesc<PluginMouse, void(shared_ptr<const Instances>)> evt_dragEnterEvent(
    &PluginMouse::dragEnterEventSignal, "DragEnter", "instances", Security::Plugin);
REFLECTION_END();


PluginMouse::PluginMouse() {}

PluginMouse::~PluginMouse() {}

void PluginMouse::fireDragEnterEvent(shared_ptr<const Aya::Instances> instances, shared_ptr<InputObject> input)
{
    update(input);
    dragEnterEventSignal(instances);
}

} // namespace Aya
