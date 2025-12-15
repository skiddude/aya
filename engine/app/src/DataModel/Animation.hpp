
#pragma once

#include "Tree/Instance.hpp"
#include "Utility/AnimationId.hpp"

namespace Aya
{
extern const char* const sAnimation;

class KeyframeSequence;
class Animation : public DescribedCreatable<Animation, Instance, sAnimation>
{
private:
    typedef DescribedCreatable<Animation, Instance, sAnimation> Super;
    ContentId assetId;

public:
    Animation();

    shared_ptr<const KeyframeSequence> getKeyframeSequence() const;

    shared_ptr<const KeyframeSequence> getKeyframeSequence(const Instance* context) const;

    AnimationId getAssetId() const
    {
        return assetId;
    }
    void setAssetId(AnimationId value);

    bool isEmbeddedAsset() const;

    /*override*/ bool askSetParent(const Instance* instance) const
    {
        return true;
    }
    /*override*/ int getPersistentDataCost() const
    {
        return Super::getPersistentDataCost() + Instance::computeStringCost(getAssetId().toString());
    }
};

} // namespace Aya
