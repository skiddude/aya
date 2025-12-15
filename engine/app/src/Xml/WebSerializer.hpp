#pragma once

#include "Reflection/Type.hpp"

class XmlElement;

namespace Aya
{
class WebSerializer
{
public:
    static XmlElement* writeTable(const Aya::Reflection::ValueMap& result);
    static XmlElement* writeList(const Aya::Reflection::ValueArray& result);
    static XmlElement* writeEntry(const std::string& key, const Aya::Reflection::Variant& value);
    static XmlElement* writeValue(const Aya::Reflection::Variant& value);
};
} // namespace Aya
