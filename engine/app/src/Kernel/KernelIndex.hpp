

#pragma once

#include "Debug.hpp"

namespace Aya
{

class KernelIndex
{
protected:
    int kernelIndex;

public:
    bool indexInKernel() const
    {
        return (kernelIndex != -1);
    }

    KernelIndex()
        : kernelIndex(-1)
    {
    }

    ~KernelIndex()
    {
        AYAASSERT(!indexInKernel());
    }
};

} // namespace Aya