#pragma once

#include "Utility/TextureId.hpp"
#include "Utility/MeshId.hpp"
#include "CharacterAppearance.hpp"

namespace Aya
{

class Humanoid;

extern const char* const sCharacterMesh;
class CharacterMesh : public DescribedCreatable<CharacterMesh, CharacterAppearance, sCharacterMesh>
{
public:
    enum BodyPart
    {
        HEAD = 0,
        TORSO = 1,
        LEFTARM = 2,
        RIGHTARM = 3,
        LEFTLEG = 4,
        RIGHTLEG = 5
    };

    CharacterMesh();


    BodyPart getBodyPart() const
    {
        return bodyPart;
    }
    void setBodyPart(BodyPart value);


    int64_t baseTextureAssetId;
    int64_t overlayTextureAssetId;
    int64_t meshAssetId;

    static Reflection::BoundProp<int64_t> prop_baseTextureAssetId;
    static Reflection::BoundProp<int64_t> prop_overlayTextureAssetId;
    static Reflection::BoundProp<int64_t> prop_meshAssetId;

    TextureId getBaseTextureId() const;
    TextureId getOverlayTextureId() const;
    MeshId getMeshId() const;

protected:
    virtual void applyByMyself(Humanoid* humanoid);
    /*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);

private:
    BodyPart bodyPart;
};


} // namespace Aya
