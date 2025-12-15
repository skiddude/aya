#pragma once

#include "Core/Texture.hpp"
#include "Core/States.hpp"
#include <bgfx/bgfx.h>
#include <boost/enable_shared_from_this.hpp>
#include <map>

namespace Aya
{
namespace Graphics
{

class DeviceBGFX;

class TextureBGFX
    : public Texture
    , public boost::enable_shared_from_this<TextureBGFX>
{
public:
    TextureBGFX(
        Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);
    ~TextureBGFX();

    virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size);

    virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size);

    virtual bool supportsLocking() const;
    virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region);
    virtual void unlock(unsigned int index, unsigned int mip);

    virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip);

    virtual void commitChanges();
    virtual void generateMipmaps();

    void bind(unsigned int stage, const SamplerState& state);
    void bindWithUniform(unsigned int stage, bgfx::UniformHandle samplerUniform, const SamplerState& state);

    bgfx::TextureHandle getHandle() const
    {
        return handle;
    }

    static bgfx::TextureFormat::Enum getBGFXFormat(Format format);

private:
    bgfx::TextureHandle handle;
    SamplerState cachedState;

    typedef std::map<std::pair<unsigned, unsigned>, weak_ptr<Renderbuffer>> RenderBufferMap;
    RenderBufferMap renderBuffers;
};

} // namespace Graphics
} // namespace Aya
