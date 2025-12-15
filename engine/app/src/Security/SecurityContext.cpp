

#include "Security/SecurityContext.hpp"
#include "Thread.hpp"
#include "Debug.hpp"

namespace Aya
{
namespace Security
{
bool Context::isInRole(Identities identity, Permissions p)
{
    if (p == None)
        return true;

    switch (identity)
    {
    case Anonymous:
    case GameScript_:
        return false;
#if defined(AYA_STUDIO)
    case StudioPlugin:
        return p == Plugin;
#endif
    case GameScriptInRobloxPlace_:
        return p == RobloxPlace;
    case RobloxGameScript_:
        return p == Plugin || p == RobloxPlace || p == LocalUser || p == RobloxScript;
    case LocalGUI_:
    case CmdLine_:
        return p == Plugin || p == RobloxPlace || p == LocalUser;
    case Replicator_:
        return p == RobloxPlace || p == WritePlayer || p == RobloxScript;
    case COM:
    case WebService:
        return true;
    default:
        AYAASSERT(false);
        return false;
    }
}

Context& Context::current()
{
    Context* t = ptr().get();
    if (!t)
    {
        static Context anonymous(Anonymous);
        t = &anonymous;
        ptr().reset(t);
    }
    return *t;
}


void Context::tssCleanup(Context*) {}

boost::thread_specific_ptr<Context>& Context::ptr()
{
    static boost::thread_specific_ptr<Context> value(tssCleanup);
    return value;
}

Impersonator::Impersonator(Identities identity)
    : current(identity)
{
    previous = Context::ptr().get();
    Context::ptr().reset(&current);
}

Impersonator::~Impersonator()
{
    Context::ptr().reset(previous);
}


} // namespace Security
} // namespace Aya
