

#pragma once

#include "DataModel/Mouse.hpp"
#include "DataModel/DataModel.hpp"
#include "Tree/Instance.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/InputObject.hpp"
#include "Utility/TextureId.hpp"
#include "signal.hpp"
#include "DataModel/Filters.hpp"
#include "DataModel/MouseCommand.hpp"


namespace Aya
{

class PartInstance;
class PVInstance;

extern const char* const sPluginMouse;

class PluginMouse : public DescribedNonCreatable<PluginMouse, Mouse, sPluginMouse>
{
private:
public:
    PluginMouse();
    ~PluginMouse();

    void fireDragEnterEvent(shared_ptr<const Aya::Instances> instances, shared_ptr<InputObject> input);

    Aya::signal<void(shared_ptr<const Instances>)> dragEnterEventSignal;
};


} // namespace Aya
