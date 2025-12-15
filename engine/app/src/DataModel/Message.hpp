

#pragma once

#include "Tree/Instance.hpp"
#include "Base/IAdornable.hpp"
#include "Utility/ContentFilter.hpp"
#include <string>

namespace Aya
{

extern const char* const sMessage;
class Message
    : public DescribedCreatable<Message, Instance, sMessage>
    , public IAdornable
{
private:
    typedef DescribedCreatable<Message, Instance, sMessage> Super;

protected:
    std::string text;
    void renderFullScreen(Adorn* adorn);

public:
    Message();
    ~Message();

    /*override*/ bool shouldRender2d() const
    {
        return true;
    }
    /*override*/ void render2d(Adorn* adorn);

    /*override*/ bool askSetParent(const Instance* instance) const
    {
        return true;
    }

    const std::string& getText() const;
    void setText(const std::string& value);


    /*override*/ int getPersistentDataCost() const
    {
        return Super::getPersistentDataCost() + Instance::computeStringCost(getText());
    }
};

} // namespace Aya