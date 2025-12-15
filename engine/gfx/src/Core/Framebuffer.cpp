#include "Core/Framebuffer.hpp"

#include "Profiler.hpp"

namespace Aya
{
namespace Graphics
{

Renderbuffer::Renderbuffer(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
    : Resource(device)
    , format(format)
    , width(width)
    , height(height)
    , samples(samples)
{
    AYAPROFILER_COUNTER_ADD("memory/gpu/renderbuffer", Texture::getImageSize(format, width, height) * samples);
}

Renderbuffer::~Renderbuffer()
{
    AYAPROFILER_COUNTER_SUB("memory/gpu/renderbuffer", Texture::getImageSize(format, width, height) * samples);
}

Framebuffer::Framebuffer(Device* device, unsigned int width, unsigned int height, unsigned int samples)
    : Resource(device)
    , width(width)
    , height(height)
    , samples(samples)
{
}

Framebuffer::~Framebuffer() {}

} // namespace Graphics
} // namespace Aya