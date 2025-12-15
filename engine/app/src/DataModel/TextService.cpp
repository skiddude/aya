

#include "DataModel/TextService.hpp"

FASTFLAGVARIABLE(TypesettersReleaseResources, true);

namespace Aya
{
const char* const sTextService = "TextService";

namespace Reflection
{
template<>
EnumDesc<TextService::FontSize>::EnumDesc()
    : EnumDescriptor("FontSize")
{
    addPair(TextService::SIZE_8, "Size8");
    addPair(TextService::SIZE_9, "Size9");
    addPair(TextService::SIZE_10, "Size10");
    addPair(TextService::SIZE_11, "Size11");
    addPair(TextService::SIZE_12, "Size12");
    addPair(TextService::SIZE_14, "Size14");
    addPair(TextService::SIZE_18, "Size18");
    addPair(TextService::SIZE_24, "Size24");
    addPair(TextService::SIZE_36, "Size36");
    addPair(TextService::SIZE_48, "Size48");
    addPair(TextService::SIZE_28, "Size28");
    addPair(TextService::SIZE_32, "Size32");
    addPair(TextService::SIZE_42, "Size42");
    addPair(TextService::SIZE_60, "Size60");
    addPair(TextService::SIZE_96, "Size96");
}


template<>
EnumDesc<TextService::Font>::EnumDesc()
    : EnumDescriptor("Font")
{
    addPair(TextService::FONT_LEGACY, "Legacy");
    addPair(TextService::FONT_ARIAL, "Arial");
    addPair(TextService::FONT_ARIALBOLD, "ArialBold");
    addPair(TextService::FONT_SOURCESANS, "SourceSans");
    addPair(TextService::FONT_SOURCESANSBOLD, "SourceSansBold");
    addPair(TextService::FONT_SOURCESANSLIGHT, "SourceSansLight");
    addPair(TextService::FONT_SOURCESANSITALIC, "SourceSansItalic");
    addPair(TextService::FONT_BODONI, "Bodoni");
    addPair(TextService::FONT_GARAMOND, "Garamond");
    addPair(TextService::FONT_CARTOON, "Cartoon");
    addPair(TextService::FONT_CODE, "Code");
    addPair(TextService::FONT_HIGHWAY, "Highway");
    addPair(TextService::FONT_SCIFI, "SciFi");
    addPair(TextService::FONT_ARCADE, "Arcade");
    addPair(TextService::FONT_FANTASY, "Fantasy");
    addPair(TextService::FONT_ANTIQUE, "Antique");
    addPair(TextService::FONT_SOURCESANSSEMIBOLD, "SourceSansSemiBold");
    addPair(TextService::FONT_GOTHAM, "Gotham");
    addPair(TextService::FONT_GOTHAMSEMIBOLD, "GothamSemiBold");
    addPair(TextService::FONT_GOTHAMBOLD, "GothamBold");
    addPair(TextService::FONT_GOTHAMBLACK, "GothamBlack");
    addPair(TextService::FONT_AMATICSC, "AmaticSC");
    addPair(TextService::FONT_BANGERS, "Bangers");
    addPair(TextService::FONT_CREEPSTER, "Creepster");
    addPair(TextService::FONT_DENKONE, "Denkone");
    addPair(TextService::FONT_FONDAMENTO, "Fondamento");
    addPair(TextService::FONT_FREDOKAONE, "FredokaOne");
    addPair(TextService::FONT_GRENZEGOTISCH, "GrenzeGotisch");
    addPair(TextService::FONT_INDIEFLOWER, "IndieFlower");
    addPair(TextService::FONT_JOSEFINSANS, "JosefinSans");
    addPair(TextService::FONT_JURA, "Jura");
    addPair(TextService::FONT_KALAM, "Kalam");
    addPair(TextService::FONT_LUCKIESTGUY, "LuckiestGuy");
    addPair(TextService::FONT_MERRIWEATHER, "Merriweather");
    addPair(TextService::FONT_MICHROMA, "Michroma");
    addPair(TextService::FONT_NUNITO, "Nunito");
    addPair(TextService::FONT_OSWALD, "Oswald");
    addPair(TextService::FONT_PATRICKHAND, "PatrickHand");
    addPair(TextService::FONT_PERMANENTMARKER, "PermanentMarker");
    addPair(TextService::FONT_ROBOTO, "Roboto");
    addPair(TextService::FONT_ROBOTOCONDENSED, "RobotoCondensed");
    addPair(TextService::FONT_ROBOTOMONO, "RobotoMono");
    addPair(TextService::FONT_SARPANCH, "Sarpanch");
    addPair(TextService::FONT_SPECIALELITE, "SpecialElite");
    addPair(TextService::FONT_TITILLIUMWEB, "TitilliumWeb");
    addPair(TextService::FONT_UBUNTU, "Ubuntu");
    addPair(TextService::FONT_NOTOSANS, "NotoSans");
    addPair(TextService::FONT_NOTOSANSMEDIUM, "NotoSansMedium");
    addPair(TextService::FONT_NOTOSANSSEMIBOLD, "NotoSansSemiBold");
    addPair(TextService::FONT_NOTOSANSBOLD, "NotoSansBold");
    addPair(TextService::FONT_NOTOSANSEXTRABOLD, "NotoSansExtraBold");
    addPair(TextService::FONT_NOTOSANSBLACK, "NotoSansBlack");
    addPair(TextService::FONT_NOTOSANSITALIC, "NotoSansItalic");
    addPair(TextService::FONT_NOTOSANSLIGHT, "NotoSansLight");
    addPair(TextService::FONT_NOTOSANSEXTRALIGHT, "NotoSansExtraLight");
    addPair(TextService::FONT_NOTOSANSTHIN, "NotoSansThin");
    addPair(TextService::FONT_CONSOLAS, "Consolas");
    addPair(TextService::FONT_COMICSANS, "ComicSans");
}

template<>
TextService::Font& Variant::convert<TextService::Font>(void)
{
    return genericConvert<TextService::Font>();
}

template<>
EnumDesc<TextService::XAlignment>::EnumDesc()
    : EnumDescriptor("TextXAlignment")
{
    addPair(TextService::XALIGNMENT_LEFT, "Left");
    addPair(TextService::XALIGNMENT_CENTER, "Center");
    addPair(TextService::XALIGNMENT_RIGHT, "Right");
}

template<>
EnumDesc<TextService::YAlignment>::EnumDesc()
    : EnumDescriptor("TextYAlignment")
{
    addPair(TextService::YALIGNMENT_TOP, "Top");
    addPair(TextService::YALIGNMENT_CENTER, "Center");
    addPair(TextService::YALIGNMENT_BOTTOM, "Bottom");
}
} // namespace Reflection

template<>
bool StringConverter<TextService::Font>::convertToValue(const std::string& text, TextService::Font& value)
{
    return Reflection::EnumDesc<TextService::Font>::singleton().convertToValue(text.c_str(), value);
}

static Reflection::BoundFuncDesc<TextService, Vector2(std::string, int, TextService::Font, Vector2)> func_getTextSize(
    &TextService::getTextSize, "GetTextSize", "string", "fontSize", "font", "frameSize", Security::RobloxScript);

TextService::Font TextService::FromTextFont(Text::Font font)
{
    switch (font)
    {
    case Text::FONT_LEGACY:
        return FONT_LEGACY;
    case Text::FONT_ARIAL:
        return FONT_ARIAL;
    case Text::FONT_ARIALBOLD:
        return FONT_ARIALBOLD;
    case Text::FONT_SOURCESANS:
        return FONT_SOURCESANS;
    case Text::FONT_SOURCESANSBOLD:
        return FONT_SOURCESANSBOLD;
    case Text::FONT_SOURCESANSLIGHT:
        return FONT_SOURCESANSLIGHT;
    case Text::FONT_SOURCESANSITALIC:
        return FONT_SOURCESANSITALIC;
    case Text::FONT_BODONI:
        return FONT_BODONI;
    case Text::FONT_GARAMOND:
        return FONT_GARAMOND;
    case Text::FONT_CARTOON:
        return FONT_CARTOON;
    case Text::FONT_CODE:
        return FONT_CODE;
    case Text::FONT_HIGHWAY:
        return FONT_HIGHWAY;
    case Text::FONT_SCIFI:
        return FONT_SCIFI;
    case Text::FONT_ARCADE:
        return FONT_ARCADE;
    case Text::FONT_FANTASY:
        return FONT_FANTASY;
    case Text::FONT_ANTIQUE:
        return FONT_ANTIQUE;
    case Text::FONT_SOURCESANSSEMIBOLD:
        return FONT_SOURCESANSSEMIBOLD;
    case Text::FONT_GOTHAM:
        return FONT_GOTHAM;
    case Text::FONT_GOTHAMSEMIBOLD:
        return FONT_GOTHAMSEMIBOLD;
    case Text::FONT_GOTHAMBOLD:
        return FONT_GOTHAMBOLD;
    case Text::FONT_GOTHAMBLACK:
        return FONT_GOTHAMBLACK;
    case Text::FONT_AMATICSC:
        return FONT_AMATICSC;
    case Text::FONT_BANGERS:
        return FONT_BANGERS;
    case Text::FONT_CREEPSTER:
        return FONT_CREEPSTER;
    case Text::FONT_DENKONE:
        return FONT_DENKONE;
    case Text::FONT_FONDAMENTO:
        return FONT_FONDAMENTO;
    case Text::FONT_FREDOKAONE:
        return FONT_FREDOKAONE;
    case Text::FONT_GRENZEGOTISCH:
        return FONT_GRENZEGOTISCH;
    case Text::FONT_INDIEFLOWER:
        return FONT_INDIEFLOWER;
    case Text::FONT_JOSEFINSANS:
        return FONT_JOSEFINSANS;
    case Text::FONT_JURA:
        return FONT_JURA;
    case Text::FONT_KALAM:
        return FONT_KALAM;
    case Text::FONT_LUCKIESTGUY:
        return FONT_LUCKIESTGUY;
    case Text::FONT_MERRIWEATHER:
        return FONT_MERRIWEATHER;
    case Text::FONT_MICHROMA:
        return FONT_MICHROMA;
    case Text::FONT_NUNITO:
        return FONT_NUNITO;
    case Text::FONT_OSWALD:
        return FONT_OSWALD;
    case Text::FONT_PATRICKHAND:
        return FONT_PATRICKHAND;
    case Text::FONT_PERMANENTMARKER:
        return FONT_PERMANENTMARKER;
    case Text::FONT_ROBOTO:
        return FONT_ROBOTO;
    case Text::FONT_ROBOTOCONDENSED:
        return FONT_ROBOTOCONDENSED;
    case Text::FONT_ROBOTOMONO:
        return FONT_ROBOTOMONO;
    case Text::FONT_SARPANCH:
        return FONT_SARPANCH;
    case Text::FONT_SPECIALELITE:
        return FONT_SPECIALELITE;
    case Text::FONT_TITILLIUMWEB:
        return FONT_TITILLIUMWEB;
    case Text::FONT_UBUNTU:
        return FONT_UBUNTU;
    case Text::FONT_NOTOSANS:
        return FONT_NOTOSANS;
    case Text::FONT_NOTOSANSMEDIUM:
        return FONT_NOTOSANSMEDIUM;
    case Text::FONT_NOTOSANSSEMIBOLD:
        return FONT_NOTOSANSSEMIBOLD;
    case Text::FONT_NOTOSANSBOLD:
        return FONT_NOTOSANSBOLD;
    case Text::FONT_NOTOSANSEXTRABOLD:
        return FONT_NOTOSANSEXTRABOLD;
    case Text::FONT_NOTOSANSBLACK:
        return FONT_NOTOSANSBLACK;
    case Text::FONT_NOTOSANSITALIC:
        return FONT_NOTOSANSITALIC;
    case Text::FONT_NOTOSANSLIGHT:
        return FONT_NOTOSANSLIGHT;
    case Text::FONT_NOTOSANSEXTRALIGHT:
        return FONT_NOTOSANSEXTRALIGHT;
    case Text::FONT_NOTOSANSTHIN:
        return FONT_NOTOSANSTHIN;
    case Text::FONT_CONSOLAS:
        return FONT_CONSOLAS;
    case Text::FONT_COMICSANS:
        return FONT_COMICSANS;
    default:
        AYAASSERT(0);
        return FONT_LEGACY;
    }
}
Text::Font TextService::ToTextFont(Font font)
{
    switch (font)
    {
    case FONT_LEGACY:
        return Text::FONT_LEGACY;
    case FONT_ARIAL:
        return Text::FONT_ARIAL;
    case FONT_ARIALBOLD:
        return Text::FONT_ARIALBOLD;
    case FONT_SOURCESANS:
        return Text::FONT_SOURCESANS;
    case FONT_SOURCESANSBOLD:
        return Text::FONT_SOURCESANSBOLD;
    case FONT_SOURCESANSLIGHT:
        return Text::FONT_SOURCESANSLIGHT;
    case FONT_SOURCESANSITALIC:
        return Text::FONT_SOURCESANSITALIC;
    case FONT_BODONI:
        return Text::FONT_BODONI;
    case FONT_GARAMOND:
        return Text::FONT_GARAMOND;
    case FONT_CARTOON:
        return Text::FONT_CARTOON;
    case FONT_CODE:
        return Text::FONT_CODE;
    case FONT_HIGHWAY:
        return Text::FONT_HIGHWAY;
    case FONT_SCIFI:
        return Text::FONT_SCIFI;
    case FONT_ARCADE:
        return Text::FONT_ARCADE;
    case FONT_FANTASY:
        return Text::FONT_FANTASY;
    case FONT_ANTIQUE:
        return Text::FONT_ANTIQUE;
    case FONT_SOURCESANSSEMIBOLD:
        return Text::FONT_SOURCESANSSEMIBOLD;
    case FONT_GOTHAM:
        return Text::FONT_GOTHAM;
    case FONT_GOTHAMSEMIBOLD:
        return Text::FONT_GOTHAMSEMIBOLD;
    case FONT_GOTHAMBOLD:
        return Text::FONT_GOTHAMBOLD;
    case FONT_GOTHAMBLACK:
        return Text::FONT_GOTHAMBLACK;
    case FONT_AMATICSC:
        return Text::FONT_AMATICSC;
    case FONT_BANGERS:
        return Text::FONT_BANGERS;
    case FONT_CREEPSTER:
        return Text::FONT_CREEPSTER;
    case FONT_DENKONE:
        return Text::FONT_DENKONE;
    case FONT_FONDAMENTO:
        return Text::FONT_FONDAMENTO;
    case FONT_FREDOKAONE:
        return Text::FONT_FREDOKAONE;
    case FONT_GRENZEGOTISCH:
        return Text::FONT_GRENZEGOTISCH;
    case FONT_INDIEFLOWER:
        return Text::FONT_INDIEFLOWER;
    case FONT_JOSEFINSANS:
        return Text::FONT_JOSEFINSANS;
    case FONT_JURA:
        return Text::FONT_JURA;
    case FONT_KALAM:
        return Text::FONT_KALAM;
    case FONT_LUCKIESTGUY:
        return Text::FONT_LUCKIESTGUY;
    case FONT_MERRIWEATHER:
        return Text::FONT_MERRIWEATHER;
    case FONT_MICHROMA:
        return Text::FONT_MICHROMA;
    case FONT_NUNITO:
        return Text::FONT_NUNITO;
    case FONT_OSWALD:
        return Text::FONT_OSWALD;
    case FONT_PATRICKHAND:
        return Text::FONT_PATRICKHAND;
    case FONT_PERMANENTMARKER:
        return Text::FONT_PERMANENTMARKER;
    case FONT_ROBOTO:
        return Text::FONT_ROBOTO;
    case FONT_ROBOTOCONDENSED:
        return Text::FONT_ROBOTOCONDENSED;
    case FONT_ROBOTOMONO:
        return Text::FONT_ROBOTOMONO;
    case FONT_SARPANCH:
        return Text::FONT_SARPANCH;
    case FONT_SPECIALELITE:
        return Text::FONT_SPECIALELITE;
    case FONT_TITILLIUMWEB:
        return Text::FONT_TITILLIUMWEB;
    case FONT_UBUNTU:
        return Text::FONT_UBUNTU;
    case FONT_NOTOSANS:
        return Text::FONT_NOTOSANS;
    case FONT_NOTOSANSMEDIUM:
        return Text::FONT_NOTOSANSMEDIUM;
    case FONT_NOTOSANSSEMIBOLD:
        return Text::FONT_NOTOSANSSEMIBOLD;
    case FONT_NOTOSANSBOLD:
        return Text::FONT_NOTOSANSBOLD;
    case FONT_NOTOSANSEXTRABOLD:
        return Text::FONT_NOTOSANSEXTRABOLD;
    case FONT_NOTOSANSBLACK:
        return Text::FONT_NOTOSANSBLACK;
    case FONT_NOTOSANSITALIC:
        return Text::FONT_NOTOSANSITALIC;
    case FONT_NOTOSANSLIGHT:
        return Text::FONT_NOTOSANSLIGHT;
    case FONT_NOTOSANSEXTRALIGHT:
        return Text::FONT_NOTOSANSEXTRALIGHT;
    case FONT_NOTOSANSTHIN:
        return Text::FONT_NOTOSANSTHIN;
    case FONT_CONSOLAS:
        return Text::FONT_CONSOLAS;
    case FONT_COMICSANS:
        return Text::FONT_COMICSANS;
    default:
        AYAASSERT(0);
        return Text::FONT_LEGACY;
    }
}

Text::XAlign TextService::ToTextXAlign(XAlignment xalign)
{

    switch (xalign)
    {
    case TextService::XALIGNMENT_LEFT:
        return Text::XALIGN_LEFT;
    case TextService::XALIGNMENT_RIGHT:
        return Text::XALIGN_RIGHT;
    case TextService::XALIGNMENT_CENTER:
        return Text::XALIGN_CENTER;
    default:
        AYAASSERT(0);
        return Text::XALIGN_LEFT;
    }
}
Text::YAlign TextService::ToTextYAlign(YAlignment yalign)
{
    switch (yalign)
    {
    case TextService::YALIGNMENT_TOP:
        return Text::YALIGN_TOP;
    case TextService::YALIGNMENT_CENTER:
        return Text::YALIGN_CENTER;
    case TextService::YALIGNMENT_BOTTOM:
        return Text::YALIGN_BOTTOM;
    default:
        AYAASSERT(0);
        return Text::YALIGN_TOP;
    }
}

TextService::TextService()
    : Super()
{
    this->setName(sTextService);

    clearTypesetters();
}

void TextService::clearTypesetters()
{
    if (FFlag::TypesettersReleaseResources)
    {
        if (m_typesetters.get())
        {
            for (size_t i = 0; i < FONT_LAST; ++i)
            {
                m_typesetters[i].get()->releaseResources();
            }
        }
        else
        {
            m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
        }
    }
    else
    {
        m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
    }
}
void TextService::registerTypesetter(Font font, shared_ptr<Aya::Typesetter> typesetter)
{
    AYAASSERT(font < FONT_LAST);
    m_typesetters[font] = typesetter;
}

Typesetter* TextService::getTypesetter(Font font)
{
    AYAASSERT(font < FONT_LAST);
    return m_typesetters[font].get();
}

Vector2 TextService::getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize)
{
    if (font >= FONT_LAST || font < FONT_LEGACY)
    {
        return Vector2::zero();
    }

    if (Typesetter* typesetter = getTypesetter(font))
    {
        return typesetter->measure(text, (float)fontSize, frameSize);
    }

    return Vector2::zero();
}

} // namespace Aya
