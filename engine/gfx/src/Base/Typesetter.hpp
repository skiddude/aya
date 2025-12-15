#pragma once

#include "Utility/G3DCore.hpp"
#include "Utility/Rotation2d.hpp"
#include "Base/Type.hpp"
#include <boost/shared_ptr.hpp>

namespace Aya
{
class Adorn;

namespace Graphics
{
class Texture;
class TextureManager;
class TextureAtlas;
}; // namespace Graphics

// abstract base class
class Typesetter
{
public:
    virtual ~Typesetter() {}

    virtual Vector2 draw(Adorn* adorn, const std::string& s, const Vector2& position, float size, bool autoScale, const Color4& color,
        const Color4& outline, Aya::Text::XAlign xalign = Aya::Text::XALIGN_LEFT, Aya::Text::YAlign yalign = Aya::Text::YALIGN_TOP,
        const Vector2& availableSpace = Vector2::zero(), const Rect2D& clippingRect = Rect2D::xyxy(-1, -1, -1, -1),
        const Rotation2D& rotation = Rotation2D()) const = 0;


    virtual int getCursorPositionInText(const std::string& s, const Aya::Vector2& pos2D, float size, Aya::Text::XAlign xalign,
        Aya::Text::YAlign yalign, const Aya::Vector2& availableSpace, const Rotation2D& rotation, Aya::Vector2 cursorPos) const = 0;


    /**
     Useful for drawing centered text and boxes around text.
     */
    virtual Vector2 measure(const std::string& s, float size, const Vector2& availableSpace = Vector2::zero(), bool* textFits = NULL) const = 0;

    virtual void loadResources(Aya::Graphics::TextureManager* textureManager, Aya::Graphics::TextureAtlas* glyphAtlas) = 0;
    virtual void releaseResources() = 0;
    virtual const boost::shared_ptr<Graphics::Texture>& getTexture() const = 0;

    static bool isCharNonWhitespace(char c)
    {
        return (c >= '!' && c <= '~');
    }
    static bool isCharWhitespace(char c)
    {
        return (c == ' ' || c == '\t' || c == '\n');
    }
    static bool isCharSupported(char c)
    {
        return isCharNonWhitespace(c) || isCharWhitespace(c);
    }
    static bool isStringSupported(std::string& stringToCheck)
    {
        for (std::string::iterator iter = stringToCheck.begin(); iter != stringToCheck.end(); ++iter)
        {
            if (!isCharSupported(*iter))
            {
                return false;
            }
        }
        return true;
    }
};
} // namespace Aya
