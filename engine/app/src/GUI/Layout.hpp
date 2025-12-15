

#pragma once

#include "Utility/Rect.hpp"
#include "Utility/G3DCore.hpp"

namespace Aya
{

class Layout
{
public:
    enum Style
    {
        HORIZONTAL = 0,
        VERTICAL = 1
    };

    Rect::Location xLocation;
    Rect::Location yLocation;
    Vector2int16 offset;
    Style layoutStyle;
    Color4 backdropColor;

    Layout()
        : xLocation(Rect::LEFT)
        , yLocation(Rect::TOP)
        , offset(Vector2int16(0, 0))
        , layoutStyle(HORIZONTAL)
        , backdropColor(Color4::clear())
    {
    }
};
} // namespace Aya
