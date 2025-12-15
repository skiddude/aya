


#include "DataModel/CharacterAppearance.hpp"
#include "DataModel/Decal.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/ModelInstance.hpp"
#include "Humanoid/Humanoid.hpp"
#include "Players.hpp"

DYNAMIC_FASTFLAG(UseR15Character)

namespace Aya
{

const char* const sCharacterAppearance = "CharacterAppearance";

const char* const sShirt = "Shirt";
const char* const sPants = "Pants";
const char* const sShirtGraphic = "ShirtGraphic";
const char* const sClothing = "Clothing";


Reflection::BoundProp<TextureId> ShirtGraphic::prop_Graphic("Graphic", category_Appearance, &ShirtGraphic::graphic, &ShirtGraphic::dataChanged);

Reflection::BoundProp<TextureId> Clothing::prop_outfit1(
    "Outfit1", category_Appearance, &Clothing::outfit1, &Clothing::dataChanged, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<TextureId> Clothing::prop_outfit2(
    "Outfit2", category_Appearance, &Clothing::outfit2, &Clothing::dataChanged, Reflection::PropertyDescriptor::LEGACY);

Reflection::PropDescriptor<Shirt, TextureId> Shirt::prop_ShirtTemplate(
    "ShirtTemplate", category_Appearance, &Shirt::getTemplate, &Shirt::setTemplate);
Reflection::PropDescriptor<Pants, TextureId> Pants::prop_PantsTemplate(
    "PantsTemplate", category_Appearance, &Pants::getTemplate, &Pants::setTemplate);

ShirtGraphic::ShirtGraphic()
{
    setName("Shirt Graphic");
}

Clothing::Clothing()
{
    setName("Clothing");
}

Shirt::Shirt()
{
    // Don't remove this! Gcc needs it so that Creatable works
}
Pants::Pants()
{
    // Don't remove this! Gcc needs it so that Creatable works
}

void ShirtGraphic::applyByMyself(Humanoid* humanoid)
{
    if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
        if (Decal* decal = torso->findFirstChildOfType<Decal>())
            decal->setTexture(graphic);
}

void Clothing::applyByMyself(Humanoid* humanoid)
{
    if (humanoid->getUseR15())
    {
        Instance* pParent = getParent();
        if (pParent)
        {
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LowerTorso")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("UpperTorso")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("Head")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperArm")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerArm")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightHand")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperArm")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerArm")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftHand")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperLeg")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerLeg")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightFoot")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperLeg")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerLeg")))
                answer->fireOutfitChanged();
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftFoot")))
                answer->fireOutfitChanged();
        }
    }
    else
    {
        if (PartInstance* leftLeg = humanoid->getLeftLegSlow())
        {
            leftLeg->fireOutfitChanged();
        }
        if (PartInstance* rightLeg = humanoid->getRightLegSlow())
        {
            rightLeg->fireOutfitChanged();
        }
        if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
        {
            torso->fireOutfitChanged();
        }
        if (PartInstance* leftSleeve = humanoid->getLeftArmSlow())
        {
            leftSleeve->fireOutfitChanged();
        }
        if (PartInstance* rightSleeve = humanoid->getRightArmSlow())
        {
            rightSleeve->fireOutfitChanged();
        }
    }
}

void Shirt::setTemplate(TextureId value)
{
    if (value != Clothing::outfit2)
    {
        prop_outfit2.setValue(this, value);
        raisePropertyChanged(prop_ShirtTemplate);
    }
}

void Pants::setTemplate(TextureId value)
{
    if (value != Clothing::outfit1)
    {
        prop_outfit1.setValue(this, value);
        raisePropertyChanged(prop_PantsTemplate);
    }
}

const char* const sSkin = "Skin";

Reflection::BoundProp<BrickColor> Skin::prop_skinColor("SkinColor", category_Appearance, &Skin::skinColor, &Skin::dataChanged);

Skin::Skin()
    : skinColor(BrickColor::brick_226)
{
    setName("Skin");
}

void Skin::applyByMyself(Humanoid* humanoid)
{
    if (PartInstance* head = humanoid->getHeadSlow())
        head->setColor(skinColor);
    if (PartInstance* leftLeg = humanoid->getLeftLegSlow())
        leftLeg->setColor(skinColor);
    if (PartInstance* rightLeg = humanoid->getRightLegSlow())
        rightLeg->setColor(skinColor);
    if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
        torso->setColor(skinColor);
    if (PartInstance* leftSleeve = humanoid->getLeftArmSlow())
        leftSleeve->setColor(skinColor);
    if (PartInstance* rightSleeve = humanoid->getRightArmSlow())
        rightSleeve->setColor(skinColor);
}

