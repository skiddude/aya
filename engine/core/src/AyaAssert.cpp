/**
 @file debugAssert.cpp

 Windows implementation of assertion routines.

 @maintainer Morgan McGuire, graphics3d.com

 @created 2001-08-26
 @edited  2006-02-02
 */

#include "AyaPlatform.hpp" // includes <Windows.h>

#include "AyaAssert.hpp"
#include "AyaFormat.hpp"
#include <string>

using namespace std;

namespace Aya
{

namespace _internal
{
AssertionHook _debugHook;
AssertionHook _failureHook;
} // namespace _internal

void setAssertionHook(AssertionHook hook)
{
    Aya::_internal::_debugHook = hook;
}

AssertionHook assertionHook()
{
    return Aya::_internal::_debugHook;
}

void setFailureHook(AssertionHook hook)
{
    Aya::_internal::_failureHook = hook;
}

AssertionHook failureHook()
{
    return Aya::_internal::_failureHook;
}


} // namespace Aya
