


#include "Utility/CameraSubject.hpp"
#include "Debug.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Filters.hpp"
#include "DataModel/Camera.hpp"
#include "World/ContactManager.hpp"
#include "World/World.hpp"

namespace Aya
{

ContactManager* CameraSubject::getContactManager()
{
    if (Instance* instance = dynamic_cast<Instance*>(this))
    {
        if (World* world = Workspace::getWorldIfInWorkspace(instance))
        {
            return world->getContactManager();
        }
    }
    return NULL;
}

} // namespace Aya