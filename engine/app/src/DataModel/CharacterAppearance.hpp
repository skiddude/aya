#pragma once

#include "DataModel/GlobalSettings.hpp"
#include "Utility/BrickColor.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/IModelModifier.hpp"
#include "Utility/TextureId.hpp"

namespace Aya
{

class Humanoid;

extern const char* const sCharacterAppearance;
class AyaBaseClass CharacterAppearance
    : public DescribedNonCreatable<CharacterAppearance, Instance, sCharacterAppearance>
    , public IModelModifier
{
private:
    typedef Instance Super;

public:
    virtual void apply();

protected:
    virtual void onAncestorChanged(const AncestorChanged& event);
    virtual bool askSetParent(const Instance* instance) const;

private:
    virtual void applyByMyself(Humanoid* humanoid) = 0;
};

class LegacyCharacterAppearance : public CharacterAppearance
{
private:
    typedef CharacterAppearance Super;

public:
    // Hack: This apply() function is ugly, but necessary because
    //       this class changes properties of other objects (like
    //       Part colors and Decal images).  However, to avoid
    //       crosstalk the apply function should only do something
    //       in the backend case.
    /*override*/ void apply();
};

// Old-style T-shirts
extern const char* const sShirtGraphic;
class ShirtGraphic : public DescribedCreatable<ShirtGraphic, LegacyCharacterAppearance, sShirtGraphic>
{
public:
    TextureId graphic;
    static Reflection::BoundProp<TextureId> prop_Graphic;
    ShirtGraphic();

protected:
    /*override*/ void applyByMyself(Humanoid* humanoid);

private:
    void dataChanged(const Reflection::PropertyDescriptor&)
    {
        CharacterAppearance::apply();
    }
};

extern const char* const sClothing;
class Clothing : public DescribedNonCreatable<Clothing, CharacterAppearance, sClothing>
{
    friend class CharacterAppearance;

public:
    TextureId outfit1;
    TextureId outfit2;

    static Reflection::BoundProp<TextureId> prop_outfit1;
    static Reflection::BoundProp<TextureId> prop_outfit2;

    Clothing();

    virtual TextureId getTemplate() const
    {
        AYAASSERT(false);
        return NULL;
    }

protected:
    /*override*/ void applyByMyself(Humanoid* humanoid);
    void dataChanged(const Reflection::PropertyDescriptor&)
    {
        CharacterAppearance::apply();
    }
};

extern const char* const sPants;
class Pants : public DescribedCreatable<Pants, Clothing, sPants>
{
public:
    Pants();
    static Reflection::PropDescriptor<Pants, TextureId> prop_PantsTemplate;
    TextureId getTemplate() const
    {
        return outfit1;
    }
    void setTemplate(TextureId value);
};

extern const char* const sShirt;
class Shirt : public DescribedCreatable<Shirt, Clothing, sShirt>
{
public:
    Shirt();
    static Reflection::PropDescriptor<Shirt, TextureId> prop_ShirtTemplate;
    TextureId getTemplate() const
    {
        return outfit2;
    }
    void setTemplate(TextureId value);
};

extern const char* const sBodyColors;
class BodyColors : public DescribedCreatable<BodyColors, LegacyCharacterAppearance, sBodyColors>
{
    BrickColor headColor;
    Color3uint8 headColor3;
    BrickColor leftArmColor;
    Color3uint8 leftArmColor3;
    BrickColor rightArmColor;
    Color3uint8 rightArmColor3;
    BrickColor torsoColor;
    Color3uint8 torsoColor3;
    BrickColor leftLegColor;
    Color3uint8 leftLegColor3;
    BrickColor rightLegColor;
    Color3uint8 rightLegColor3;

    BrickColor getHeadColor() const
    {
        return headColor;
    }
    BrickColor getLeftArmColor() const
    {
        return leftArmColor;
    }
    BrickColor getRightArmColor() const
    {
        return rightArmColor;
    }
    BrickColor getTorsoColor() const
    {
        return torsoColor;
    }
    BrickColor getLeftLegColor() const
    {
        return leftLegColor;
    }
    BrickColor getRightLegColor() const
    {
        return rightLegColor;
    }

    Color3 getHeadColor3() const
    {
        return headColor3;
    }
    Color3 getLeftArmColor3() const
    {
        return leftArmColor3;
    }
    Color3 getRightArmColor3() const
    {
        return rightArmColor3;
    }
    Color3 getTorsoColor3() const
    {
        return torsoColor3;
    }
    Color3 getLeftLegColor3() const
    {
        return leftLegColor3;
    }
    Color3 getRightLegColor3() const
    {
        return rightLegColor3;
    }

    void setHeadColor(BrickColor value);
    void setLeftArmColor(BrickColor value);
    void setRightArmColor(BrickColor value);
    void setTorsoColor(BrickColor value);
    void setLeftLegColor(BrickColor value);
    void setRightLegColor(BrickColor value);

    void setHeadColor3(Color3 value);
    void setLeftArmColor3(Color3 value);
    void setRightArmColor3(Color3 value);
    void setTorsoColor3(Color3 value);
    void setLeftLegColor3(Color3 value);
    void setRightLegColor3(Color3 value);

public:
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_HeadColor;
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_LeftArmColor;
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_RightArmColor;
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_TorsoColor;
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_LeftLegColor;
    static Reflection::PropDescriptor<BodyColors, BrickColor> prop_RightLegColor;

    static Reflection::PropDescriptor<BodyColors, Color3> prop_HeadColor3;
    static Reflection::PropDescriptor<BodyColors, Color3> prop_LeftArmColor3;
    static Reflection::PropDescriptor<BodyColors, Color3> prop_RightArmColor3;
    static Reflection::PropDescriptor<BodyColors, Color3> prop_TorsoColor3;
    static Reflection::PropDescriptor<BodyColors, Color3> prop_LeftLegColor3;
    static Reflection::PropDescriptor<BodyColors, Color3> prop_RightLegColor3;

    BodyColors();

private:
    virtual void applyByMyself(Humanoid* humanoid);
    void dataChanged(const Reflection::PropertyDescriptor&)
    {
        CharacterAppearance::apply();
    }
};

extern const char* const sSkin;
class Skin : public DescribedCreatable<Skin, LegacyCharacterAppearance, sSkin>
{
    BrickColor skinColor;

public:
    static Reflection::BoundProp<BrickColor> prop_skinColor;
    Skin();

private:
    virtual void applyByMyself(Humanoid* humanoid);
    void dataChanged(const Reflection::PropertyDescriptor&)
    {
        CharacterAppearance::apply();
    }
};

} // namespace Aya
