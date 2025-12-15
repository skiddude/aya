#include "FramebufferBGFX.hpp"
#include "TextureBGFX.hpp"
#include "DeviceBGFX.hpp"

#include <bgfx/bgfx.h>

namespace Aya
{
namespace Graphics
{

RenderbufferBGFX::RenderbufferBGFX(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
    : Renderbuffer(device, format, width, height, samples)
{
    AYAASSERT(samples);

    if (samples > device->getCaps().maxSamples)
    {
        throw Aya::runtime_error("Unsupported renderbuffer: too many samples (%d)", samples);
    }

    // Create a texture that will be used as a renderbuffer
    bgfx::TextureFormat::Enum bgfxFormat = TextureBGFX::getBGFXFormat(format);

    if (bgfxFormat == bgfx::TextureFormat::Unknown)
    {
        throw Aya::runtime_error("Unsupported renderbuffer format");
    }

    uint64_t flags = BGFX_TEXTURE_RT;

    if (samples > 1)
    {
        flags |= BGFX_TEXTURE_RT_MSAA_X4; // BGFX has predefined MSAA levels
    }

    textureHandle = bgfx::createTexture2D(width, height, false, 1, bgfxFormat, flags);

    if (!bgfx::isValid(textureHandle))
    {
        throw Aya::runtime_error("Failed to create renderbuffer texture");
    }
}

RenderbufferBGFX::~RenderbufferBGFX()
{
    if (bgfx::isValid(textureHandle))
    {
        bgfx::destroy(textureHandle);
    }
}

FramebufferBGFX::FramebufferBGFX(Device* device, bgfx::FrameBufferHandle handle)
    : Framebuffer(device, 0, 0, 1)
    , handle(handle)
    , isMain(true)
{
    // Main framebuffer - dimensions will be set by the window
}

FramebufferBGFX::FramebufferBGFX(Device* device, const std::vector<shared_ptr<Renderbuffer>>& color, const shared_ptr<Renderbuffer>& depth)
    : Framebuffer(device, 0, 0, 0)
    , handle(BGFX_INVALID_HANDLE)
    , isMain(false)
    , color(color)
    , depth(depth)
{
    AYAASSERT(!color.empty());

    if (color.size() > device->getCaps().maxDrawBuffers)
    {
        throw Aya::runtime_error("Unsupported framebuffer configuration: too many buffers (%d)", (int)color.size());
    }

    // Collect all texture handles for the framebuffer
    std::vector<bgfx::TextureHandle> textures;

    for (size_t i = 0; i < color.size(); ++i)
    {
        RenderbufferBGFX* buffer = static_cast<RenderbufferBGFX*>(color[i].get());
        AYAASSERT(buffer);
        AYAASSERT(!Texture::isFormatDepth(buffer->getFormat()));

        textures.push_back(buffer->getTextureHandle());

        if (i == 0)
        {
            width = buffer->getWidth();
            height = buffer->getHeight();
            samples = buffer->getSamples();
        }
        else
        {
            AYAASSERT(width == buffer->getWidth());
            AYAASSERT(height == buffer->getHeight());
            AYAASSERT(samples == buffer->getSamples());
        }
    }

    // Add depth buffer if present
    if (depth)
    {
        RenderbufferBGFX* buffer = static_cast<RenderbufferBGFX*>(depth.get());
        AYAASSERT(Texture::isFormatDepth(buffer->getFormat()));

        AYAASSERT(width == buffer->getWidth());
        AYAASSERT(height == buffer->getHeight());
        AYAASSERT(samples == buffer->getSamples());

        textures.push_back(buffer->getTextureHandle());
    }

    // Create framebuffer from textures
    handle = bgfx::createFrameBuffer(textures.size(), textures.data(), false);

    if (!bgfx::isValid(handle))
    {
        throw Aya::runtime_error("Failed to create framebuffer");
    }
}

FramebufferBGFX::~FramebufferBGFX()
{
    if (!isMain && bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }
}

void FramebufferBGFX::download(void* data, unsigned int size)
{
    AYAASSERT(size == width * height * 4);

    // BGFX doesn't support synchronous framebuffer downloads like OpenGL
    // You would need to use bgfx::readTexture which is asynchronous
    // and requires a callback

    // For now, we'll just assert that this isn't implemented
    // In a real implementation, you'd need to:
    // 1. Create a callback to receive the data
    // 2. Call bgfx::readTexture on the color attachment
    // 3. Wait for the callback (which might require frame coordination)

    AYAASSERT(false && "Framebuffer download not implemented for BGFX");
}

} // namespace Graphics
} // namespace Aya
