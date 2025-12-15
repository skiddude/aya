#include "DeviceBGFX.hpp"

#include "GeometryBGFX.hpp"
#include "ShaderBGFX.hpp"
#include "TextureBGFX.hpp"
#include "FramebufferBGFX.hpp"

#include "Profiler.hpp"
#include "ImGui.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <sstream>

#ifdef __linux
#include <wayland-client.h>
#include <X11/Xlib.h>
#endif

LOGGROUP(Graphics)

FASTFLAGVARIABLE(DebugGraphicsBGFX, false)

namespace Aya
{
namespace Graphics
{

static DeviceCapsBGFX createDeviceCaps()
{
    const bgfx::Caps* bgfxCaps = bgfx::getCaps();

    DeviceCapsBGFX caps;
    caps.caps = bgfxCaps;

    caps.supportsFramebuffer = true;
    caps.supportsShaders = true;
    caps.supportsFFP = false;
    caps.supportsStencil = true;
    caps.supportsIndex32 = bgfxCaps->supported & BGFX_CAPS_INDEX32;

    caps.supportsTextureDXT = (bgfxCaps->formats[bgfx::TextureFormat::BC1] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    caps.supportsTexturePVR = (bgfxCaps->formats[bgfx::TextureFormat::PTC12] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    caps.supportsTextureHalfFloat = (bgfxCaps->formats[bgfx::TextureFormat::R16F] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    caps.supportsTexture3D = (bgfxCaps->supported & BGFX_CAPS_TEXTURE_3D) != 0;
    caps.supportsTextureNPOT = true;
    caps.supportsTextureETC1 = (bgfxCaps->formats[bgfx::TextureFormat::ETC1] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0;
    caps.supportsTexturePartialMipChain = true;

    caps.maxDrawBuffers = bgfxCaps->limits.maxFBAttachments;
    caps.maxSamples = 1; // BGFX doesn't expose max MSAA samples directly
    caps.maxTextureSize = bgfxCaps->limits.maxTextureSize;
    caps.maxTextureUnits = bgfxCaps->limits.maxTextureSamplers;

    caps.colorOrderBGR = bgfxCaps->rendererType == bgfx::RendererType::Direct3D11 || bgfxCaps->rendererType == bgfx::RendererType::Direct3D12;
    caps.requiresRenderTargetFlipping =
        bgfxCaps->rendererType == bgfx::RendererType::OpenGL || bgfxCaps->rendererType == bgfx::RendererType::OpenGLES;

    caps.retina = false;

    caps.supportsCompute = (bgfxCaps->supported & BGFX_CAPS_COMPUTE) != 0;
    caps.supportsInstancing = (bgfxCaps->supported & BGFX_CAPS_INSTANCING) != 0;
    caps.supportsTextureBlits = (bgfxCaps->supported & BGFX_CAPS_TEXTURE_BLIT) != 0;
    caps.supportsTextureReadBack = (bgfxCaps->supported & BGFX_CAPS_TEXTURE_READ_BACK) != 0;
    caps.supportsOcclusionQuery = (bgfxCaps->supported & BGFX_CAPS_OCCLUSION_QUERY) != 0;

    return caps;
}

enum class DisplayBackend
{
    Wayland,
    X11,
    Unknown
};

static DisplayBackend detectDisplayBackend()
{
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    const char* x11Display = std::getenv("DISPLAY");

    if (waylandDisplay && waylandDisplay[0])
        return DisplayBackend::Wayland;
    if (x11Display && x11Display[0])
        return DisplayBackend::X11;
    return DisplayBackend::Unknown;
}

DeviceBGFX::DeviceBGFX(void* windowHandle, void* displayHandle)
    : width(0)
    , height(0)
    , frameNumber(0)
{
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // Auto-select
    init.platformData.nwh = windowHandle;

#ifdef __linux__
    DisplayBackend backend = detectDisplayBackend();
    if (backend == DisplayBackend::Wayland)
    {
        init.platformData.ndt = static_cast<wl_display*>(displayHandle);
        init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
    }
    else if (backend == DisplayBackend::X11)
    {
        init.platformData.ndt = static_cast<::Display*>(displayHandle);
        init.platformData.type = bgfx::NativeWindowHandleType::Default; // X11 is default on Linux
    }
#endif

    if (!bgfx::init(init))
    {
        throw Aya::runtime_error("Failed to initialize BGFX");
    }

    rendererType = bgfx::getCaps()->rendererType;
    caps = createDeviceCaps();

    const char* rendererName = bgfx::getRendererName(rendererType);
    FASTLOGS(FLog::Graphics, "BGFX Renderer: %s", rendererName);

    caps.dumpToFLog(FLog::Graphics);

    FASTLOG5(FLog::Graphics, "Caps: Compute %d Instancing %d TextureBlits %d TextureReadBack %d OcclusionQuery %d", caps.supportsCompute,
        caps.supportsInstancing, caps.supportsTextureBlits, caps.supportsTextureReadBack, caps.supportsOcclusionQuery);

    immediateContext.reset(new DeviceContextBGFX(this));
    mainFramebuffer.reset(new FramebufferBGFX(this, BGFX_INVALID_HANDLE));

    bgfx::setDebug(FFlag::DebugGraphicsBGFX ? BGFX_DEBUG_TEXT : BGFX_DEBUG_NONE);

#if defined(_WIN32) || defined(__linux__)
    Profiler::gpuInit(0);
#endif
    ImGui::gpuInit();
}

DeviceBGFX::~DeviceBGFX()
{
#if defined(_WIN32) || defined(__linux__)
    Profiler::gpuShutdown();
#endif
    ImGui::gpuShutdown();

    immediateContext.reset();
    mainFramebuffer.reset();

    bgfx::shutdown();
}

void DeviceBGFX::resize(int w, int h)
{
    width = w;
    height = h;

    bgfx::reset(width, height, BGFX_RESET_VSYNC);
}

std::string DeviceBGFX::getAPIName()
{
    return bgfx::getRendererName(rendererType);
}

std::string DeviceBGFX::getFeatureLevel()
{
    std::ostringstream oss;
    oss << bgfx::getRendererName(rendererType);
    return oss.str();
}

bool DeviceBGFX::validate()
{
    // BGFX handles window resizing internally
    return true;
}

DeviceContext* DeviceBGFX::beginFrame()
{
    immediateContext->setCurrentView(0);
    immediateContext->bindFramebuffer(mainFramebuffer.get());
    immediateContext->clearStates();

    return immediateContext.get();
}

void DeviceBGFX::endFrame()
{
    bgfx::frame();
    frameNumber++;
}

Framebuffer* DeviceBGFX::getMainFramebuffer()
{
    return mainFramebuffer.get();
}

void DeviceBGFX::defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants)
{
    AYAASSERT(globalConstants.empty());
    AYAASSERT(!constants.empty());

    globalConstants = constants;

    immediateContext->defineGlobalConstants(dataSize);
}


std::string DeviceBGFX::createShaderSource(
    const std::string& path, const std::string& defines, boost::function<std::string(const std::string&)> fileCallback)
{
    return fileCallback(path);
}

std::vector<char> DeviceBGFX::createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint)
{
    AYAASSERT(false && "BGFX shaders must be pre-compiled");
    return std::vector<char>();
}


std::string DeviceBGFX::getShadingLanguage()
{
    switch (rendererType)
    {
    case bgfx::RendererType::Direct3D11:
        return "dx12";
    case bgfx::RendererType::Direct3D12:
        return "dx11";
    case bgfx::RendererType::Metal:
        return "metal";
    case bgfx::RendererType::OpenGL:
        return "glsl";
    case bgfx::RendererType::OpenGLES:
        return "glsles";
    case bgfx::RendererType::Vulkan:
        return "spirv";
    default:
        return "unknown";
    }
}

shared_ptr<VertexShader> DeviceBGFX::createVertexShader(const std::vector<char>& bytecode)
{
    return shared_ptr<VertexShader>(new VertexShaderBGFX(this, bytecode));
}

shared_ptr<FragmentShader> DeviceBGFX::createFragmentShader(const std::vector<char>& bytecode)
{
    return shared_ptr<FragmentShader>(new FragmentShaderBGFX(this, bytecode));
}

shared_ptr<ShaderProgram> DeviceBGFX::createShaderProgram(
    const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
{
    return shared_ptr<ShaderProgram>(new ShaderProgramBGFX(this, vertexShader, fragmentShader));
}

shared_ptr<ShaderProgram> DeviceBGFX::createShaderProgramFFP()
{
    throw Aya::runtime_error("No FFP support in BGFX");
}

shared_ptr<VertexBuffer> DeviceBGFX::createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
{
    return shared_ptr<VertexBuffer>(new VertexBufferBGFX(this, elementSize, elementCount, usage));
}

shared_ptr<IndexBuffer> DeviceBGFX::createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
{
    return shared_ptr<IndexBuffer>(new IndexBufferBGFX(this, elementSize, elementCount, usage));
}

shared_ptr<VertexLayout> DeviceBGFX::createVertexLayout(const std::vector<VertexLayout::Element>& elements)
{
    return shared_ptr<VertexLayout>(new VertexLayoutBGFX(this, elements));
}

shared_ptr<Texture> DeviceBGFX::createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth,
    unsigned int mipLevels, Texture::Usage usage)
{
    return shared_ptr<Texture>(new TextureBGFX(this, type, format, width, height, depth, mipLevels, usage));
}

shared_ptr<Renderbuffer> DeviceBGFX::createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
{
    return shared_ptr<Renderbuffer>(new RenderbufferBGFX(this, format, width, height, samples));
}

shared_ptr<Geometry> DeviceBGFX::createGeometryImpl(const shared_ptr<VertexLayout>& layout,
    const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
{
    return shared_ptr<Geometry>(new GeometryBGFX(this, layout, vertexBuffers, indexBuffer, baseVertexIndex));
}

shared_ptr<Framebuffer> DeviceBGFX::createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer>>& color, const shared_ptr<Renderbuffer>& depth)
{
    return shared_ptr<Framebuffer>(new FramebufferBGFX(this, color, depth));
}

DeviceStats DeviceBGFX::getStatistics() const
{
    DeviceStats result = {};

    const bgfx::Stats* stats = bgfx::getStats();

    result.gpuFrameTime = static_cast<float>(stats->gpuTimeEnd - stats->gpuTimeBegin) / stats->gpuTimerFreq * 1000.0f;

    return result;
}

} // namespace Graphics
} // namespace Aya