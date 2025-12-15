


#include "Tool/HammerTool.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/Explosion.hpp"
#include "World/Primitive.hpp"
#include "Utility/SoundService.hpp"
#include "Utility/SoundWorld.hpp"
#include "SelectState.hpp"

namespace Aya
{


//////////////////////////////////////////////////////////////////////////////

const char* const sHammerTool = "Hammer";

HammerTool::HammerTool(Workspace* workspace)
    : Named<MouseCommand, sHammerTool>(workspace)
{
    FASTLOG1(FLog::MouseCommandLifetime, "HammerTool created: %p", this);
}

HammerTool::~HammerTool()
{
    FASTLOG1(FLog::MouseCommandLifetime, "HammerTool destroyed: %p", this);
}


void HammerTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
    hammerPart = shared_from(getUnlockedPartByLocalCharacter(inputObject));
}


shared_ptr<MouseCommand> HammerTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    if (hammerPart)
    {
        shared_ptr<Explosion> explosion = Creatable<Instance>::create<Explosion>();
        Explosion::propPosition.setValue(explosion.get(), hammerPart->getCoordinateFrame().translation);
        explosion->setVisualOnly();
        explosion->setParent(workspace);

        hammerPart->setParent(NULL);
        ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(BOMB_SOUND);
    }
    return shared_ptr<MouseCommand>();
}

const std::string HammerTool::getCursorName() const
{
    return hammerPart ? "HammerOverCursor" : "HammerCursor";
}


void HammerTool::render3dAdorn(Adorn* adorn)
{
    Super::render3dAdorn(adorn);

    if (hammerPart)
    {
        hammerPart->render3dSelect(adorn, SELECT_LIMIT);
    }
}

} // namespace Aya