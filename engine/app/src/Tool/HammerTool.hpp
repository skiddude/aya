

#pragma once

#include "DataModel/MouseCommand.hpp"

namespace Aya
{

extern const char* const sHammerTool;
class HammerTool : public Named<MouseCommand, sHammerTool>
{
private:
    typedef Named<MouseCommand, sHammerTool> Super;
    shared_ptr<PartInstance> hammerPart;

    /*ovverrid*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
    /*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
    /*override*/ void render3dAdorn(Adorn* adorn);
    /*override*/ const std::string getCursorName() const;

public:
    HammerTool(Workspace* workspace);
    ~HammerTool();
    /*override*/ shared_ptr<MouseCommand> isSticky() const
    {
        return Creatable<MouseCommand>::create<HammerTool>(workspace);
    }
};

} // namespace Aya