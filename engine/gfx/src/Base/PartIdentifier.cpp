#include "Base/PartIdentifier.hpp"

#include "Humanoid/Humanoid.hpp"

#include "DataModel/DataModelMesh.hpp"
#include "DataModel/FileMesh.hpp"
#include "DataModel/Decal.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/CharacterAppearance.hpp"
#include "DataModel/CharacterMesh.hpp"
#include "DataModel/Accoutrement.hpp"
#include "DataModel/MeshPartInstance.hpp"

#include "DataModel/PartCookie.hpp"

namespace Aya
{

static const Vector3 humanoidPartScalesR15[HumanoidIdentifier::PartType_Count] = {
    Vector3(2, 1.6, 1), // PartType_UpperTorso
    Vector3(2, 0.4, 1), // PartType_LowerTorso
    Vector3(1, 1.2, 1), // PartType_LowerArm
    Vector3(1, 1.4, 1), // PartType_UpperArm
    Vector3(1, 0.3, 1), // PartType_Hand
    Vector3(1, 1.5, 1), // PartType_UpperLeg
    Vector3(1, 1.5, 1), // PartType_LowerLeg
    Vector3(1, 0.3, 1)  // PartType_Foot
};

static const Vector3 humanoidPartScales[HumanoidIdentifier::PartType_Count] = {
    Vector3(1, 1, 1), // PartType_Head
    Vector3(2, 2, 1), // PartType_Torso
    Vector3(1, 2, 1), // PartType_Arm
    Vector3(1, 2, 1), // PartType_Leg
    Vector3(1, 1, 1)  // PartType_Unknown
};

// if the part is a humanoid, get further details with this.
HumanoidIdentifier::HumanoidIdentifier(Aya::Humanoid* humanoid)
    : humanoid(humanoid)
    , head(0)
    , leftLeg(0)
    , rightLeg(0)
    , leftArm(0)
    , rightArm(0)
    , torso(0)
    , upperTorso(0)
    , lowerTorso(0)
    , rightUpperArm(0)
    , rightLowerArm(0)
    , rightHand(0)
    , leftUpperArm(0)
    , leftLowerArm(0)
    , leftHand(0)
    , rightUpperLeg(0)
    , rightLowerLeg(0)
    , rightFoot(0)
    , leftUpperLeg(0)
    , leftLowerLeg(0)
    , leftFoot(0)
    , leftLegMesh(0)
    , rightLegMesh(0)
    , leftArmMesh(0)
    , rightArmMesh(0)
    , torsoMesh(0)
{
    if (!humanoid)
        return;

    Instance* parent = humanoid->getParent();
    if (!parent)
        return;

    const Instances& children = *parent->getChildren();

    for (size_t i = 0; i < children.size(); ++i)
    {
        Instance* inst = children[i].get();

        if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(inst))
        {
            const std::string& name = part->getName();

            if (name == "Head")
                head = part;
            else if (name == "Left Leg")
                leftLeg = part;
            else if (name == "Right Leg")
                rightLeg = part;
            else if (name == "Left Arm")
                leftArm = part;
            else if (name == "Right Arm")
                rightArm = part;
            else if (name == "Torso")
                torso = part;
            if (name == "UpperTorso")
                upperTorso = part;
            else if (name == "LowerTorso")
                lowerTorso = part;
            else if (name == "LeftUpperArm")
                leftUpperArm = part;
            else if (name == "LeftLowerArm")
                leftLowerArm = part;
            else if (name == "LeftHand")
                leftHand = part;
            else if (name == "RightUpperArm")
                rightUpperArm = part;
            else if (name == "RightLowerArm")
                rightLowerArm = part;
            else if (name == "RightHand")
                rightHand = part;
            else if (name == "LeftUpperLeg")
                leftUpperLeg = part;
            else if (name == "LeftLowerLeg")
                leftLowerLeg = part;
            else if (name == "LeftFoot")
                leftFoot = part;
            else if (name == "RightUpperLeg")
                rightUpperLeg = part;
            else if (name == "RightLowerLeg")
                rightLowerLeg = part;
            else if (name == "RightFoot")
                rightFoot = part;
        }
        else if (Clothing* c = Instance::fastDynamicCast<Clothing>(inst))
        {
            if (pants.isNull() && !c->outfit1.isNull())
                pants = c->outfit1;
            if (shirt.isNull() && !c->outfit2.isNull())
                shirt = c->outfit2;
        }
        else if (ShirtGraphic* s = Instance::fastDynamicCast<ShirtGraphic>(inst))
        {
            if (shirtGraphic.isNull() && !s->graphic.isNull())
                shirtGraphic = s->graphic;
        }
        else if (CharacterMesh* m = Instance::fastDynamicCast<CharacterMesh>(inst))
        {
            switch (m->getBodyPart())
            {
            case CharacterMesh::LEFTARM:
                leftArmMesh = m;
                break;
            case CharacterMesh::RIGHTARM:
                rightArmMesh = m;
                break;
            case CharacterMesh::LEFTLEG:
                leftLegMesh = m;
                break;
            case CharacterMesh::RIGHTLEG:
                rightLegMesh = m;
                break;
            case CharacterMesh::TORSO:
                torsoMesh = m;
                break;
            default:
                AYAASSERT(!"Unsupported body part type");
                break;
            }
        }
        else if (Accoutrement* a = Instance::fastDynamicCast<Accoutrement>(inst))
        {
            accoutrements.push_back(a);
        }
    }
}

