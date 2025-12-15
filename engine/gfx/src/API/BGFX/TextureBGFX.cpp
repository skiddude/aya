#include "TextureBGFX.hpp"
#include "DeviceBGFX.hpp"
#include "FramebufferBGFX.hpp"

#include <bgfx/bgfx.h>

LOGGROUP(Graphics)

namespace Aya
{
namespace Graphics
{

bgfx::TextureFormat::Enum TextureBGFX::getBGFXFormat(Format format)
{
    switch (format)
    {
    case Format_L8:
        return bgfx::TextureFormat::R8;
    case Format_LA8:
        return bgfx::TextureFormat::RG8;
    case Format_RGB5A1:
        return bgfx::TextureFormat::RGB5A1;
    case Format_RGBA8:
        return bgfx::TextureFormat::RGBA8;
    case Format_RG16:
        return bgfx::TextureFormat::RG16;
    case Format_RGBA16F:
        return bgfx::TextureFormat::RGBA16F;
    case Format_BC1:
        return bgfx::TextureFormat::BC1;
    case Format_BC2:
        return bgfx::TextureFormat::BC2;
    case Format_BC3:
        return bgfx::TextureFormat::BC3;
    case Format_PVRTC_RGB2:
        return bgfx::TextureFormat::PTC12;
    case Format_PVRTC_RGBA2:
        return bgfx::TextureFormat::PTC12A;
    case Format_PVRTC_RGB4:
        return bgfx::TextureFormat::PTC14;
    case Format_PVRTC_RGBA4:
        return bgfx::TextureFormat::PTC14A;
    case Format_ETC1:
        return bgfx::TextureFormat::ETC1;
    case Format_D16:
        return bgfx::TextureFormat::D16;
    case Format_D24S8:
        return bgfx::TextureFormat::D24S8;
    default:
        return bgfx::TextureFormat::Unknown;
    }
}

static uint64_t getBGFXTextureFlags(Texture::Usage usage, unsigned int mipLevels)
{
    uint64_t flags = BGFX_TEXTURE_NONE;

    if (usage == Texture::Usage_Renderbuffer)
    {
        flags |= BGFX_TEXTURE_RT;
    }

    if (mipLevels > 1)
    {
        flags |= BGFX_TEXTURE_NONE; // Mips are created by default
    }

    return flags;
}

static uint32_t getBGFXSamplerFlags(const SamplerState& state)
{
    uint32_t flags = BGFX_SAMPLER_NONE;

    // Filter mode
    switch (state.getFilter())
    {
    case SamplerState::Filter_Point:
        flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        break;
    case SamplerState::Filter_Linear:
        flags |= BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
        break;
    case SamplerState::Filter_Anisotropic:
        flags |= BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
        break;
    }

    // Address mode
    switch (state.getAddress())
    {
    case SamplerState::Address_Wrap:
        // Wrap is the default - no flags needed
        break;
    case SamplerState::Address_Clamp:
        flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP;
        break;
    }

    return flags;
}

TextureBGFX::TextureBGFX(
    Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage)
    : Texture(device, type, format, width, height, depth, mipLevels, usage)
    , cachedState(SamplerState::Filter_Count)
{
    bgfx::TextureFormat::Enum bgfxFormat = getBGFXFormat(format);

    if (bgfxFormat == bgfx::TextureFormat::Unknown)
    {
        throw Aya::runtime_error("Unsupported texture format");
    }

    uint64_t flags = getBGFXTextureFlags(usage, mipLevels);

    if (type == Type_3D)
    {
        handle = bgfx::createTexture3D(width, height, depth, mipLevels > 1, bgfxFormat, flags);
    }
    else if (type == Type_Cube)
    {
        handle = bgfx::createTextureCube(width, mipLevels > 1, 1, bgfxFormat, flags);
    }
    else if (type == Type_2DMultisampled)
    {
        handle = bgfx::createTexture2D(width, height, mipLevels > 1, 1, bgfxFormat, flags | BGFX_TEXTURE_RT_MSAA_X4);
    }
    else // Type_2D
    {
        handle = bgfx::createTexture2D(width, height, mipLevels > 1, 1, bgfxFormat, flags);
    }

    if (!bgfx::isValid(handle))
    {
        throw Aya::runtime_error("Failed to create texture");
    }
}

TextureBGFX::~TextureBGFX()
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }
}

