#pragma once

#include "DataModel/GuiObject.hpp"
#include "DataModel/GuiMixin.hpp"

namespace Aya
{
extern const char* const sImageLabel;

class ImageLabel
    : public DescribedCreatable<ImageLabel, GuiLabel, sImageLabel>
    , public GuiImageMixin
{
public:
    ImageLabel();

    DECLARE_GUI_IMAGE_MIXIN(ImageLabel);

private:
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // IAdornable
    /*override*/ void render2d(Adorn* adorn);
    /*override*/ void renderBackground2d(Adorn* adorn);
};
} // namespace Aya
