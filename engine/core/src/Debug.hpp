#pragma once

#include "AyaPlatform.hpp"
#include "AyaAssert.hpp"
#include "AyaFormat.hpp"
#include <set>
#include <ostream>
#include <fstream>
#include <assert.h>

#if (defined(_DEBUG) && defined(_WIN32))
#include <crtdbg.h>
#endif

#ifdef __ANDROID__
#include <typeinfo>
#include <cstdlib>
#endif

#ifdef _WIN32
#undef min
#undef max
#endif

#include "Declarations.hpp"
#include "FastLog.hpp"

#ifndef _WIN32
#define __noop
inline void DebugBreak()
{
#if defined(__i386__)
    // gcc on intel
    __asm__ __volatile__("int $3");
#else
    // some other gcc
    ::abort();
#endif
}
#endif

LOGGROUP(Asserts)

/* Overview of builds and switches:

AYAASSERT:					Standard assert.  Should be reasonably fast.  Do not do "finds" or complex stuff here.  Simple bools,
simple math, a couple levels of pointer indirection, etc. AYAASSERT_VERY_FAST:		High fr  equency, extremely fast assert.  Not in regular debug
build because frequency too high.  Mostly inner engine stuff AYAASSERT_SLOW:				Put things like "find" here.  Will always run
in debug builds AYAASSERT_IF_VALIDATING:	Very slow stuff.  Only turns on if the "validating debug" switch is turned on in debug or noOpt build
AYAASSERT_FISHING:			Usually doesn't go off, should be safe - turn on for engine testing

                                                        AYAASSERT()			AYAASSERT_VERY_FAST()	AYAASSERT_SLOW()
AYAASSERT_IF_VALIDATING()	AYAASSERT_FISHING() DEBUG						X					X
X							X							- NoOpt
X					X						-							-
- ReleaseAssert				X					-						-
-							- Release						- -
-							-							-

*/


#ifdef _DEBUG
#define __AYA_VERY_FAST_ASSERT
#define __AYA_VALIDATE_ASSERT
//	#define __AYA_SLOW_ASSERT	// TODO: Hire a physics guy to enable them
//	#define __AYA_FISHING_ASSERT
#define __AYA_NOT_RELEASE
#endif

#ifdef _NOOPT
#define __AYA_CRASH_ON_ASSERT
#define __AYA_VERY_FAST_ASSERT
#define __AYA_NOT_RELEASE
#endif

namespace Aya
{

// Used for memory leak detection and other stuff
class Debugable
{
public:
    // this is here as a last chance way to debug an assert build, force assertions on, but not crash
    static volatile bool doCrashEnabled;

    static void doCrash();
    static void doCrash(const char*);

    static void* badMemory()
    {
        return reinterpret_cast<void*>(0x00000003);
    } // set values to this when deleting to check if ever coming back
};

} // namespace Aya

void AYACRASH();
void AYACRASH(const char* message);

void ReleaseAssert(int channel, const char* msg);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// macro to convince a compiler a variable is used while not generating instructions (useful for removing warnings)
#define AYA_UNUSED(x) (void)(sizeof((x), 0))

// This macro will cause a crash. Usually you don't call it directly. Use AYAASSERT instead
#define AYA_CRASH_ASSERT(expr) ((void)(!!(expr) || (Aya::Debugable::doCrash(#expr), 0)))

// This macro will just log an assert string, if we will run into crash log with the assert information will be sent to us
#define AYA_LOG_ASSERT(expr)

// LEGACY_ASSERT should be used when we have some assert bogging us and it seems like this guy is a good candidate for removal
// usage just replace AYAASSERT with LEGACY_ASSERT and it will gone by default, but if you need to see it temporary define FIRE_LEGACY_ASSERT
#undef FIRE_LEGACY_ASSERT

#ifdef FIRE_LEGACY_ASSERT
#define LEGACY_ASSERT(expr) AYAASSERT(expr)
#else
#define LEGACY_ASSERT(expr) ((void)0)
#endif

#define AYAASSERTENABLED

// AYAASSERT()
//
#ifdef __AYA_CRASH_ON_ASSERT
#define AYAASSERT AYA_CRASH_ASSERT
#else
#if (defined(_DEBUG) && defined(__APPLE__)) // Apple Debug
#include "TargetConditionals.hpp"
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define AYAASSERT AYA_LOG_ASSERT // iOS has no way to step over asserts (makes debugging hard)
#else
#define AYAASSERT(expr) assert(expr)
#define AYAASSERTENABLED
#endif
#elif (defined(_DEBUG) && defined(_WIN32)) // Windows Debug
/*
#define AYAASSERT(expr) \
        ((void) (!!(expr) || \
        ((Aya::_internal::_debugHook != NULL) && (Aya::_internal::_debugHook(#expr, __FILE__, __LINE__))) || \
        (_ASSERTE(expr), 0)))
*/
#define AYAASSERT(expr) assert(expr)
#define AYAASSERTENABLED
#else // All Platform Release
#define AYAASSERT AYA_LOG_ASSERT
#endif
#endif



// AYAASSERT_VERY_FAST()
//
#ifdef __AYA_VERY_FAST_ASSERT
#define AYAASSERT_VERY_FAST(expr) AYAASSERT(expr)
#else
#define AYAASSERT_VERY_FAST(expr) ((void)0)
#endif


// AYAASSERT_SLOW()
//
#ifdef __AYA_SLOW_ASSERT
#define AYAASSERT_SLOW(expr) AYAASSERT(expr)
#else
#define AYAASSERT_SLOW(expr) ((void)0)
#endif


// AYAASSERT_FISHING)
//
#ifdef __AYA_FISHING_ASSERT
#define AYAASSERT_FISHING(expr) AYAASSERT(expr)
#else
#define AYAASSERT_FISHING(expr) ((void)0)
#endif


// AYAASSERT_IF_VALIDATING()
//
#ifdef __AYA_VALIDATE_ASSERT
#define AYAASSERT_IF_VALIDATING(expr) AYAASSERT((expr))

#else
#define AYAASSERT_IF_VALIDATING(expr) ((void)0)
#endif


// AYAASSERT_NOT_RELEASE()		make sure this code is not being compiled in release build
#ifdef __AYA_NOT_RELEASE
#define AYAASSERT_NOT_RELEASE() ((void)0)
#else
#define AYAASSERT_NOT_RELEASE() AYACRASH()
#endif


// Same as boost::polymorphic_downcast but with an AYAASSERT
template<class T, class U>
inline T aya_static_cast(U u)
{
    AYAASSERT_SLOW(dynamic_cast<T>(u) == u);
    return static_cast<T>(u);
}
