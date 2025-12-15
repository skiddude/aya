


#include "Xml/SerializerV2.hpp"
#include "Xml/XmlSerializer.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Debug.hpp"
#include "Utility/StandardOut.hpp"

#include "Xml/SerializerBinary.hpp"

#include <map>

#if defined(__linux) || defined(__APPLE__)
#include <algorithm>  // for std::count_if
#include <functional> // for std::function, std::bind
#endif

// using std::mem_fun;

#ifdef _WIN64
using std::mem_fun;
#else
#ifdef __APPLE__
using std::mem_fn;
#endif
#endif

using std::string;
using std::vector;

using namespace Aya;

// An implementation of IReferenceBinder
class ArchiveBinder : public Aya::MergeBinder
{
private:
    typedef Aya::MergeBinder Super;
    // A map of temporary IDs to Referents
    std::map<std::string, InstanceHandle> idMap;

    struct IDREFBinding
    {
        const XmlNameValuePair* valueIDREF;
        Reflection::DescribedBase* propertyOwner;
        const IIDREF* idref;
    };
    std::list<IDREFBinding> idrefBindings;

public:
    virtual bool processID(const XmlNameValuePair* valueID, Reflection::DescribedBase* source)
    {
        if (!Super::processID(valueID, source))
        {
            std::string s;
            bool foundString = valueID->getValue(s);
            AYAASSERT(foundString);

            idMap[s].linkTo(shared_from(source));
            if (source)
            {
                source->setXmlId(s);
            }
        }
        return true;
    }
    virtual bool processIDREF(const XmlNameValuePair* valueIDREF, Reflection::DescribedBase* propertyOwner, const IIDREF* idref)
    {
        if (!Super::processIDREF(valueIDREF, propertyOwner, idref))
        {
            IDREFBinding binding = {valueIDREF, propertyOwner, idref};
            idrefBindings.push_back(binding);
        }
        return true;
    }

    bool resolveIDREF(IDREFBinding binding)
    {
        std::string s;
        bool foundString = binding.valueIDREF->getValue(s);
        AYAASSERT(foundString);

        // The following 3 cases should have been handled during the processIDREF phase
        AYAASSERT(value_IDREF_nil != s);
        AYAASSERT(value_IDREF_null != s);
        AYAASSERT(s != "");

        // Find the InstanceHandle belonging to the requested ID
        std::map<std::string, InstanceHandle>::iterator iter = idMap.find(s);
        if (iter != idMap.end())
        {
            // TODO: should we give the handle  over to valueIDREF?
            assign(binding.idref, binding.propertyOwner, iter->second);
            return true;
        }

        // Unable to find the requested InstanceHandle
        assign(binding.idref, binding.propertyOwner, InstanceHandle(NULL));
        return false;
    }

    bool resolveRefs()
    {
        int count = 0;

// aya - gcc
#ifdef _WIN32
        count += count_if(idrefBindings.begin(), idrefBindings.end(), std::bind1st(mem_fun(&ArchiveBinder::resolveIDREF), this));
#elif defined(__APPLE__)
        count += std::count_if(idrefBindings.begin(), idrefBindings.end(),
            [this](IDREFBinding& binding)
            {
                return this->resolveIDREF(binding);
            });
#else
        count += std::count_if(idrefBindings.begin(), idrefBindings.end(), std::bind(&ArchiveBinder::resolveIDREF, this, std::placeholders::_1));
#endif

        return Super::resolveRefs() && (count == idrefBindings.size());
    }
};

void SerializerV2::load(std::istream& stream, Aya::DataModel* dataModel)
{
    // See file format spec in CWorkspace::Save()
    char header[8];
    if (!stream.read(header, 8).good())
        throw std::runtime_error("SerializerV2::load can't read header");

    stream.clear();
    stream.seekg(0, std::ios::beg);

    if (memcmp(header, SerializerBinary::kMagicHeader, 8) == 0)
    {
        // read the binary content
        SerializerBinary::deserialize(stream, dataModel);
    }
    else
    {
        // read the XML content
        loadXML(stream, dataModel);
    }
}

