#include "RobloxUser.hpp"

// Roblox headers
#include "AuthenticationHelper.hpp"

RobloxUser& RobloxUser::singleton()
{
    static RobloxUser user;
    return user;
}

RobloxUser::RobloxUser()
    : m_webKitUserId(-1)
{
    connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
}

RobloxUser::~RobloxUser()
{
    //
}

void RobloxUser::init()
{
    if (m_webKitUserId == -1)
        getWebkitUserId();
}

// Keep this user up to sync on auth changes
void RobloxUser::onAuthenticationChanged(bool)
{
    m_webKitUserId = -1;
    getWebkitUserId();
}

void RobloxUser::currentUserReplied(Aya::HttpFuture future)
{
    m_webKitUserId = 0;
}

void RobloxUser::getWebkitUserId()
{
    //
}

int RobloxUser::getUserId()
{
    //

    m_webKitUserId = 0;

    return m_webKitUserId;
}
