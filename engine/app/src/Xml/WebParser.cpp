

#include "Xml/WebParser.hpp"
#include "Xml/XmlSerializer.hpp"
#include "Xml/Serializer.hpp"
#include "make_shared.hpp"
#include "SafeToLower.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace Aya
{
bool WebParser::parseWebListResponse(std::istream& stream, Aya::Reflection::ValueArray& result)
{
    TextXmlParser machine(stream.rdbuf());
    std::auto_ptr<XmlElement> root(machine.parse());
    if (root->getTag() == tag_WebList)
    {
        return loadList(root.get(), result);
    }
    return false;
}
bool WebParser::parseWebGenericResponse(std::istream& stream, Aya::Reflection::Variant& result)
{
    TextXmlParser machine(stream.rdbuf());
    std::auto_ptr<XmlElement> root(machine.parse());
    return parseWebGenericResponse(root.get(), result);
}
bool WebParser::parseWebGenericResponse(const XmlElement* root, Aya::Reflection::Variant& result)
{
    if (root->getTag() == tag_WebTable)
    {
        shared_ptr<Reflection::ValueMap> table(new Reflection::ValueMap());
        if (loadTable(root, *table))
        {
            result = shared_ptr<const Aya::Reflection::ValueMap>(table);
            return true;
        }
    }
    else if (root->getTag() == tag_WebList)
    {
        shared_ptr<Reflection::ValueArray> list(Aya::make_shared<Reflection::ValueArray>());
        if (loadList(root, *list))
        {
            result = shared_ptr<const Aya::Reflection::ValueArray>(list);
            return true;
        }
    }
    else if (root->getTag() == tag_WebValue)
    {
        if (loadValue(root, result))
        {
            return true;
        }
    }
    return false;
}
bool WebParser::loadList(const XmlElement* listElement, Aya::Reflection::ValueArray& result)
{
    const XmlElement* childEntry = listElement->findFirstChildByTag(tag_WebValue);
    while (childEntry)
    {
        Reflection::Variant value;
        if (loadValue(childEntry, value))
        {
            result.push_back(value);
        }
        else
        {
            return false;
        }
        childEntry = listElement->findNextChildWithSameTag(childEntry);
    }
    return true;
}
bool WebParser::loadTable(const XmlElement* tableElement, Aya::Reflection::ValueMap& result)
{
    const XmlElement* childEntry = tableElement->findFirstChildByTag(tag_WebEntry);
    while (childEntry)
    {
        std::string key;
        Reflection::Variant value;
        if (loadEntry(childEntry, key, value))
        {
            result[key] = value;
        }
        else
        {
            return false;
        }
        childEntry = tableElement->findNextChildWithSameTag(childEntry);
    }
    return true;
}
bool WebParser::loadValue(const XmlElement* valueElement, Aya::Reflection::Variant& value)
{
    if (const XmlElement* tableElement = valueElement->findFirstChildByTag(tag_WebTable))
    {
        shared_ptr<Reflection::ValueMap> table(new Reflection::ValueMap());
        if (loadTable(tableElement, *table))
        {
            value = shared_ptr<const Aya::Reflection::ValueMap>(table);
            return true;
        }
    }
    else if ((tableElement = valueElement->findFirstChildByTag(tag_WebList)))
    {
        shared_ptr<Reflection::ValueArray> list(Aya::make_shared<Reflection::ValueArray>());
        if (loadList(tableElement, *list))
        {
            value = shared_ptr<const Aya::Reflection::ValueArray>(list);
            return true;
        }
    }
    else
    {
        if (const XmlAttribute* typeAttribute = valueElement->findAttribute(tag_WebType))
        {
            std::string type;
            if (typeAttribute->getValue(type))
            {
                if (type == "boolean")
                {
                    bool boolResult;
                    if (valueElement->getValue(boolResult))
                    {
                        value = boolResult;
                        return true;
                    }
                }
                else if (type == "string")
                {
                    std::string stringResult;
                    if (valueElement->getValue(stringResult))
                    {
                        value = stringResult;
                        return true;
                    }
                }
                else if (type == "number")
                {
                    double doubleResult;
                    if (valueElement->getValue(doubleResult))
                    {
                        value = doubleResult;
                        return true;
                    }
                }
                else if (type == "integer")
                {
                    int intResult;
                    if (valueElement->getValue(intResult))
                    {
                        value = intResult;
                        return true;
                    }
                }
                else if (type == "instance")
                {
                    if (const XmlElement* robloxRoot = valueElement->findFirstChildByTag(tag_roblox))
                    {
                        Serializer serializer;
                        Instances instances;
                        serializer.loadInstancesFromText(robloxRoot, instances);
                        if (instances.size() == 1)
                        {
                            value = instances[0];
                            return true;
                        }
                    }
                }
                return false;
            }
        }

        // fallback to legacy parsing
        if (valueElement->isValueType<std::string>())
        {
            std::string stringResult;
            if (valueElement->getValue(stringResult))
            {
                std::string lowerStringResult = stringResult;
                safeToLower(lowerStringResult);
                if (lowerStringResult == "true")
                    value = true;
                else if (lowerStringResult == "false")
                    value = false;
                else
                    value = stringResult;
                return true;
            }
        }
    }
    return false;
}
bool WebParser::loadEntry(const XmlElement* entryElement, std::string& key, Aya::Reflection::Variant& value)
{
    bool gotKey = false;
    bool gotValue = false;
    if (const XmlElement* keyElement = entryElement->findFirstChildByTag(tag_WebKey))
    {
        if (keyElement->isValueType<std::string>())
        {
            if (keyElement->getValue(key))
            {
                gotKey = true;
            }
        }
    }

    if (const XmlElement* valueElement = entryElement->findFirstChildByTag(tag_WebValue))
    {
        gotValue = loadValue(valueElement, value);
    }
    return gotKey && gotValue;
}

Reflection::Variant populateValueTableFromRapidJson(rapidjson::Value& node)
{
    Reflection::Variant v;
    if (node.IsInt())
        v = node.GetInt();
    else if (node.IsNumber())
        v = node.GetDouble();
    else if (node.IsBool())
        v = node.GetBool();
    else if (node.IsString())
        v = std::string(node.GetString());
    else if (node.IsObject())
    {
        shared_ptr<Reflection::ValueTable> subMap = Aya::make_shared<Reflection::ValueTable>();
        for (auto it = node.MemberBegin(); it != node.MemberEnd(); ++it)
        {
            Reflection::Variant subValue = populateValueTableFromRapidJson(it->value);
            (*subMap)[it->name.GetString()] = subValue;
        }
        v = shared_ptr<const Reflection::ValueTable>(subMap);
    }
    else if (node.IsArray())
    {
        shared_ptr<Reflection::ValueArray> subArray = Aya::make_shared<Reflection::ValueArray>();
        for (auto it = node.Begin(); it != node.End(); ++it)
        {
            Reflection::Variant subValue = populateValueTableFromRapidJson(*it);
            subArray->push_back(subValue);
        }
        v = shared_ptr<const Reflection::ValueArray>(subArray);
    }
    return v;
}

bool WebParser::parseJSONObject(const std::string& rawWebResponse, Reflection::Variant& result)
{
    rapidjson::Document root;
    root.Parse(rawWebResponse.c_str());

    if (root.HasParseError())
        return false;

    AYAASSERT(root.IsObject() || root.IsArray());

    result = populateValueTableFromRapidJson(root);

    return true;
}

bool checkStringForASCII(const std::string& value)
{
    for (char c : value)
    {
        if (c < 0)
            return false;
    }
    return true;
}

bool writeToRapidJSON(
    const Reflection::Variant& value, rapidjson::Value& node, WebParser::NonJSONBehavior skip, rapidjson::Document::AllocatorType& allocator)
{
    if (value.isVoid())
        node.SetNull();
    else if (value.isFloat())
        node.SetDouble(value.get<double>());
    else if (value.isType<bool>())
        node.SetBool(value.get<bool>());
    else if (value.isNumber())
        node.SetInt(value.get<int>());
    else if (value.isString())
    {
        std::string strValue = value.get<std::string>();
        if (!checkStringForASCII(strValue))
            return false;

        node.SetString(strValue.c_str(), allocator);
    }
    else if (value.isType<shared_ptr<const Reflection::ValueTable>>())
    {
        node.SetObject();
        shared_ptr<const Reflection::ValueTable> table = value.get<shared_ptr<const Reflection::ValueTable>>();

        for (Reflection::ValueTable::const_iterator it = table->begin(); it != table->end(); ++it)
        {
            rapidjson::Value keyName(it->first.c_str(), allocator);
            rapidjson::Value subNode;

            if (!writeToRapidJSON(it->second, subNode, skip, allocator))
                return false;

            node.AddMember(keyName, subNode, allocator);
        }
    }
    else if (value.isType<shared_ptr<const Reflection::ValueArray>>())
    {
        node.SetArray();
        shared_ptr<const Reflection::ValueArray> array = value.get<shared_ptr<const Reflection::ValueArray>>();

        for (Reflection::ValueArray::const_iterator it = array->begin(); it != array->end(); ++it)
        {
            rapidjson::Value subNode;

            if (!writeToRapidJSON(*it, subNode, skip, allocator))
                return false;

            node.PushBack(subNode, allocator);
        }
    }
    else if (skip == WebParser::SkipNonJSON)
    {
        node.SetNull();
    }
    else
    {
        return false;
    }
    return true;
}

bool WebParser::writeJSON(const Reflection::Variant& value, std::string& result, NonJSONBehavior skip)
{
    rapidjson::Document root;
    if (!writeToRapidJSON(value, root, skip, root.GetAllocator()))
        return false;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    root.Accept(writer);

    result = buffer.GetString();

    return true;
}

bool WebParser::parseJSONTable(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueTable>& valueTable)
{
    Reflection::Variant result;
    if (!parseJSONObject(rawWebResponse, result))
        return false;

    if (!result.isType<shared_ptr<const Reflection::ValueTable>>())
        return false;

    valueTable = result.cast<shared_ptr<const Aya::Reflection::ValueTable>>();
    return true;
}

bool WebParser::parseJSONArray(const std::string& rawWebResponse, shared_ptr<const Reflection::ValueArray>& valueArray)
{
    Reflection::Variant result;
    if (!parseJSONObject(rawWebResponse, result))
        return false;

    if (!result.isType<shared_ptr<const Reflection::ValueArray>>())
        return false;

    valueArray = result.cast<shared_ptr<const Aya::Reflection::ValueArray>>();
    return true;
}

} // namespace Aya
