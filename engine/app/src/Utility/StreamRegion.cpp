

#include "Utility/StreamRegion.hpp"

namespace Aya
{
namespace StreamRegion
{

std::size_t hash_value(const Id& key)
{
    Id::boost_compatible_hash_value hash;
    return hash(key);
}
} // namespace StreamRegion
} // namespace Aya