

#pragma once

#include "DataModel/GuiCore.hpp"
#include "Xml/Reference.hpp"
#include "Utility/TextureId.hpp"
#include "Base/TextureProxyBase.hpp"
#include "Base/Adorn.hpp"
#include "signal.hpp"

namespace Aya
{

class Adorn;

class GuiDrawImage
{
public:
    enum ImageState
    {
        NORMAL = 0x1,
        HOVER = 0x2,
        DOWN = 0x4,
        DISABLE = 0x8,
        SELECTED = 0x10,
        SELECTED_HOVER = 0x20,
        SELECTED_DOWN = 0x40,
        ALL = 0x7F
    };

private:
    TextureId currentTexture;
    TextureId loadingTexture;
    Aya::TextureProxyBaseRef disable;
    Aya::TextureProxyBaseRef normal;
    Aya::TextureProxyBaseRef hover;
    Aya::TextureProxyBaseRef down;
    Aya::TextureProxyBaseRef selected;
    Aya::TextureProxyBaseRef selectedHover;
    Aya::TextureProxyBaseRef selectedDown;
    mutable Vector2 size;
    Aya::signals::scoped_connection unbindResourceSignalHint;

    void OnUnbindResourceSignalHint();

    void draw(Adorn* adorn, const Aya::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr,
        const Color4& color, const Rect& clipRect, const Color4& behind, const Color4& inFront);

    void draw(Adorn* adorn, const Aya::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr,
        const Color4& color, const Rotation2D& rotation, const Color4& behind, const Color4& inFront);

    template<typename Modifier>
    void render2dImpl(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
        const Modifier& modifier, Gui::WidgetState state, bool isSelected);

    void tryCreateTextureProxy(
        Adorn* adorn, const std::string& contentString, const std::string& context, Aya::TextureProxyBaseRef& textureRef, bool& isWaiting);

public:
    GuiDrawImage()
        : size(Vector2(0, 0))
    {
    }
    GuiDrawImage(Adorn* adorn, const std::string& textureName, unsigned imageState)
    {
        setImageFromName(adorn, textureName, imageState);
    }

    void render2d(Adorn* adorn, bool enabled, const Rect& rect, Gui::WidgetState state, bool isSelected);
    void render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
        const Rotation2D& rotation, Gui::WidgetState state, bool isSelected);
    void render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rect& clipRect,
        Gui::WidgetState state, bool isSelected);

    void setImageSize(const Vector2& _size);
    Vector2 getImageSize() const;

    bool setImage(Adorn* adorn, const TextureId& textureId, unsigned imageState, Vector2* outSize = NULL, Instance* contextInstance = NULL,
        const char* context = "");
    bool setImageFromName(
        Adorn* adorn, const std::string& textureName, unsigned imageState, Instance* contextInstance = NULL, const char* context = "");
    void setImageFromRef(Aya::TextureProxyBaseRef ref);

    void computeUV(Vector2& uvtl, Vector2& uvbr, const Vector2& imageRectOffset, const Vector2& imageRectSize, const Vector2& imageSize);
};

} // namespace Aya
