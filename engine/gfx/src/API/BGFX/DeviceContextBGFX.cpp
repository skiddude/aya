#include "DeviceBGFX.hpp"

#include "GeometryBGFX.hpp"
#include "ShaderBGFX.hpp"
#include "TextureBGFX.hpp"
#include "FramebufferBGFX.hpp"

#include <bgfx/bgfx.h>

#ifdef AYA_OS_WINDOWS
#include <Windows.h>
#endif

namespace Aya
{
namespace Graphics
{

// Convert RasterizerState cull mode to BGFX state flags
static uint64_t getCullStateFlags(RasterizerState::CullMode mode)
{
    switch (mode)
    {
    case RasterizerState::Cull_None:
        return BGFX_STATE_CULL_CW | BGFX_STATE_CULL_CCW; // Disable culling
    case RasterizerState::Cull_Back:
        return BGFX_STATE_CULL_CW;
    case RasterizerState::Cull_Front:
        return BGFX_STATE_CULL_CCW;
    default:
        return 0;
    }
}

// Convert BlendState factors to BGFX blend equation
static uint64_t getBlendStateFlags(const BlendState& state)
{
    if (!state.blendingNeeded())
        return 0;

    static const uint64_t blendFactorsBGFX[BlendState::Factor_Count] = {
        BGFX_STATE_BLEND_ONE,           // Factor_One
        BGFX_STATE_BLEND_ZERO,          // Factor_Zero
        BGFX_STATE_BLEND_DST_COLOR,     // Factor_DstColor
        BGFX_STATE_BLEND_SRC_ALPHA,     // Factor_SrcAlpha
        BGFX_STATE_BLEND_INV_SRC_ALPHA, // Factor_InvSrcAlpha
        BGFX_STATE_BLEND_DST_ALPHA,     // Factor_DstAlpha
        BGFX_STATE_BLEND_INV_DST_ALPHA  // Factor_InvDstAlpha
    };

    uint64_t srcBlend = blendFactorsBGFX[state.getColorSrc()];
    uint64_t dstBlend = blendFactorsBGFX[state.getColorDst()];

    uint64_t flags = BGFX_STATE_BLEND_FUNC(srcBlend, dstBlend);

    if (state.separateAlphaBlend())
    {
        uint64_t srcAlpha = blendFactorsBGFX[state.getAlphaSrc()];
        uint64_t dstAlpha = blendFactorsBGFX[state.getAlphaDst()];
        flags = BGFX_STATE_BLEND_FUNC_SEPARATE(srcBlend, dstBlend, srcAlpha, dstAlpha);
    }

    return flags;
}

// Convert DepthState to BGFX state flags
static uint64_t getDepthStateFlags(const DepthState& state)
{
    uint64_t flags = 0;

    if (state.getFunction() != DepthState::Function_None)
    {
        switch (state.getFunction())
        {
        case DepthState::Function_Always:
            flags |= BGFX_STATE_DEPTH_TEST_ALWAYS;
            break;
        case DepthState::Function_Less:
            flags |= BGFX_STATE_DEPTH_TEST_LESS;
            break;
        case DepthState::Function_LessEqual:
            flags |= BGFX_STATE_DEPTH_TEST_LEQUAL;
            break;
        default:
            break;
        }
    }

    if (state.getWrite())
    {
        flags |= BGFX_STATE_WRITE_Z;
    }

    return flags;
}

// Convert color mask to BGFX state flags
static uint64_t getColorMaskFlags(unsigned int colorMask)
{
    uint64_t flags = 0;

    if (colorMask & BlendState::Color_R)
        flags |= BGFX_STATE_WRITE_R;
    if (colorMask & BlendState::Color_G)
        flags |= BGFX_STATE_WRITE_G;
    if (colorMask & BlendState::Color_B)
        flags |= BGFX_STATE_WRITE_B;
    if (colorMask & BlendState::Color_A)
        flags |= BGFX_STATE_WRITE_A;

    return flags;
}

DeviceContextBGFX::DeviceContextBGFX(DeviceBGFX* dev)
    : globalDataVersion(0)
    , device(dev)
    , defaultAnisotropy(1)
    , cachedProgram(nullptr)
    , cachedRasterizerState(RasterizerState::Cull_None)
    , cachedBlendState(BlendState::Mode_None)
    , cachedDepthState(DepthState::Function_Always, false)
    , cachedStateFlags(0)
    , currentView(0)
{
    globalUniform = BGFX_INVALID_HANDLE;

    for (size_t i = 0; i < ARRAYSIZE(cachedTextures); ++i)
    {
        cachedTextures[i] = nullptr;

        // Create sampler uniforms for each texture stage
        char uniformName[32];
        snprintf(uniformName, sizeof(uniformName), "s_texColor%d", (int)i);
        samplerUniforms[i] = bgfx::createUniform(uniformName, bgfx::UniformType::Sampler);
    }
}

DeviceContextBGFX::~DeviceContextBGFX()
{
    if (bgfx::isValid(globalUniform))
    {
        bgfx::destroy(globalUniform);
        globalUniform = BGFX_INVALID_HANDLE;
    }

    // Destroy sampler uniforms
    for (size_t i = 0; i < ARRAYSIZE(samplerUniforms); ++i)
    {
        if (bgfx::isValid(samplerUniforms[i]))
        {
            bgfx::destroy(samplerUniforms[i]);
            samplerUniforms[i] = BGFX_INVALID_HANDLE;
        }
    }
}

void DeviceContextBGFX::clearStates()
{
    // Clear program cache
    cachedProgram = nullptr;

    // Clear texture cache
    for (size_t i = 0; i < ARRAYSIZE(cachedTextures); ++i)
        cachedTextures[i] = nullptr;

    // Clear states to invalid values to guarantee a cache miss on the next setup
    cachedRasterizerState = RasterizerState(RasterizerState::Cull_Count);
    cachedBlendState = BlendState(BlendState::Mode_Count);
    cachedDepthState = DepthState(DepthState::Function_Count, false);

    cachedStateFlags = 0;
}

void DeviceContextBGFX::invalidateCachedProgram()
{
    cachedProgram = nullptr;
}

void DeviceContextBGFX::invalidateCachedTexture(Texture* texture)
{
    TextureBGFX* textureBGFX = static_cast<TextureBGFX*>(texture);
    for (unsigned int stage = 0; stage < ARRAYSIZE(cachedTextures); ++stage)
        if (cachedTextures[stage] == textureBGFX)
            cachedTextures[stage] = nullptr;
}

void DeviceContextBGFX::invalidateCachedTextureStage(unsigned int stage)
{
    AYAASSERT(stage < ARRAYSIZE(cachedTextures));
    cachedTextures[stage] = nullptr;
}

void DeviceContextBGFX::defineGlobalConstants(size_t dataSize)
{
    AYAASSERT(globalData.empty());
    AYAASSERT(dataSize > 0);

    globalData.resize(dataSize);

    // Create a uniform buffer for global constants
    char uniformName[256];
    snprintf(uniformName, sizeof(uniformName), "u_globals");
    globalUniform = bgfx::createUniform(uniformName, bgfx::UniformType::Vec4, (dataSize + 15) / 16);
}

void DeviceContextBGFX::setDefaultAnisotropy(unsigned int value)
{
    defaultAnisotropy = value;
}

void DeviceContextBGFX::updateGlobalConstants(const void* data, size_t dataSize)
{
    AYAASSERT(dataSize == globalData.size());

    memcpy(&globalData[0], data, dataSize);
    globalDataVersion++;

    // Update the uniform buffer
    if (bgfx::isValid(globalUniform))
    {
        bgfx::setUniform(globalUniform, &globalData[0], (dataSize + 15) / 16);
    }
}

void DeviceContextBGFX::bindFramebuffer(Framebuffer* buffer)
{
    FramebufferBGFX* framebufferBGFX = static_cast<FramebufferBGFX*>(buffer);

    // Set the framebuffer for the current view
    bgfx::setViewFrameBuffer(currentView, framebufferBGFX->getHandle());
    bgfx::setViewRect(currentView, 0, 0, uint16_t(buffer->getWidth()), uint16_t(buffer->getHeight()));
}

void DeviceContextBGFX::clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil)
{
    uint16_t clearFlags = 0;

    if (mask & Buffer_Color)
    {
        clearFlags |= BGFX_CLEAR_COLOR;
    }

    if (mask & Buffer_Depth)
    {
        clearFlags |= BGFX_CLEAR_DEPTH;
    }

    if (mask & Buffer_Stencil)
    {
        clearFlags |= BGFX_CLEAR_STENCIL;
    }

    uint32_t rgba = 0;
    if (mask & Buffer_Color)
    {
        rgba = (static_cast<uint32_t>(color[0] * 255.0f) << 24) | (static_cast<uint32_t>(color[1] * 255.0f) << 16) |
               (static_cast<uint32_t>(color[2] * 255.0f) << 8) | (static_cast<uint32_t>(color[3] * 255.0f));
    }

    bgfx::setViewClear(currentView, clearFlags, rgba, depth, static_cast<uint8_t>(stencil));

    // Touch the view to ensure clear happens
    bgfx::touch(currentView);
}

void DeviceContextBGFX::copyFramebuffer(Framebuffer* buffer, Texture* texture, int xOffset, int yOffset)
{
    AYAASSERT(texture->getType() == Texture::Type_2D);
    AYAASSERT(buffer->getWidth() == texture->getWidth() && buffer->getHeight() == texture->getHeight());

    if (!device->getCapsBGFX().supportsTextureBlits)
    {
        // Fall back to a different method or log warning
        return;
    }

    FramebufferBGFX* framebufferBGFX = static_cast<FramebufferBGFX*>(buffer);
    TextureBGFX* textureBGFX = static_cast<TextureBGFX*>(texture);

    // BGFX blit from framebuffer to texture
    // Note: BGFX handles this differently - would need access to the framebuffer's texture attachments
    invalidateCachedTextureStage(0);

    // bgfx::blit(currentView, textureBGFX->getHandle(), 0, xOffset, yOffset, framebufferTexture, 0, 0, 0, width, height);
}

void DeviceContextBGFX::resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask)
{
    AYAASSERT(msaaBuffer->getSamples() > 1);
    AYAASSERT(buffer->getSamples() == 1);
    AYAASSERT(msaaBuffer->getWidth() == buffer->getWidth() && msaaBuffer->getHeight() == buffer->getHeight());

    // BGFX handles MSAA resolve automatically when rendering to a MSAA framebuffer
    // and then using it as a texture or blitting to a non-MSAA target
    // This may not need explicit implementation
}

