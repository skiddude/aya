
#include "Humanoid/StatusInstance.hpp"

namespace Aya
{

const char* const sStatusInstance = "Status";

StatusInstance::StatusInstance()
    : Super()
{
    setName(sStatusInstance);
    lockName();
}
bool StatusInstance::askSetParent(const Instance* instance) const
{
    return false;
}

} // namespace Aya
