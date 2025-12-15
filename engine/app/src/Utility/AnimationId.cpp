


#include "DataModel/ContentProvider.hpp"
#include "Utility/AnimationId.hpp"


#include "DataModel/Workspace.hpp"
#include "DataModel/Camera.hpp"

#include "Utility/StandardOut.hpp"

namespace Aya
{

// TODO: Refactor: It is a little ugly to have to implement these 6 functions for each "ContentID" derived class
//  Potentially we can template this a little. Maybe define a templated ContentIDPropDescriptor or something.
template<>
std::string StringConverter<AnimationId>::convertToString(const AnimationId& value)
{
    return value.toString();
}

template<>
bool StringConverter<AnimationId>::convertToValue(const std::string& text, AnimationId& value)
{
    value = text;
    return true;
}

namespace Reflection
{

template<>
const Type& Type::getSingleton<Aya::AnimationId>()
{
    return Type::singleton<ContentId>();
}


template<>
void TypedPropertyDescriptor<Aya::AnimationId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    if (!element->isXsiNil())
    {
        ContentId value;
        if (element->getValue(value))
            setValue(instance, value);
    }
}

template<>
void TypedPropertyDescriptor<Aya::AnimationId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    element->setValue(ContentId(getValue(instance)));
}



template<>
Aya::AnimationId& Variant::convert<AnimationId>(void)
{
    if (_type->isType<std::string>())
    {
        value = Aya::AnimationId(cast<std::string>());
        _type = &Type::singleton<AnimationId>();
    }
    return genericConvert<Aya::AnimationId>();
}

template<>
int TypedPropertyDescriptor<Aya::AnimationId>::getDataSize(const DescribedBase* instance) const
{
    return sizeof(Aya::AnimationId) + getValue(instance).toString().size();
}

template<>
bool TypedPropertyDescriptor<Aya::AnimationId>::hasStringValue() const
{
    return true;
}

template<>
std::string TypedPropertyDescriptor<Aya::AnimationId>::getStringValue(const DescribedBase* instance) const
{
    return StringConverter<Aya::AnimationId>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<Aya::AnimationId>::setStringValue(DescribedBase* instance, const std::string& text) const
{
    Aya::AnimationId value;
    if (StringConverter<Aya::AnimationId>::convertToValue(text, value))
    {
        setValue(instance, value);
        return true;
    }
    else
        return false;
}

} // namespace Reflection
} // namespace Aya