void DeviceContextBGFX::discardFramebuffer(Framebuffer* buffer, unsigned int mask)
{
    // BGFX handles framebuffer discard internally
    // bgfx::discard() is used differently - it discards all pending draw calls
    uint8_t discardFlags = 0;

    // BGFX doesn't have per-buffer discard flags like GL
    // The discard() function discards all state changes
    if (mask != 0)
    {
        bgfx::discard();
    }
}

void DeviceContextBGFX::bindProgram(ShaderProgram* program)
{
    ShaderProgramBGFX* programBGFX = static_cast<ShaderProgramBGFX*>(program);

    cachedProgram = programBGFX;

    // BGFX sets the program per-draw call via bgfx::submit, not here
    // Just cache the program for later use
}

void DeviceContextBGFX::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    if (cachedProgram)
    {
        cachedProgram->setWorldTransforms4x3(data, matrixCount);
    }
}

void DeviceContextBGFX::setConstant(int handle, const float* data, size_t vectorCount)
{
    if (cachedProgram)
    {
        cachedProgram->setConstant(handle, data, vectorCount);
    }
}

void DeviceContextBGFX::bindTexture(unsigned int stage, Texture* texture, const SamplerState& state)
{
    SamplerState realState = (state.getFilter() == SamplerState::Filter_Anisotropic && state.getAnisotropy() == 0)
                                 ? SamplerState(state.getFilter(), state.getAddress(), defaultAnisotropy)
                                 : state;

    AYAASSERT(stage < device->getCaps().maxTextureUnits);
    AYAASSERT(stage < ARRAYSIZE(cachedTextures));

    TextureBGFX* textureBGFX = static_cast<TextureBGFX*>(texture);
    if (textureBGFX != cachedTextures[stage])
    {
        cachedTextures[stage] = textureBGFX;
    }

    // Set texture with proper sampler uniform
    textureBGFX->bindWithUniform(stage, samplerUniforms[stage], realState);
}

