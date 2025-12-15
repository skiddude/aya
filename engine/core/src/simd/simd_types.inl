#pragma once

// Do not include this file directly

#include "simd/simd_types.hpp"

namespace Aya
{

namespace simd
{

template<class ElemType>
AYA_SIMD_INLINE v4<ElemType>::v4(const v4& u)
    : v(u.v)
{
}

template<class ElemType>
AYA_SIMD_INLINE v4<ElemType>::v4(const pod_t& u)
    : v(u)
{
}

template<class ElemType>
AYA_SIMD_INLINE void v4<ElemType>::operator=(const v4& u)
{
    v = u.v;
}

template<class ElemType>
AYA_SIMD_INLINE void v4<ElemType>::operator=(const pod_t& u)
{
    v = u;
}

template<class ElemType>
AYA_SIMD_INLINE v4<ElemType>::operator pod_t() const
{
    return v;
}

} // namespace simd

} // namespace Aya
