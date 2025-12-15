#pragma once

#include <boost/smart_ptr/scoped_array.hpp>

#include "Tree/Service.hpp"
#include "Tree/Instance.hpp"
#include "Base/Typesetter.hpp"

namespace Aya
{
class PartInstance;

extern const char* const sTextService;

class TextService
    : public DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
private:
    typedef DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
    boost::scoped_array<shared_ptr<Typesetter>> m_typesetters;

public:
    enum XAlignment
    {
        XALIGNMENT_LEFT = 0,
        XALIGNMENT_RIGHT = 1,
        XALIGNMENT_CENTER = 2
    };

    enum YAlignment
    {
        YALIGNMENT_TOP = 0,
        YALIGNMENT_CENTER = 1,
        YALIGNMENT_BOTTOM = 2
    };

    enum FontSize
    {
        SIZE_8 = 0,
        SIZE_9 = 1,
        SIZE_10 = 2,
        SIZE_11 = 3,
        SIZE_12 = 4,
        SIZE_14 = 5,
        SIZE_18 = 6,
        SIZE_24 = 7,
        SIZE_36 = 8,
        SIZE_48 = 9,

        SIZE_28 = 10,
        SIZE_32 = 11,
        SIZE_42 = 12,
        SIZE_60 = 13,
        SIZE_96 = 14,

        // ADD NEW FONT SIZES ABOVE HERE, AND UPDATE BELOW


        SIZE_SMALLEST = SIZE_8,
        SIZE_LARGEST = SIZE_96
    };

    enum Font
    {
        FONT_LEGACY = 0,
        FONT_ARIAL = 1,
        FONT_ARIALBOLD = 2,
        FONT_SOURCESANS = 3,
        FONT_SOURCESANSBOLD = 4,
        FONT_SOURCESANSLIGHT = 5,
        FONT_SOURCESANSITALIC = 6,
        FONT_BODONI = 7,
        FONT_GARAMOND = 8,
        FONT_CARTOON = 9,
        FONT_CODE = 10,
        FONT_HIGHWAY = 11,
        FONT_SCIFI = 12,
        FONT_ARCADE = 13,
        FONT_FANTASY = 14,
        FONT_ANTIQUE = 15,
        FONT_SOURCESANSSEMIBOLD = 16,
        FONT_GOTHAM = 17,
        FONT_GOTHAMSEMIBOLD = 18,
        FONT_GOTHAMBOLD = 19,
        FONT_GOTHAMBLACK = 20,
        FONT_AMATICSC = 21,
        FONT_BANGERS = 22,
        FONT_CREEPSTER = 23,
        FONT_DENKONE = 24,
        FONT_FONDAMENTO = 25,
        FONT_FREDOKAONE = 26,
        FONT_GRENZEGOTISCH = 27,
        FONT_INDIEFLOWER = 28,
        FONT_JOSEFINSANS = 29,
        FONT_JURA = 30,
        FONT_KALAM = 31,
        FONT_LUCKIESTGUY = 32,
        FONT_MERRIWEATHER = 33,
        FONT_MICHROMA = 34,
        FONT_NUNITO = 35,
        FONT_OSWALD = 36,
        FONT_PATRICKHAND = 37,
        FONT_PERMANENTMARKER = 38,
        FONT_ROBOTO = 39,
        FONT_ROBOTOCONDENSED = 40,
        FONT_ROBOTOMONO = 41,
        FONT_SARPANCH = 42,
        FONT_SPECIALELITE = 43,
        FONT_TITILLIUMWEB = 44,
        FONT_UBUNTU = 45,
        FONT_NOTOSANS = 46,
        FONT_NOTOSANSMEDIUM = 47,
        FONT_NOTOSANSSEMIBOLD = 48,
        FONT_NOTOSANSBOLD = 49,
        FONT_NOTOSANSEXTRABOLD = 50,
        FONT_NOTOSANSBLACK = 51,
        FONT_NOTOSANSITALIC = 52,
        FONT_NOTOSANSLIGHT = 53,
        FONT_NOTOSANSEXTRALIGHT = 54,
        FONT_NOTOSANSTHIN = 55,
        FONT_CONSOLAS = 56,
        FONT_COMICSANS = 57,

        FONT_LAST = 58
    };

    static Font FromTextFont(Text::Font font);


    static Text::Font ToTextFont(Font font);
    static Text::XAlign ToTextXAlign(XAlignment xalign);
    static Text::YAlign ToTextYAlign(YAlignment xalign);
    TextService();


    void registerTypesetter(Font font, shared_ptr<Aya::Typesetter> typesetter);
    void clearTypesetters();

    Vector2 getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize);

    Typesetter* getTypesetter(Font font);
};
} // namespace Aya