void DeviceContextBGFX::setRasterizerState(const RasterizerState& state)
{
    if (cachedRasterizerState != state)
    {
        cachedRasterizerState = state;

        // Update cached state flags
        cachedStateFlags &= ~(BGFX_STATE_CULL_MASK);
        cachedStateFlags |= getCullStateFlags(state.getCullMode());

        // Handle depth bias (polygon offset)
        if (state.getDepthBias() != 0)
        {
            // BGFX doesn't have a direct polygon offset equivalent in state flags
            // This would need to be handled differently, possibly in shader or via uniforms
        }
    }
}

void DeviceContextBGFX::setBlendState(const BlendState& state)
{
    if (cachedBlendState != state)
    {
        cachedBlendState = state;

        // Update cached state flags
        cachedStateFlags &= ~(BGFX_STATE_BLEND_MASK | BGFX_STATE_WRITE_MASK);
        cachedStateFlags |= getBlendStateFlags(state);
        cachedStateFlags |= getColorMaskFlags(state.getColorMask());
    }
}

void DeviceContextBGFX::setDepthState(const DepthState& state)
{
    if (cachedDepthState != state)
    {
        cachedDepthState = state;

        // Update cached state flags
        cachedStateFlags &= ~(BGFX_STATE_DEPTH_TEST_MASK | BGFX_STATE_WRITE_Z);
        cachedStateFlags |= getDepthStateFlags(state);

        // Handle stencil modes - BGFX uses bgfx::setStencil() separately
        // For now, we'll handle basic stencil modes
        // More complex stencil operations would need to be set per-draw call
        switch (state.getStencilMode())
        {
        case DepthState::Stencil_None:
            // No stencil test
            break;

        case DepthState::Stencil_IsNotZero:
        case DepthState::Stencil_UpdateZFail:
        case DepthState::Stencil_Increment:
        case DepthState::Stencil_Decrement:
        case DepthState::Stencil_IsNotZeroReplace:
            // Stencil operations in BGFX require setting stencil per-draw
            // Mark that stencil is needed
            break;

        default:
            AYAASSERT(false);
        }
    }
}

