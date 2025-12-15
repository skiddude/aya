

#include "Utility/KeywordFilter.hpp"
#include "Reflection/EnumConverter.hpp"

namespace Aya
{
namespace Reflection
{
template<>
Reflection::EnumDesc<KeywordFilterType>::EnumDesc()
    : Aya::Reflection::EnumDescriptor("KeywordFilterType")
{
    addPair(INCLUDE_KEYWORDS, "Include");
    addPair(EXCLUDE_KEYWORDS, "Exclude");
}
} // namespace Reflection
} // namespace Aya
