#pragma once

#include "Reflection/Type.hpp"

class XmlElement;

namespace Aya
{
class WebParser
{
private:
    static boost::mutex JSONmutex;

public:
    typedef enum
    {
        FailOnNonJSON,
        SkipNonJSON
    } NonJSONBehavior;

    static bool parseWebGenericResponse(std::istream& stream, Aya::Reflection::Variant& result);
    static bool parseWebGenericResponse(const XmlElement* root, Aya::Reflection::Variant& result);
    static bool parseWebListResponse(std::istream& stream, Aya::Reflection::ValueArray& result);

    static bool parseJSONTable(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueTable>& valueTable);
    static bool parseJSONArray(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueArray>& valueArray);
    static bool parseJSONObject(const std::string& rawWebResponse, Reflection::Variant& result);

    static bool writeJSON(const Reflection::Variant& value, std::string& result, NonJSONBehavior skip = SkipNonJSON);

protected:
    static bool loadTable(const XmlElement* tableElement, Aya::Reflection::ValueMap& result);
    static bool loadList(const XmlElement* listElement, Aya::Reflection::ValueArray& result);
    static bool loadEntry(const XmlElement* entryElement, std::string& key, Aya::Reflection::Variant& value);
    static bool loadValue(const XmlElement* valueElement, Aya::Reflection::Variant& value);
};
} // namespace Aya
