

#pragma once

#include "Tree/Instance.hpp"
#include "Base/IAdornable.hpp"
#include "DataModel/Effect.hpp"
#include "DataModel/PartInstance.hpp"

namespace Aya
{
class PartInstance;
class ModelInstance;

extern const char* const sSparkles;
class Sparkles
    : public DescribedCreatable<Sparkles, Instance, sSparkles>
    , public Effect

{
private:
    bool enabled;

    Color3 color;

public:
    Sparkles();
    virtual ~Sparkles() {}

    bool getEnabled() const
    {
        return enabled;
    }
    void setColor(Color3 color);
    Color3 getColor() const
    {
        return color;
    }
    void setLegacyColor(Color3 color);
    Color3 getLegacyColor() const;

    static Reflection::BoundProp<bool> prop_Enabled;
    static Reflection::PropDescriptor<Sparkles, Color3> prop_Color;

    void onChangedEnabled(const Reflection::PropertyDescriptor&);

protected:
    bool askSetParent(const Instance* parent) const
    {
        return Instance::fastDynamicCast<PartInstance>(parent) != NULL;
    }
    bool askAddChild(const Instance* instance) const
    {
        return true;
    }
};
} // namespace Aya