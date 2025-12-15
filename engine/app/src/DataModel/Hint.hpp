

#pragma once

#include "DataModel/Message.hpp"
#include "Base/IAdornable.hpp"

namespace Aya
{

extern const char* const sHint;
class Hint
    : public DescribedCreatable<Hint, Instance, sHint>
    , public IAdornable
{
private:
    typedef DescribedCreatable<Hint, Instance, sHint> Super;

protected:
    std::string text;
    void renderFullScreen(Adorn* adorn);

public:
    Hint();
    ~Hint();

    void setText(const std::string& value);
    const std::string& getText() const;
    /*override*/ virtual void render2d(Adorn* adorn);
    /*override*/ bool canClientCreate()
    {
        return true;
    }

    /*override*/ bool shouldRender2d() const
    {
        return true;
    }

    /*override*/ bool askSetParent(const Instance* instance) const
    {
        return true;
    }

    /*override*/ int getPersistentDataCost() const
    {
        return Super::getPersistentDataCost() + Instance::computeStringCost(getText());
    }
};

} // namespace Aya