


#include "DataModel/Message.hpp"
#include "Utility/G3DCore.hpp"
#include "Utility/Rect.hpp"
#include "DrawPrimitives.hpp"
#include "Base/Adorn.hpp"


#include "Player.hpp"

namespace Aya
{

const char* const sMessage = "Message";


static Reflection::PropDescriptor<Message, std::string> desc_Text("Text", category_Appearance, &Message::getText, &Message::setText);
REFLECTION_END();

Message::Message()
    : DescribedCreatable<Message, Instance, sMessage>("Message")
{
}

Message::~Message() {}

void Message::setText(const std::string& value)
{
    if (text != value)
    {
        text = value;
        this->raiseChanged(desc_Text);
    }
}

const std::string& Message::getText() const
{
    return text;
}

void Message::renderFullScreen(Adorn* adorn)
{
    Rect2D rect = adorn->getUserGuiRect();

    Rect translucentArea = rect;

    translucentArea.inset(100);

    Vector2 textPos = translucentArea.center();

    adorn->rect2d(translucentArea.toRect2D(), Color4(0.5, 0.5, 0.5, 0.5));

    adorn->drawFont2D(text, textPos,
        14, // size
        false, Color3::white(), Color3::black(), Text::FONT_LEGACY, Text::XALIGN_CENTER, Text::YALIGN_CENTER);
}

void Message::render2d(Adorn* adorn)
{
    if (text.size() > 0)
    {
        renderFullScreen(adorn);
    }
}

} // namespace Aya
