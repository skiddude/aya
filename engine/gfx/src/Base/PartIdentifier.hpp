#pragma once

#include "Utility/TextureId.hpp"
#include "Utility/G3DCore.hpp"

namespace Aya
{
class PartInstance;
class PartInstance;
class Humanoid;
class CharacterMesh;
class Accoutrement;

// if the part is a humanoid, get further details with this.
class HumanoidIdentifier
{
public:
    explicit HumanoidIdentifier(Aya::Humanoid* humanoid);

    Humanoid* humanoid;

    PartInstance* head;
    PartInstance* leftLeg;
    PartInstance* rightLeg;
    PartInstance* leftArm;
    PartInstance* rightArm;
    PartInstance* torso;

    PartInstance* upperTorso;
    PartInstance* lowerTorso;
    PartInstance* leftUpperArm;
    PartInstance* leftLowerArm;
    PartInstance* leftHand;
    PartInstance* rightUpperArm;
    PartInstance* rightLowerArm;
    PartInstance* rightHand;
    PartInstance* leftUpperLeg;
    PartInstance* leftLowerLeg;
    PartInstance* leftFoot;
    PartInstance* rightUpperLeg;
    PartInstance* rightLowerLeg;
    PartInstance* rightFoot;

    std::vector<Accoutrement*> accoutrements;

    TextureId pants;
    TextureId shirt;
    TextureId shirtGraphic;

    CharacterMesh* leftLegMesh;
    CharacterMesh* rightLegMesh;
    CharacterMesh* leftArmMesh;
    CharacterMesh* rightArmMesh;
    CharacterMesh* torsoMesh;

    CharacterMesh* leftUpperLegMesh;
    CharacterMesh* leftLowerLegMesh;
    CharacterMesh* leftFootMesh;
    CharacterMesh* rightUpperLegMesh;
    CharacterMesh* rightLowerLegMesh;
    CharacterMesh* rightFootMesh;
    CharacterMesh* leftUpperArmMesh;
    CharacterMesh* leftLowerArmMesh;
    CharacterMesh* leftHandMesh;
    CharacterMesh* rightUpperArmMesh;
    CharacterMesh* rightLowerArmMesh;
    CharacterMesh* rightHandMesh;
    CharacterMesh* upperTorsoMesh;
    CharacterMesh* lowerTorsoMesh;

    bool isBodyPart(Aya::PartInstance* part) const;
    bool isBodyPartComposited(Aya::PartInstance* part) const;
    bool isPartComposited(Aya::PartInstance* part) const;
    bool isPartHead(Aya::PartInstance* part) const;

    // helper
    CharacterMesh* getRelevantMesh(Aya::PartInstance* bodyPart) const;

    enum BodyPartType
    {
        PartType_Head,
        PartType_Torso,
        PartType_Arm,
        PartType_Leg,
        PartType_UpperTorso,
        PartType_LowerTorso,
        PartType_LowerArm,
        PartType_UpperArm,
        PartType_Hand,
        PartType_UpperLeg,
        PartType_LowerLeg,
        PartType_Foot,
        PartType_Unknown,
        PartType_Count
    };

    BodyPartType getBodyPartType(Aya::PartInstance* bodyPart) const;
    Vector3 getBodyPartScale(Aya::PartInstance* bodyPart) const;
};
} // namespace Aya
