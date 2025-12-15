

#include "DataModel/TextLabel.hpp"
#include "Players.hpp"

DYNAMIC_FASTFLAGVARIABLE(TextTransparencyRenderingFix, false)

namespace Aya
{

const char* const sTextLabel = "TextLabel";

TextLabel::TextLabel()
    : DescribedCreatable<TextLabel, GuiLabel, sTextLabel>("TextLabel")
    , GuiTextMixin("Label", BrickColor::brickBlack().color3())
{
    setGuiQueue(GUIQUEUE_TEXT);
}

IMPLEMENT_GUI_TEXT_MIXIN(TextLabel);

void TextLabel::render2d(Adorn* adorn)
{
    render2dContext(adorn, NULL);
}

void TextLabel::render2dContext(Adorn* adorn, const Instance* context)
{
    if (filterState == ContentFilter::Waiting)
        if (ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(this))
            filterState = contentFilter->getStringState(text);

    if (filterState != ContentFilter::Succeeded)
        if (ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(context))
            filterState = contentFilter->getStringState(text);

    if (filterState == ContentFilter::Succeeded)
    {
        // aya: This implementation of TextSize fucking sucks
        if (getRenderTextAlpha(getTextTransparency()) > 0.0f)
        {
            Rect2D rect2D = getRect2D();

            if (getTextScale())
            {
                if (getTextSize() <= 0 || getTextSize() > (pow(10, 100)))
                {
                    render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), getFont(),
                        getScaledFontSize(rect2D, getText(), getFont(), getTextWrap(), getFontSize()), getRenderTextColor4(),
                        getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());
                }
                else
                {
                    render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), getFont(),
                        getScaledFontSize(rect2D, getText(), getFont(), getTextWrap(), getTextSize()), getRenderTextColor4(),
                        getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());
                }
            }
            else if (getFontSize())
            {
                render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), getFont(), getFontSize(), getRenderTextColor4(),
                    getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());
            }
            else if (getTextSize())
            {
                if (getTextSize() <= 0 || getTextSize() > (pow(10, 100)))
                {
                    render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), getFont(), getFontSize(), getRenderTextColor4(),
                        getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());
                }
                else
                {
                    render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), getFont(), getTextSize() * 0.55, getRenderTextColor4(),
                        getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());
                }
            }
        }
    }

    renderStudioSelectionBox(adorn);
}

} // namespace Aya