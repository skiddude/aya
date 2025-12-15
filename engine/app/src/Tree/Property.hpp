#pragma once

#include "Reflection/Property.hpp"
#include "Xml/Reference.hpp"
#include "Xml/XmlElement.hpp"
#include "Utility/Object.hpp"

#include "Vector3.hpp"
#include "Color3.hpp"
#include <string>
#include <iostream>
#include <vector>

namespace Aya
{

class PropertyChanged
{
    Aya::Reflection::Property property;

public:
    const Aya::Reflection::Property& getProperty() const
    {
        return property;
    }
    const Aya::Reflection::PropertyDescriptor& getDescriptor() const
    {
        return property.getDescriptor();
    }
    const Aya::Name& getName()
    {
        return property.getName();
    }

    PropertyChanged(const PropertyChanged& other)
        : property(other.property)
    {
    }

private:
    friend class Instance;
    PropertyChanged(const Aya::Reflection::Property& p)
        : property(p)
    {
    }
};

// TODO: Move this out of Reflection?
// Some standard categories
#define category_Data "Data"
#define category_Behavior "Behavior"
#define category_State "State"
#define category_Appearance "Appearance"
#define category_Team "Team"
#define category_Image "Image"
#define category_Video "Video"
#define category_Control "Control"

} // namespace Aya