CharacterMesh* HumanoidIdentifier::getRelevantMesh(Aya::PartInstance* bodyPart) const
{
    if (bodyPart == leftLeg || bodyPart == leftUpperLeg || bodyPart == leftLowerLeg || bodyPart == leftFoot)
        return leftLegMesh;
    if (bodyPart == rightLeg || bodyPart == rightUpperLeg || bodyPart == rightLowerLeg || bodyPart == rightFoot)
        return rightLegMesh;
    if (bodyPart == leftArm || bodyPart == leftUpperArm || bodyPart == leftLowerArm || bodyPart == leftHand)
        return leftArmMesh;
    if (bodyPart == rightArm || bodyPart == rightUpperArm || bodyPart == rightLowerArm || bodyPart == rightHand)
        return rightArmMesh;
    if (bodyPart == torso || bodyPart == upperTorso || bodyPart == lowerTorso)
        return torsoMesh;
    return NULL;
}

HumanoidIdentifier::BodyPartType HumanoidIdentifier::getBodyPartType(Aya::PartInstance* bodyPart) const
{
    if (humanoid->getUseR15())
    {
        if (bodyPart == leftUpperLeg || bodyPart == rightUpperLeg)
            return PartType_UpperLeg;
        if (bodyPart == leftUpperArm || bodyPart == rightUpperArm)
            return PartType_UpperArm;
        if (bodyPart == leftLowerLeg || bodyPart == rightLowerLeg)
            return PartType_LowerLeg;
        if (bodyPart == leftLowerArm || bodyPart == rightLowerArm)
            return PartType_LowerArm;
        if (bodyPart == leftHand || bodyPart == rightHand)
            return PartType_Hand;
        if (bodyPart == leftFoot || bodyPart == rightFoot)
            return PartType_Foot;
        if (bodyPart == upperTorso)
            return PartType_UpperTorso;
        if (bodyPart == lowerTorso)
            return PartType_LowerTorso;
    }
    else
    {
        if (bodyPart == leftLeg || bodyPart == rightLeg)
            return PartType_Leg;
        if (bodyPart == leftArm || bodyPart == rightArm)
            return PartType_Arm;
        if (bodyPart == torso)
            return PartType_Torso;
        if (bodyPart == head)
            return PartType_Head;
    }

    return PartType_Unknown;
}

Vector3 HumanoidIdentifier::getBodyPartScale(Aya::PartInstance* bodyPart) const
{
    return humanoidPartScales[getBodyPartType(bodyPart)];
}

