#include "Core/Resource.hpp"

#include "Core/Device.hpp"

namespace Aya
{
namespace Graphics
{

Resource::Resource(Device* device)
    : device(device)
    , prev(NULL)
    , next(NULL)
{
    if (device->resourceListHead)
    {
        AYAASSERT(device->resourceListTail);

        prev = device->resourceListTail;
        device->resourceListTail->next = this;
        device->resourceListTail = this;
    }
    else
    {
        AYAASSERT(!device->resourceListTail);

        device->resourceListHead = this;
        device->resourceListTail = this;
    }
}

Resource::~Resource()
{
    if (prev)
        prev->next = next;
    else
    {
        AYAASSERT(device->resourceListHead == this);
        device->resourceListHead = next;
    }

    if (next)
        next->prev = prev;
    else
    {
        AYAASSERT(device->resourceListTail == this);
        device->resourceListTail = prev;
    }
}

void Resource::onDeviceLost() {}

void Resource::onDeviceRestored() {}

void Resource::setDebugName(const std::string& value)
{
    debugName = value;
}

} // namespace Graphics
} // namespace Aya