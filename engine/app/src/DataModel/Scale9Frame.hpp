

#pragma once

#include "DataModel/Frame.hpp"
#include "Utility/TextureId.hpp"
#include "GUI/GuiDraw.hpp"

namespace Aya
{
extern const char* const sScale9Frame;

class Scale9Frame : public DescribedNonCreatable<Scale9Frame, GuiObject, sScale9Frame>
{
private:
    typedef DescribedNonCreatable<Scale9Frame, GuiObject, sScale9Frame> Super;

    Vector2int16 scaleEdgeSize;
    std::string slicePrefix;
    GuiDrawImage image;

public:
    Scale9Frame();

    std::string getSlicePrefix() const
    {
        return slicePrefix;
    }
    void setSlicePrefix(std::string value);

    Vector2int16 getScaleEdgeSize() const
    {
        return scaleEdgeSize;
    }
    void setScaleEdgeSize(Vector2int16 value);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // IAdornable
    /*override*/ void render2d(Adorn* adorn);
};
} // namespace Aya