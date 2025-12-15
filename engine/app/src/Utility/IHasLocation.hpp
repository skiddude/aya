

#pragma once

#include "Utility/G3DCore.hpp"
#include "Declarations.hpp"

namespace Aya
{

//
// http://www.parashift.com/c++-faq-lite/multiple-inheritance.html#faq-25.10

// This is a virtual base class - see note above.  Any object that descends from it
// should use the "virtual" keyword, so only one is included.
//
class AyaInterface IHasLocation
{
public:
    virtual const CoordinateFrame getLocation() = 0;

    virtual ~IHasLocation() {}
};

} // namespace Aya
