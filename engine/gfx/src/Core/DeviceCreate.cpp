#include "API/BGFX/DeviceBGFX.hpp"
#include "API/GL/DeviceGL.hpp"

namespace Aya
{
namespace Graphics
{

Device* Device::create(API api, void* windowHandle, void* display, int w, int h)
{
    if (api == API_BGFX)
        return new DeviceBGFX(windowHandle, display);
    if (api == API_OpenGL)
        return new DeviceGL(windowHandle, display, w, h);

    throw Aya::runtime_error("Unsupported API: %d", api);
}

} // namespace Graphics
} // namespace Aya