void TextureBGFX::upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size)
{
    AYAASSERT(index < (type == Type_Cube ? 6u : 1u));
    AYAASSERT(mip < mipLevels);

    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    AYAASSERT(region.x + region.width <= mipWidth);
    AYAASSERT(region.y + region.height <= mipHeight);
    AYAASSERT(region.z + region.depth <= mipDepth);
    AYAASSERT(size == getImageSize(format, region.width, region.height) * region.depth);

    const bgfx::Memory* mem = bgfx::copy(data, size);

    if (type == Type_3D)
    {
        bgfx::updateTexture3D(handle, mip, region.x, region.y, region.z, region.width, region.height, region.depth, mem);
    }
    else if (type == Type_Cube)
    {
        bgfx::updateTextureCube(handle, 0, index, mip, region.x, region.y, region.width, region.height, mem);
    }
    else
    {
        bgfx::updateTexture2D(handle, 0, mip, region.x, region.y, region.width, region.height, mem);
    }
}

bool TextureBGFX::download(unsigned int index, unsigned int mip, void* data, unsigned int size)
{
    // BGFX doesn't support direct texture download in the same way as GL
    // This would require reading from a framebuffer that the texture is attached to
    const DeviceCapsBGFX& caps = static_cast<DeviceBGFX*>(device)->getCapsBGFX();

    if (!caps.supportsTextureReadBack)
    {
        return false;
    }

    // Reading back texture data would require creating a framebuffer and using bgfx::readTexture
    // which is async in BGFX

    return false;
}

bool TextureBGFX::supportsLocking() const
{
    // BGFX doesn't support texture locking in the traditional sense
    return false;
}

Texture::LockResult TextureBGFX::lock(unsigned int index, unsigned int mip, const TextureRegion& region)
{
    // BGFX doesn't support direct texture locking
    LockResult result = {0, 0, 0};
    return result;
}

void TextureBGFX::unlock(unsigned int index, unsigned int mip)
{
    // BGFX doesn't support direct texture locking
}

shared_ptr<Renderbuffer> TextureBGFX::getRenderbuffer(unsigned int index, unsigned int mip)
{
    AYAASSERT(index < (type == Type_Cube ? 6u : 1u));
    AYAASSERT(mip < mipLevels);
    AYAASSERT(usage == Usage_Renderbuffer);

    std::pair<unsigned, unsigned> key(index, mip);

    shared_ptr<Renderbuffer> result = renderBuffers[key].lock();

    if (!result)
    {
        result.reset(new RenderbufferBGFX(device, format, getMipSide(width, mip), getMipSide(height, mip), getType() == Type_2DMultisampled ? 4 : 1));
        renderBuffers[key] = result;
    }

    return result;
}

void TextureBGFX::commitChanges()
{
    // BGFX handles texture updates immediately
    // No deferred commit needed
}

void TextureBGFX::generateMipmaps()
{
    // BGFX generates mipmaps automatically when creating textures with mipLevels > 1
    // Or we can request regeneration if needed
    // Note: Not all backends support runtime mipmap generation
}

void TextureBGFX::bind(unsigned int stage, const SamplerState& state)
{
    uint32_t samplerFlags = getBGFXSamplerFlags(state);

    // In BGFX, textures are set per-draw call, not globally
    // We just set the texture and sampler for the given stage
    // Note: This requires a valid sampler uniform handle to be passed from the caller
    bgfx::setTexture(stage, BGFX_INVALID_HANDLE, handle, samplerFlags);

    cachedState = state;
}

void TextureBGFX::bindWithUniform(unsigned int stage, bgfx::UniformHandle samplerUniform, const SamplerState& state)
{
    uint32_t samplerFlags = getBGFXSamplerFlags(state);

    // Set texture with the proper sampler uniform
    bgfx::setTexture(stage, samplerUniform, handle, samplerFlags);

    cachedState = state;
}

} // namespace Graphics
} // namespace Aya
