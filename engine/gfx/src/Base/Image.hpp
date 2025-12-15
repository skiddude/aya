#pragma once

#include <stddef.h>

namespace Aya
{

class Image
{
public:
    virtual ~Image() {}

    virtual size_t getSize() const = 0;

    virtual int getOriginalWidth() const = 0;
    virtual int getOriginalHeight() const = 0;
};

} // namespace Aya
