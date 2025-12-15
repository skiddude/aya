#pragma once

#include "Core/Device.hpp"
#include "Core/States.hpp"

#include <bgfx/bgfx.h>

namespace Aya
{
namespace Graphics
{

class FramebufferBGFX;
class ShaderProgramBGFX;
class TextureBGFX;
class DeviceBGFX;

struct DeviceCapsBGFX : DeviceCaps
{
    const bgfx::Caps* caps;

    bool supportsCompute;
    bool supportsInstancing;
    bool supportsTextureBlits;
    bool supportsTextureReadBack;
    bool supportsOcclusionQuery;
};

class DeviceContextBGFX : public DeviceContext
{
public:
    DeviceContextBGFX(DeviceBGFX* dev);
    ~DeviceContextBGFX();

    void defineGlobalConstants(size_t dataSize);

    void clearStates();

    void invalidateCachedProgram();
    void invalidateCachedTexture(Texture* texture);
    void invalidateCachedTextureStage(unsigned int stage);

    void handleStencilState();

    virtual void setDefaultAnisotropy(unsigned int value);

    virtual void updateGlobalConstants(const void* data, size_t dataSize);

    virtual void bindFramebuffer(Framebuffer* buffer);
    virtual void clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil);

    virtual void copyFramebuffer(Framebuffer* buffer, Texture* texture, int xOffset, int yOffset);
    virtual void resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask);
    virtual void discardFramebuffer(Framebuffer* buffer, unsigned int mask);

    virtual void bindProgram(ShaderProgram* program);
    virtual void setWorldTransforms4x3(const float* data, size_t matrixCount);
    virtual void setConstant(int handle, const float* data, size_t vectorCount);

    virtual void bindTexture(unsigned int stage, Texture* texture, const SamplerState& state);

    virtual void setRasterizerState(const RasterizerState& state);
    virtual void setBlendState(const BlendState& state);
    virtual void setDepthState(const DepthState& state);

    virtual void drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin,
        unsigned int indexRangeEnd);

    virtual void pushDebugMarkerGroup(const char* text);
    virtual void popDebugMarkerGroup();
    virtual void setDebugMarker(const char* text);

    bgfx::ViewId getCurrentView() const
    {
        return currentView;
    }
    void setCurrentView(bgfx::ViewId view)
    {
        currentView = view;
    }

private:
    std::vector<char> globalData;
    bgfx::UniformHandle globalUniform;
    unsigned int globalDataVersion;

    unsigned int defaultAnisotropy;

    ShaderProgramBGFX* cachedProgram;
    TextureBGFX* cachedTextures[16];

    // Sampler uniforms for each texture stage
    bgfx::UniformHandle samplerUniforms[16];

    RasterizerState cachedRasterizerState;
    BlendState cachedBlendState;
    DepthState cachedDepthState;

    uint64_t cachedStateFlags;

    bgfx::ViewId currentView;

    DeviceBGFX* device;
};

class DeviceBGFX : public Device
{
public:
    DeviceBGFX(void* windowHandle, void* displayHandle);
    ~DeviceBGFX();

    virtual bool validate();
    virtual void resize(int w, int h);
    virtual DeviceContext* beginFrame();
    virtual void endFrame();

    virtual Framebuffer* getMainFramebuffer();

    virtual void defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants);

    virtual std::string getAPIName();
    virtual std::string getFeatureLevel();
    virtual std::string getShadingLanguage();
    virtual std::string createShaderSource(
        const std::string& path, const std::string& defines, boost::function<std::string(const std::string&)> fileCallback);
    virtual std::vector<char> createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint);

    virtual shared_ptr<VertexShader> createVertexShader(const std::vector<char>& bytecode);
    virtual shared_ptr<FragmentShader> createFragmentShader(const std::vector<char>& bytecode);
    virtual shared_ptr<ShaderProgram> createShaderProgram(
        const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
    virtual shared_ptr<ShaderProgram> createShaderProgramFFP();

    virtual shared_ptr<VertexBuffer> createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage);
    virtual shared_ptr<IndexBuffer> createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage);
    virtual shared_ptr<VertexLayout> createVertexLayout(const std::vector<VertexLayout::Element>& elements);

    virtual shared_ptr<Texture> createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth,
        unsigned int mipLevels, Texture::Usage usage);

    virtual shared_ptr<Renderbuffer> createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);

    virtual shared_ptr<Geometry> createGeometryImpl(const shared_ptr<VertexLayout>& layout,
        const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);

    virtual shared_ptr<Framebuffer> createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer>>& color, const shared_ptr<Renderbuffer>& depth);

    virtual const DeviceCaps& getCaps() const
    {
        return caps;
    }

    virtual DeviceStats getStatistics() const;

    DeviceContextBGFX* getImmediateContextBGFX()
    {
        return immediateContext.get();
    }

    const DeviceCapsBGFX& getCapsBGFX() const
    {
        return caps;
    }

    const std::vector<ShaderGlobalConstant>& getGlobalConstants() const
    {
        return globalConstants;
    }

    bgfx::RendererType::Enum getRendererType() const
    {
        return rendererType;
    }

private:
    DeviceCapsBGFX caps;

    scoped_ptr<DeviceContextBGFX> immediateContext;

    scoped_ptr<FramebufferBGFX> mainFramebuffer;

    std::vector<ShaderGlobalConstant> globalConstants;

    bgfx::RendererType::Enum rendererType;

    uint32_t width;
    uint32_t height;

    uint32_t frameNumber;

    float gpuTime;
};

} // namespace Graphics
} // namespace Aya