const char* const sBodyColors = "BodyColors";

Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_HeadColor(
    "HeadColor", category_Appearance, &BodyColors::getHeadColor, &BodyColors::setHeadColor);
Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_LeftArmColor(
    "LeftArmColor", category_Appearance, &BodyColors::getLeftArmColor, &BodyColors::setLeftArmColor);
Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_RightArmColor(
    "RightArmColor", category_Appearance, &BodyColors::getRightArmColor, &BodyColors::setRightArmColor);
Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_TorsoColor(
    "TorsoColor", category_Appearance, &BodyColors::getTorsoColor, &BodyColors::setTorsoColor);
Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_LeftLegColor(
    "LeftLegColor", category_Appearance, &BodyColors::getLeftLegColor, &BodyColors::setLeftLegColor);
Reflection::PropDescriptor<BodyColors, BrickColor> BodyColors::prop_RightLegColor(
    "RightLegColor", category_Appearance, &BodyColors::getRightLegColor, &BodyColors::setRightLegColor);

Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_HeadColor3(
    "HeadColor3", category_Appearance, &BodyColors::getHeadColor3, &BodyColors::setHeadColor3);
Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_LeftArmColor3(
    "LeftArmColor3", category_Appearance, &BodyColors::getLeftArmColor3, &BodyColors::setLeftArmColor3);
Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_RightArmColor3(
    "RightArmColor3", category_Appearance, &BodyColors::getRightArmColor3, &BodyColors::setRightArmColor3);
Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_TorsoColor3(
    "TorsoColor3", category_Appearance, &BodyColors::getTorsoColor3, &BodyColors::setTorsoColor3);
Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_LeftLegColor3(
    "LeftLegColor3", category_Appearance, &BodyColors::getLeftLegColor3, &BodyColors::setLeftLegColor3);
Reflection::PropDescriptor<BodyColors, Color3> BodyColors::prop_RightLegColor3(
    "RightLegColor3", category_Appearance, &BodyColors::getRightLegColor3, &BodyColors::setRightLegColor3);

BodyColors::BodyColors()
    : headColor(BrickColor::brick_226)
    , leftArmColor(BrickColor::brick_226)
    , rightArmColor(BrickColor::brick_226)
    , torsoColor(BrickColor::brick_28)
    , leftLegColor(BrickColor::brick_23)
    , rightLegColor(BrickColor::brick_23)
    , headColor3(BrickColor(BrickColor::brick_226).color3uint8())
    , leftArmColor3(BrickColor(BrickColor::brick_226).color3uint8())
    , rightArmColor3(BrickColor(BrickColor::brick_226).color3uint8())
    , torsoColor3(BrickColor(BrickColor::brick_28).color3uint8())
    , leftLegColor3(BrickColor(BrickColor::brick_23).color3uint8())
    , rightLegColor3(BrickColor(BrickColor::brick_23).color3uint8())
{
    setName("Body Colors");
}

void BodyColors::setHeadColor(BrickColor value)
{
    if (value != headColor)
    {
        headColor = value;
        headColor3 = value.color3uint8();
        raisePropertyChanged(prop_HeadColor);
        raisePropertyChanged(prop_HeadColor3);
    }
}

void BodyColors::setLeftArmColor(BrickColor value)
{
    if (value != leftArmColor)
    {
        leftArmColor = value;
        leftArmColor3 = value.color3uint8();
        raisePropertyChanged(prop_LeftArmColor);
        raisePropertyChanged(prop_LeftArmColor3);
    }
}

void BodyColors::setRightArmColor(BrickColor value)
{
    if (value != rightArmColor)
    {
        rightArmColor = value;
        rightArmColor3 = value.color3uint8();
        raisePropertyChanged(prop_RightArmColor);
        raisePropertyChanged(prop_RightArmColor3);
    }
}

void BodyColors::setTorsoColor(BrickColor value)
{
    if (value != torsoColor)
    {
        torsoColor = value;
        torsoColor3 = value.color3uint8();
        raisePropertyChanged(prop_TorsoColor);
        raisePropertyChanged(prop_TorsoColor3);
    }
}

void BodyColors::setLeftLegColor(BrickColor value)
{
    if (value != leftLegColor)
    {
        leftLegColor = value;
        leftLegColor3 = value.color3uint8();
        raisePropertyChanged(prop_LeftLegColor);
        raisePropertyChanged(prop_LeftLegColor3);
    }
}

void BodyColors::setRightLegColor(BrickColor value)
{
    if (value != rightLegColor)
    {
        rightLegColor = value;
        rightLegColor3 = value.color3uint8();
        raisePropertyChanged(prop_RightLegColor);
        raisePropertyChanged(prop_RightLegColor3);
    }
}

