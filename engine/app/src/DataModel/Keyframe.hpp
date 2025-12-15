
#pragma once

#include "Tree/Instance.hpp"
#include "DataModel/Pose.hpp"

namespace Aya
{

extern const char* const sKeyframe;

class Keyframe : public DescribedCreatable<Keyframe, Instance, sKeyframe>
{
private:
    typedef DescribedCreatable<Keyframe, Instance, sKeyframe> Super;

protected:
    float time;

public:
    Keyframe();

    float getTime() const
    {
        return time;
    }
    void setTime(float value);

    shared_ptr<const Instances> getPoses();
    void addPose(shared_ptr<Instance>);
    void removePose(shared_ptr<Instance>);

    void invalidate();

protected:
    /*override*/ bool askAddChild(const Instance* instance) const
    {
        return Instance::fastDynamicCast<Pose>(instance) != NULL;
    }
    /*override*/ void onChildAdded(Instance* child)
    {
        invalidate();
    }
    /*override*/ void onChildRemoved(Instance* child)
    {
        invalidate();
    }
    /*override*/ void verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const;
};

} // namespace Aya
