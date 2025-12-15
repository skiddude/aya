

#include "Utility/Region3.hpp"
#include "Utility/Extents.hpp"
#include "Reflection/Type.hpp"
namespace Aya
{

namespace Reflection
{
template<>
const Type& Type::getSingleton<Region3>()
{
    static TType<Region3> type("Region3");
    return type;
}
} // namespace Reflection
Region3::Region3() {}

Region3::Region3(const Vector3& min, const Vector3& max)
{
    init(Extents(min, max));
}

Region3::Region3(const Extents& extents)
{
    init(extents);
}

void Region3::init(const Extents& extents)
{
    cframe = CoordinateFrame(extents.center());
    size = extents.size();
}

Vector3 Region3::minPos() const
{
    return cframe.translation - (size * 0.5f);
}
Vector3 Region3::maxPos() const
{
    return cframe.translation + (size * 0.5f);
}

} // namespace Aya