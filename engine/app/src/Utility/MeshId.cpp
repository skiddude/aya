


#include "DataModel/ContentProvider.hpp"
#include "Utility/MeshId.hpp"


#include "DataModel/Workspace.hpp"
#include "DataModel/Camera.hpp"

#include "Utility/StandardOut.hpp"

namespace Aya
{
// TODO: Refactor: It is a little ugly to have to implement these 6 functions for each "ContentID" derived class
//  Potentially we can template this a little. Maybe define a templated ContentIDPropDescriptor or something.
template<>
std::string StringConverter<MeshId>::convertToString(const MeshId& value)
{
    return value.toString();
}

template<>
bool StringConverter<MeshId>::convertToValue(const std::string& text, MeshId& value)
{
    value = text;
    return true;
}

namespace Reflection
{
template<>
const Reflection::Type& Reflection::Type::getSingleton<MeshId>()
{
    return Reflection::Type::singleton<ContentId>();
}


template<>
void TypedPropertyDescriptor<Aya::MeshId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    if (!element->isXsiNil())
    {
        ContentId value;
        if (element->getValue(value))
            setValue(instance, value);
    }
}

template<>
void TypedPropertyDescriptor<Aya::MeshId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    element->setValue(ContentId(getValue(instance)));
}



template<>
Aya::MeshId& Variant::convert<MeshId>(void)
{
    if (_type->isType<std::string>())
    {
        value = Aya::MeshId(cast<std::string>());
        _type = &Type::singleton<MeshId>();
    }
    return genericConvert<Aya::MeshId>();
}

template<>
int TypedPropertyDescriptor<Aya::MeshId>::getDataSize(const DescribedBase* instance) const
{
    return sizeof(Aya::MeshId) + getValue(instance).toString().size();
}

template<>
bool TypedPropertyDescriptor<Aya::MeshId>::hasStringValue() const
{
    return true;
}

template<>
std::string TypedPropertyDescriptor<Aya::MeshId>::getStringValue(const DescribedBase* instance) const
{
    return StringConverter<Aya::MeshId>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<Aya::MeshId>::setStringValue(DescribedBase* instance, const std::string& text) const
{
    Aya::MeshId value;
    if (StringConverter<Aya::MeshId>::convertToValue(text, value))
    {
        setValue(instance, value);
        return true;
    }
    else
        return false;
}

} // namespace Reflection
} // namespace Aya