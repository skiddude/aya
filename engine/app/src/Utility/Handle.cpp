

#include "Utility/Handle.hpp"
#include "Utility/Utilities.hpp"
#include "AyaAssert.hpp"
#include <limits>
#include "boost/lexical_cast.hpp"
#include "Reflection/Object.hpp"

namespace Aya
{

InstanceHandle::InstanceHandle(Reflection::DescribedBase* target)
    : target(shared_from(target))
{
}

bool InstanceHandle::operatorLess(const InstanceHandle& other) const
{
    return target < other.target;
}
bool InstanceHandle::empty() const
{
    return !target;
}

void InstanceHandle::linkTo(shared_ptr<Reflection::DescribedBase> target)
{
    this->target = target;
}



} // end namespace Aya