#pragma once

#include <stdexcept>

namespace Aya
{

class initialization_error : public std::runtime_error
{
public:
    initialization_error(const char* const errorMessage)
        : std::runtime_error(errorMessage)
    {
    }
};

} // namespace Aya