
#pragma once

#include "Utility/G3DCore.hpp"

namespace G3D
{
class RenderDevice;
}

namespace Aya
{
class Rect;

class DrawPrimitives
{
public:
    static void rawBox(const AABox& box, G3D::RenderDevice* rd);

    static void rawSphere(float radius, G3D::RenderDevice* rd);

    static void rawCylinderAlongX(float radius, float axis, G3D::RenderDevice* rd, bool cap);

    // generic 2d - must be in 2d mode
    static void rect2d(const Aya::Rect& rect, G3D::RenderDevice* rd, const G3D::Color4& color = G3D::Color3::white());

    static void line2d(const G3D::Vector2& p0, const G3D::Vector2& p1, G3D::RenderDevice* rd, const G3D::Color4& color = G3D::Color3::white());

    static void outlineRect2d(const Aya::Rect& rect, float thick, G3D::RenderDevice* rd, const G3D::Color4& color = G3D::Color3::blue());
};
} // namespace Aya