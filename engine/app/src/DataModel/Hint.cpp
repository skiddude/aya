

#include "DataModel/Hint.hpp"
#include "Utility/G3DCore.hpp"
#include "Utility/Rect.hpp"
#include "DrawPrimitives.hpp"
#include "Base/Adorn.hpp"
#include "Player.hpp"

namespace Aya
{

const char* const sHint = "Hint";


static Reflection::PropDescriptor<Hint, std::string> desc_Text("Text", category_Appearance, &Hint::getText, &Hint::setText);
REFLECTION_END();

Hint::Hint()
    : DescribedCreatable<Hint, Instance, sHint>("Message")
{
}

Hint::~Hint() {}

void Hint::setText(const std::string& value)
{
    if (text != value)
    {
        text = value;
        this->raiseChanged(desc_Text);
    }
}

const std::string& Hint::getText() const
{
    return text;
}

void Hint::render2d(Adorn* adorn)
{
    Rect2D rect = adorn->getUserGuiRect();
    Rect blackArea = Rect(rect.x0(), rect.y0(), rect.x1(), rect.y0() + 20);
    Vector2 textPos = blackArea.center();

    adorn->rect2d(blackArea.toRect2D(), Color3::black());

    adorn->drawFont2D(text, textPos,
        14, // size
        false, Color3::white(), Color4::clear(), Text::FONT_LEGACY, Text::XALIGN_CENTER, Text::YALIGN_CENTER);
}

} // namespace Aya
