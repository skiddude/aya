#pragma once

#include "GlobalShaderData.hpp"
#include "Core/Resource.hpp"
#include "boost.hpp"

#include "DataModel/Lighting.hpp"

#include "TextureRef.hpp"

namespace Aya
{
namespace Graphics
{
class VisualEngine;
class DeviceContext;
class Framebuffer;
class BlendState;
class RasterizerState;
class ShaderProgram;

class ScreenSpaceEffect
{
public:
    static ShaderProgram* renderFullscreenBegin(DeviceContext* context, VisualEngine* visualEngine, const char* vsName, const char* fsName,
        const BlendState& blendState, unsigned fbWidth, unsigned fbHeight);
    static void renderFullscreenEnd(DeviceContext* context, VisualEngine* visualEngine);

    static void renderBlur(DeviceContext* context, VisualEngine* visualEngine, Framebuffer* fb, Framebuffer* intermediateFB, Texture* texture,
        Texture* intermediate, float strength);
    static void renderFXAA(
        DeviceContext* context, VisualEngine* visualEngine, Framebuffer* fb, Framebuffer* intermediateFB, Texture* texture, Texture* intermediate);
};

} // namespace Graphics
} // namespace Aya