void DeviceContextBGFX::handleStencilState()
{
    // Convert DepthState stencil modes to BGFX stencil operations
    uint32_t frontStencil = BGFX_STENCIL_NONE;
    uint32_t backStencil = BGFX_STENCIL_NONE;

    switch (cachedDepthState.getStencilMode())
    {
    case DepthState::Stencil_None:
        // No stencil test
        break;

    case DepthState::Stencil_IsNotZero:
        frontStencil = BGFX_STENCIL_TEST_NOTEQUAL | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                       BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_KEEP;
        backStencil = frontStencil;
        break;

    case DepthState::Stencil_UpdateZFail:
        // Front faces: decrement on z-fail
        frontStencil = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                       BGFX_STENCIL_OP_FAIL_Z_DECR | BGFX_STENCIL_OP_PASS_Z_KEEP;
        // Back faces: increment on z-fail
        backStencil = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                      BGFX_STENCIL_OP_FAIL_Z_INCR | BGFX_STENCIL_OP_PASS_Z_KEEP;
        break;

    case DepthState::Stencil_Increment:
        // This mode has special requirements - it modifies the state flags
        // Disable color writes, enable front-face culling, increment stencil
        frontStencil = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                       BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_INCR;
        backStencil = frontStencil;

        // Modify state flags: disable color writes, enable culling of front faces
        cachedStateFlags &= ~BGFX_STATE_WRITE_MASK;
        cachedStateFlags &= ~BGFX_STATE_CULL_MASK;
        cachedStateFlags |= BGFX_STATE_CULL_CCW; // Cull front faces
        break;

    case DepthState::Stencil_Decrement:
        // Continue from Increment - now cull back faces and decrement
        frontStencil = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                       BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_DECR;
        backStencil = frontStencil;

        // Modify state flags: disable color writes, enable culling of back faces
        cachedStateFlags &= ~BGFX_STATE_WRITE_MASK;
        cachedStateFlags &= ~BGFX_STATE_CULL_MASK;
        cachedStateFlags |= BGFX_STATE_CULL_CW; // Cull back faces
        break;

    case DepthState::Stencil_IsNotZeroReplace:
        // Test stencil < 1, enable blending and color writes
        frontStencil = BGFX_STENCIL_TEST_LESS | BGFX_STENCIL_FUNC_REF(1) | BGFX_STENCIL_FUNC_RMASK(0xFF) | BGFX_STENCIL_OP_FAIL_S_KEEP |
                       BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_KEEP;
        backStencil = frontStencil;

        // Re-enable color writes, disable culling, enable blending
        cachedStateFlags |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
        cachedStateFlags &= ~BGFX_STATE_CULL_MASK;
        cachedStateFlags |= BGFX_STATE_CULL_CW | BGFX_STATE_CULL_CCW; // Disable culling

        // Enable alpha blending for shadow compositing
        cachedStateFlags &= ~BGFX_STATE_BLEND_MASK;
        cachedStateFlags |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        break;

    default:
        AYAASSERT(false);
    }

    if (frontStencil != BGFX_STENCIL_NONE || backStencil != BGFX_STENCIL_NONE)
    {
        bgfx::setStencil(frontStencil, backStencil);
    }
}