bool HumanoidIdentifier::isPartHead(Aya::PartInstance* part) const
{
    if (part != head)
        return false;

    if (Aya::DataModelMesh* specialShape = Aya::getSpecialShape(part))
    {
        bool hasFace = (part->getCookie() & PartCookie::HAS_DECALS) != 0;

        if (Aya::SpecialShape* shape = specialShape->fastDynamicCast<Aya::SpecialShape>())
        {
            // A real head or a file mesh - treat it as a head even if there is no face (might use a mesh texture)
            if (shape->getMeshType() == Aya::SpecialShape::HEAD_MESH || shape->getMeshType() == Aya::SpecialShape::FILE_MESH)
                return true;

            // Probably one of the heads from the store - all character heads have faces
            if (shape->getMeshType() == Aya::SpecialShape::SPHERE_MESH)
                return hasFace;

            // Unrecognized shape type - this is not a head from the store, so don't treat it as a head.
            return false;
        }
        else if (specialShape->fastDynamicCast<Aya::FileMesh>())
        {
            // A file mesh - treat it as a head even if there is no face (might use a mesh texture)
            return true;
        }
        else
        {
            // Probably one of the heads from the store - all character heads have faces
            return hasFace;
        }
    }

    // No special shape - should be a simple part
    return false;
}

bool HumanoidIdentifier::isBodyPart(Aya::PartInstance* part) const
{
    if (humanoid->getUseR15())
        return (leftUpperArm == part || leftUpperLeg == part || rightUpperArm == part || rightUpperLeg == part || upperTorso == part ||
                lowerTorso == part || rightLowerArm == part || leftLowerArm == part || leftHand == part || rightHand == part || leftFoot == part ||
                rightFoot == part || leftLowerLeg == part || rightLowerLeg == part || head == part);
    else if (part->getCookie() & PartCookie::IS_HUMANOID_PART)
        return (leftArm == part || leftLeg == part || rightArm == part || rightLeg == part || torso == part || head == part);
    else
        return false;
}

bool HumanoidIdentifier::isBodyPartComposited(Aya::PartInstance* part) const
{
    bool noReflectance = part->getReflectance() <= 0.015f;

    if (MeshPartInstance* meshPart = part->fastDynamicCast<MeshPartInstance>())
        return humanoid->getUseR15();

    // Heads are always composited as long as they are not transparent
    if (part == head)
        return humanoid->getUseR15() ? false : (part->getTransparencyUi() <= 0 && isPartHead(part) && noReflectance);

    // Body parts with special shapes are never composited to match the old behavior
    if (part->getCookie() & PartCookie::HAS_SPECIALSHAPE)
        return false;

    // Non-block parts are never composited to match the old behavior
    if (part->getPartType() != BLOCK_PART)
        return false;

    // Parts with meshes are always composited
    if (getRelevantMesh(part))
        return true;

    // Arms are always composited if there is a shirt
    if ((part == leftArm || part == rightArm) && !shirt.isNull())
        return true;

    // Legs are always composited if there are pants
    if ((part == leftLeg || part == rightLeg) && !pants.isNull())
        return true;

    // Torso is always composited if there is a shirt, pants or a t-shirt
    if (part == torso && (!pants.isNull() || !shirt.isNull() || !shirtGraphic.isNull()))
        return true;

    // Now we have a body part and we have a choice - we can composit it or skip compositing.
    // Compositing means that we lose materials; we also replace the body part with a prebaked mesh, so we lose the size information.
    // We also lose studs, but we care way less about those - so to improve batching, we'll composit plastic parts with expected size.
    return (part->getRenderMaterial() == PLASTIC_MATERIAL || part->getRenderMaterial() == SMOOTH_PLASTIC_MATERIAL) && noReflectance;
}

bool HumanoidIdentifier::isPartComposited(Aya::PartInstance* part) const
{
    if (isBodyPart(part))
        return isBodyPartComposited(part);

    // We don't composit Accoutrements for R15
    // @mdolli
    if (Instance::isA<Accoutrement>(part->getParent()) && !this->humanoid->getUseR15())
        return true;

    return false;
}
} // namespace Aya
