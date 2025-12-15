#pragma once
#include "Tree/Instance.hpp"
#include "Utility/TweenInfo.hpp"
#include "DataModel/TweenBase.hpp"
#include "Utility/UDim.hpp"

namespace Aya
{
extern const char* const sTween;
class Tween : public DescribedNonCreatable<Tween, TweenBase, sTween, Reflection::ClassDescriptor::RUNTIME_LOCAL>
{
private:
    TweenInfo twnInfo;
    shared_ptr<Instance> instance;

public:
    Tween(shared_ptr<Instance> instance, TweenInfo twnInfo, shared_ptr<const Reflection::ValueArray> propTable);

    Instance* getInstance() const
    {
        return instance.get();
    }
    TweenInfo getTweenInfo() const
    {
        return twnInfo;
    }
};

}; // Namespace Aya