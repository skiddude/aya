

#include "Utility/Region3Int16.hpp"
#include "Utility/ExtentsInt32.hpp"
#include "Reflection/Type.hpp"
namespace Aya
{

namespace Reflection
{
template<>
const Type& Type::getSingleton<Region3int16>()
{
    static TType<Region3int16> type("Region3int16");
    return type;
}
} // namespace Reflection

} // namespace Aya