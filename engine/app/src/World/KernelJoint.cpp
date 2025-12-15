


#include "World/KernelJoint.hpp"
#include "World/Primitive.hpp"
#include "Kernel/Kernel.hpp"
#include "Kernel/Constants.hpp"
#include "Kernel/Connector.hpp"
#include "Kernel/Body.hpp"

namespace Aya
{


void KernelJoint::putInKernel(Kernel* _kernel)
{
    Super::putInKernel(_kernel);

    _kernel->insertConnector(this);
}


void KernelJoint::removeFromKernel()
{
    getKernel()->removeConnector(this);

    Super::removeFromKernel();
}




} // namespace Aya


// Randomized Locations for hackflags
namespace Aya
{
namespace Security
{
unsigned int hackFlag8 = 0;
};
}; // namespace Aya