void DeviceContextBGFX::drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count,
    unsigned int indexRangeBegin, unsigned int indexRangeEnd)
{
    AYAASSERT(cachedProgram != nullptr);

    // Update global constants if needed
    if (cachedProgram)
    {
        cachedProgram->updateGlobalConstants(&globalData[0], globalDataVersion);
    }

    // Set the accumulated state flags
    uint64_t stateFlags = cachedStateFlags;

    // Ensure we always write to RGB and A unless masked
    if ((stateFlags & BGFX_STATE_WRITE_MASK) == 0)
    {
        stateFlags |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
    }

    bgfx::setState(stateFlags);

    // Handle stencil state separately
    handleStencilState();

    // Set vertex and index buffers - GeometryBGFX handles binding
    GeometryBGFX* geometryBGFX = static_cast<GeometryBGFX*>(geometry);
    geometryBGFX->bindBuffers(offset, count);

    // CRITICAL: Actually submit the draw call with the shader program
    // This is what actually triggers rendering in BGFX!
    bgfx::submit(currentView, cachedProgram->getHandle());
}

void DeviceContextBGFX::pushDebugMarkerGroup(const char* text)
{
    bgfx::setViewName(currentView, text);
}

void DeviceContextBGFX::popDebugMarkerGroup()
{
    // BGFX doesn't have a direct equivalent to pop marker groups
    // Markers are set per-view
}

void DeviceContextBGFX::setDebugMarker(const char* text)
{
    bgfx::setMarker(text);
}

} // namespace Graphics
} // namespace Aya