void BodyColors::setHeadColor3(Color3 value)
{
    if (value != Color3(headColor3))
    {
        headColor3 = value;
        headColor = BrickColor::closest(value);
        raisePropertyChanged(prop_HeadColor3);
        raisePropertyChanged(prop_HeadColor);
    }
}

void BodyColors::setLeftArmColor3(Color3 value)
{
    if (value != Color3(leftArmColor3))
    {
        leftArmColor3 = value;
        leftArmColor = BrickColor::closest(value);
        raisePropertyChanged(prop_LeftArmColor3);
        raisePropertyChanged(prop_LeftArmColor);
    }
}

void BodyColors::setRightArmColor3(Color3 value)
{
    if (value != Color3(rightArmColor3))
    {
        rightArmColor3 = value;
        rightArmColor = BrickColor::closest(value);
        raisePropertyChanged(prop_RightArmColor3);
        raisePropertyChanged(prop_RightArmColor);
    }
}

void BodyColors::setTorsoColor3(Color3 value)
{
    if (value != Color3(torsoColor3))
    {
        torsoColor3 = value;
        torsoColor = BrickColor::closest(value);
        raisePropertyChanged(prop_TorsoColor3);
        raisePropertyChanged(prop_TorsoColor);
    }
}

void BodyColors::setLeftLegColor3(Color3 value)
{
    if (value != Color3(leftLegColor3))
    {
        leftLegColor3 = value;
        leftLegColor = BrickColor::closest(value);
        raisePropertyChanged(prop_LeftLegColor3);
        raisePropertyChanged(prop_LeftLegColor);
    }
}

void BodyColors::setRightLegColor3(Color3 value)
{
    if (value != Color3(rightLegColor3))
    {
        rightLegColor3 = value;
        rightLegColor = BrickColor::closest(value);
        raisePropertyChanged(prop_RightLegColor3);
        raisePropertyChanged(prop_RightLegColor);
    }
}

void BodyColors::applyByMyself(Humanoid* humanoid)
{
    bool hasSkin = ModelInstance::findFirstModifierOfType<Skin>(getParent()) != 0;
    if (hasSkin)
        return;

    if (humanoid->getUseR15())
    {
        Instance* pParent = getParent();
        if (pParent)
        {
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LowerTorso")))
                answer->setColor3(torsoColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("UpperTorso")))
                answer->setColor3(torsoColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("Head")))
                answer->setColor3(headColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperArm")))
                answer->setColor3(rightArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerArm")))
                answer->setColor3(rightArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightHand")))
                answer->setColor3(rightArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperArm")))
                answer->setColor3(leftArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerArm")))
                answer->setColor3(leftArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftHand")))
                answer->setColor3(leftArmColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperLeg")))
                answer->setColor3(rightLegColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerLeg")))
                answer->setColor3(rightLegColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightFoot")))
                answer->setColor3(rightLegColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperLeg")))
                answer->setColor3(leftLegColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerLeg")))
                answer->setColor3(leftLegColor3);
            if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftFoot")))
                answer->setColor3(leftLegColor3);
        }
    }
    else
    {
        if (PartInstance* head = humanoid->getHeadSlow())
            head->setColor3(headColor3);
        if (PartInstance* leftLeg = humanoid->getLeftLegSlow())
            leftLeg->setColor3(leftLegColor3);
        if (PartInstance* rightLeg = humanoid->getRightLegSlow())
            rightLeg->setColor3(rightLegColor3);
        if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
            torso->setColor3(torsoColor3);
        if (PartInstance* leftSleeve = humanoid->getLeftArmSlow())
            leftSleeve->setColor3(leftArmColor3);
        if (PartInstance* rightSleeve = humanoid->getRightArmSlow())
            rightSleeve->setColor3(rightArmColor3);
    }
}

void LegacyCharacterAppearance::apply()
{
    if (Network::Players::backendProcessing(this, false))
        Super::apply();
}
void CharacterAppearance::apply()
{
    if (Humanoid* humanoid = Humanoid::modelIsCharacter(getParent()))
        applyByMyself(humanoid);
}

void CharacterAppearance::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);

    // Search for a Humanoid sibling:
    if (event.child == this)
    {

        if (event.newParent)
        {
            Aya::Humanoid* humanoid = Humanoid::modelIsCharacter(event.newParent);
            if (humanoid)
            {
                applyByMyself(humanoid);
            }
        }
        if (event.oldParent)
        {
            Aya::Humanoid* humanoid = Humanoid::modelIsCharacter(event.oldParent);
            if (humanoid)
            {
                applyByMyself(humanoid);
            }
        }
    }
}

bool CharacterAppearance::askSetParent(const Instance* instance) const
{
    return Instance::fastDynamicCast<ModelInstance>(instance) != NULL;
}

} // namespace Aya
