#pragma once

#include "Core/Framebuffer.hpp"
#include <bgfx/bgfx.h>
#include <vector>

namespace Aya
{
namespace Graphics
{

class DeviceBGFX;
class TextureBGFX;

class RenderbufferBGFX : public Renderbuffer
{
public:
    RenderbufferBGFX(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);
    ~RenderbufferBGFX();

    bgfx::TextureHandle getTextureHandle() const
    {
        return textureHandle;
    }

private:
    bgfx::TextureHandle textureHandle;
};

class FramebufferBGFX : public Framebuffer
{
public:
    FramebufferBGFX(Device* device, bgfx::FrameBufferHandle handle);
    FramebufferBGFX(Device* device, const std::vector<shared_ptr<Renderbuffer>>& color, const shared_ptr<Renderbuffer>& depth);
    ~FramebufferBGFX();

    virtual void download(void* data, unsigned int size);

    bgfx::FrameBufferHandle getHandle() const
    {
        return handle;
    }

private:
    bgfx::FrameBufferHandle handle;
    bool isMain;

    std::vector<shared_ptr<Renderbuffer>> color;
    shared_ptr<Renderbuffer> depth;
};

} // namespace Graphics
} // namespace Aya
