

#include "Utility/Guid.hpp"
#include "atomic.hpp"
#include "Reflection/Reflection.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <algorithm>
#include <string>
#include <random>
#include "Utility/Utilities.hpp"
#include "Utility/Object.hpp"

static boost::uuids::basic_random_generator<std::mt19937> gen;


namespace Aya
{
template<>
bool StringConverter<Aya::Guid::Data>::convertToValue(const std::string& text, Aya::Guid::Data& value)
{
    AYAASSERT(false); // not fully implemented
    return false;
}
namespace Reflection
{
template<>
Aya::Guid::Data& Variant::convert<Aya::Guid::Data>(void)
{
    return genericConvert<Aya::Guid::Data>();
}

template<>
const Type& Type::getSingleton<Guid::Data>()
{
    static TType<Guid::Data> type("GuidData");
    return type;
}
} // namespace Reflection
} // namespace Aya

static Aya::atomic<int> nextIndex = 0;
static Aya::Guid::Scope* localScope;
Aya::Guid::Scope Aya::Guid::Scope::nullScope;

const Aya::Guid::Scope& Aya::Guid::Scope::null()
{
    return nullScope;
}

static void initLocalScope()
{
    Aya::Guid::Scope scope;
    Aya::Guid::generateRBXGUID(scope);

    // Note: localScope has to be a pointer to avoid initialization order fiasco between localScope and getLocalScope() callers
    // We never free this memory because we don't really need to and that avoids a symmetrical problem during deinitialization
    // (see SAFE_HEAP_STATIC in threadsafe.h)
    localScope = new Aya::Guid::Scope(scope);
}

const Aya::Guid::Scope& Aya::Guid::getLocalScope()
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&initLocalScope, flag);
    return *localScope;
}

Aya::Guid::Guid()
{
    data.scope = getLocalScope();
    data.index = ++nextIndex;
}

void Aya::Guid::generateStandardGUID(std::string& result)
{
    boost::uuids::random_generator generator;
    boost::uuids::uuid uuid = generator();
    std::string uuid_string = boost::uuids::to_string(uuid);
    std::transform(uuid_string.begin(), uuid_string.end(), uuid_string.begin(), ::toupper);
    result = std::string("{" + uuid_string + "}");
}

void Aya::Guid::generateRBXGUID(Aya::Guid::Scope& result)
{
    std::string tmp;
    generateRBXGUID(tmp);

    result.set(tmp);
}

void Aya::Guid::generateRBXGUID(std::string& result)
{
    // Start with a text character (so it conforms to xs:ID requirement)
    generateStandardGUID(result);
    result = "RBX" + result;

    // result looks like this: RBX{c200e360-38c5-11ce-ae62-08002b2b79ef}

    // strip the {}- characters
    result.erase(40, 1);
    result.erase(27, 1);
    result.erase(22, 1);
    result.erase(17, 1);
    result.erase(12, 1);
    result.erase(3, 1);

    // result looks like this: RBXc200e36038c511ceae6208002b2b79ef

    // TODO: This string could be more compact if we included g-z
}

void Aya::Guid::assign(Data data)
{
    this->data = data;
}


bool Aya::Guid::Data::operator==(const Aya::Guid::Data& other) const
{
    return index == other.index && scope == other.scope;
}

bool Aya::Guid::Data::operator<(const Aya::Guid::Data& other) const
{
    const int compare = scope.compare(other.scope);

    if (compare < 0)
        return true;
    if (compare > 0)
        return false;

    return index < other.index;
}

int Aya::Guid::compare(const Guid* a, const Guid* b)
{
    const Scope& sa = a ? a->data.scope : Scope::null();
    const Scope& sb = b ? b->data.scope : Scope::null();

    const int compare = sa.compare(sb);
    if (compare)
        return compare;
    int ia = a ? a->data.index : 0;
    int ib = b ? b->data.index : 0;
    return ia - ib;
}

int Aya::Guid::compare(const Guid* a0, const Guid* a1, const Guid* b0, const Guid* b1)
{
    int aComp = compare(a0, a1);
    int bComp = compare(b0, b1);

    const Guid* aMax = (aComp > 0) ? a0 : a1;
    const Guid* bMax = (bComp > 0) ? b0 : b1;

    const int compare = Guid::compare(aMax, bMax);
    if (compare)
        return compare;

    const Guid* aMin = (aComp > 0) ? a1 : a0;
    const Guid* bMin = (bComp > 0) ? b1 : b0;

    return Guid::compare(aMin, bMin);
}

#pragma warning(push)
#pragma warning(disable : 4996) //  warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

std::string Aya::Guid::Data::readableString(int scopeLength) const
{

    char result[64];
    if (scopeLength > 0)
    {
        std::string s = scope.getName()->toString();
        s = s.substr(std::min((size_t)3, s.size()), std::min(scopeLength, 32));
        sprintf(result, "%s_%d", s.c_str(), index);
    }
    else
        sprintf(result, "%d", index);
    return result;
}

#pragma warning(pop)
