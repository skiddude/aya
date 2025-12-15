#pragma once

#include "Utility/Name.hpp"
#include "boost/utility.hpp"
#include <boost/thread/mutex.hpp>

namespace Aya
{
namespace Reflection
{
class Descriptor : public boost::noncopyable
{

    static void checkLockedDown()
    {
        // If this following assertion fails then you need to put your class
        // into FactoryRegistrator::FactoryRegistrator() or somewhere else.
        // Failure of this test is so severe that we want to catch it in production, too.
        if (lockedDown)
            AYACRASH();
    }

public:
    struct Attributes
    {
        bool isDeprecated;
        const Descriptor* preferred; // used if isDeprecated
        Attributes()
            : isDeprecated(false)
            , preferred(NULL)
        {
        }
        static Attributes deprecated(const Descriptor& preferred)
        {
            Attributes result;
            result.isDeprecated = true;
            result.preferred = &preferred;
            return result;
        }
        static Attributes deprecated()
        {
            Attributes result;
            result.isDeprecated = true;
            return result;
        }
    };

    static bool lockedDown; // After the first instance of a described class is created we cannot modify the reflection database

    const Aya::Name& name;
    scoped_ptr<bool> isReplicable;
    scoped_ptr<bool> isOutdated;
    const Attributes attributes;

    Descriptor(const char* name, Attributes attributes)
        : name(Aya::Name::declare(name))
        , attributes(attributes)
        , isReplicable(new bool(false))
        , isOutdated(new bool(false))
    {
        checkLockedDown();
        AYAASSERT(!this->name.empty());
    }
    Descriptor(const Aya::Name& name, Attributes attributes)
        : name(name)
        , attributes(attributes)
        , isReplicable(new bool(false))
        , isOutdated(new bool(false))
    {
        checkLockedDown();
        AYAASSERT(!this->name.empty());
    }
    virtual ~Descriptor() {}
};
} // namespace Reflection
} // namespace Aya
