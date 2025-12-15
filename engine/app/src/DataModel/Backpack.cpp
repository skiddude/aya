


#include "DataModel/Backpack.hpp"
#include "DataModel/Hopper.hpp"
#include "DataModel/Workspace.hpp"
#include "Player.hpp"
#include "Players.hpp"
#include "Script/script.hpp"

namespace Aya
{

const char* const sBackpack = "Backpack"; // rename class ultimately

Backpack::Backpack()
{
    setName("Backpack");
}



/*	When scripts run in the backpack

        BaseScript:		1.	In HopperBin if local backpack		(runs local)
                                2.	If backend Processing				(runs backend)

        LocalScript:3.	If local backpack					(runs local)
*/

bool Backpack::scriptShouldRun(BaseScript* script)
{
    AYAASSERT(isAncestorOf(script));

    Workspace* workspace = ServiceProvider::find<Workspace>(this);
    if (workspace)
    {
        Aya::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);
        bool isLocalBackpack = (this->getParent() == localPlayer);
        bool isLocalScript = (script->fastDynamicCast<LocalScript>() != NULL);

        if (isLocalBackpack && isLocalScript)
        {
            script->setLocalPlayer(shared_from(localPlayer));
            return true;
        }

        bool isBackendProcessing = Network::Players::backendProcessing(this);

        if (!isLocalScript && isBackendProcessing)
        {
            return true;
        }
    }
    return false;
}


} // namespace Aya
