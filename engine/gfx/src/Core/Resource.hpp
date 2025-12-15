#pragma once

#include "boost.hpp"
#include "Debug.hpp"

namespace Aya
{
namespace Graphics
{

class Device;

class Resource : boost::noncopyable
{
    friend class Device;

public:
    explicit Resource(Device* device);
    virtual ~Resource();

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

    const std::string& getDebugName() const
    {
        return debugName;
    }
    void setDebugName(const std::string& value);

protected:
    Device* device;

    Resource* prev;
    Resource* next;

    std::string debugName;
};

} // namespace Graphics
} // namespace Aya