void SerializerV2::loadInstances(std::istream& stream, Aya::Instances& result)
{
    char header[8];
    if (!stream.read(header, 8).good())
        throw std::runtime_error("SerializerV2::loadInstances can't read header");

    stream.clear();
    stream.seekg(0, std::ios::beg);

    if (memcmp(header, SerializerBinary::kMagicHeader, 8) == 0)
    {
        // read the binary content
        SerializerBinary::deserialize(stream, result);
    }
    else
    {
        // read the XML content
        TextXmlParser machine(stream.rdbuf());
        std::auto_ptr<XmlElement> root(machine.parse());

        ArchiveBinder binder;
        loadInstancesXML(root.get(), result, binder, Aya::SerializationCreator);
    }
}

void SerializerV2::loadXML(std::istream& stream, Aya::DataModel* dataModel)
{
    TextXmlParser machine(stream.rdbuf());
    std::auto_ptr<XmlElement> root(machine.parse());

    if (root->getTag() == tag_roblox)
    {
        if (const XmlAttribute* version = root->findAttribute(tag_version))
        {
            if (!version || !version->getValue(schemaVersionLoading))
            {
                throw std::runtime_error("SerializerV2::loadXML no version number");
            }
            else if (schemaVersionLoading < 4)
            {
                throw std::runtime_error("SerializerV2::loadXML schemaVersionLoading<4");
            }
            else
            {
                ArchiveBinder binder;

                dataModel->readChildren(root.get(), binder, SerializationCreator);

                binder.resolveRefs();
            }
        }
    }
    else
    {
        schemaVersionLoading = 1;
        throw std::runtime_error("SerializerV2::loadXML ill-formed XML. No Roblox tag");
    }

    // Should we need to do this???
    dataModel->setDirty(false);
}

shared_ptr<Instance> SerializerV2::loadInstanceXML(const XmlElement* itemElement, IReferenceBinder& binder, CreatorRole creatorRole)
{
    const Aya::Name* className = NULL;
    if (itemElement->findAttributeValue(tag_class, className))
    {
        shared_ptr<Instance> instance = Creatable<Instance>::createByName(*className, Aya::SerializationCreator);
        if (instance)
        {
            instance->read(itemElement, binder, creatorRole);
            return instance;
        }
        else
            StandardOut::singleton()->printf(MESSAGE_WARNING, "Unknown object class \"%s\" while reading XML", className ? className->c_str() : "");
    }
    return shared_ptr<Instance>();
}

void SerializerV2::loadInstancesFromText(const XmlElement* root, Instances& result)
{
    ArchiveBinder binder;
    loadInstancesXML(root, result, binder, Aya::SerializationCreator);
}

void SerializerV2::loadInstancesXML(const XmlElement* root, Instances& result, IReferenceBinder& binder, CreatorRole creatorRole)
{
    // TODO: This code has a lot in common with Instance::readChildren and with SerializerV2::merge
    //       Find a way of combining these code chunks
    bool v4model = false;

    if (root->getTag() == tag_roblox)
    {
        const XmlAttribute* version = root->findAttribute(tag_version);
        if (version != NULL && version->getValue(schemaVersionLoading) && schemaVersionLoading >= 4)
        {
            v4model = true;

            const XmlElement* childElement = root->findFirstChildByTag(tag_Item);
            while (childElement)
            {
                if (shared_ptr<Instance> instance = loadInstanceXML(childElement, binder, creatorRole))
                    result.push_back(instance);
                childElement = root->findNextChildWithSameTag(childElement);
            }
            binder.resolveRefs();
            // TODO: ASSERT
            // AYAASSERT(resolvedBindings);
        }
    }
};



XmlElement* SerializerV2::newRootElement()
{
    return newRootElement("");
}
XmlElement* SerializerV2::newRootElement(const std::string& type)
{
    static const XmlTag& tag_xmlnsxmime = Name::declare("xmlns:xmime");

    XmlElement* root = new XmlElement(tag_roblox);
    root->addAttribute(tag_xmlnsxmime, "http://www.w3.org/2005/05/xmlmime");
    root->addAttribute(tag_xmlnsxsi, "http://www.w3.org/2001/XMLSchema-instance");
    root->addAttribute(tag_version, SerializerV2::CURRENT_SCHEMA_VERSION);
    if (!type.empty())
    {
        root->addAttribute(tag_assettype, type);
    }

    // Used for schema validation with Roblox.xsd:
    root->addChild(new XmlElement(tag_External, &value_IDREF_null));
    root->addChild(new XmlElement(tag_External, &value_IDREF_nil));

    return root;
}