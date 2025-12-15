//
//  AdService.h
//
//

#pragma once

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "DataModel/UserInputService.hpp"

namespace Aya
{
extern const char* const sAdService;
class AdService
    : public DescribedCreatable<AdService, Instance, sAdService, Reflection::ClassDescriptor::INTERNAL>
    , public Service
{
private:
    typedef DescribedCreatable<AdService, Instance, sAdService, Reflection::ClassDescriptor::INTERNAL> Super;

    bool showingVideoAd;

    std::string platformToWebString(const UserInputService::Platform userPlatform);
    bool canUseService();

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
    AdService();

    Aya::signal<void(bool)> videoAdClosedSignal;
    Aya::signal<void()> playVideoAdSignal;

    Aya::remote_signal<void(int, UserInputService::Platform)> sendServerVideoAdVerification;
    Aya::remote_signal<void(bool, int, std::string)> sendClientVideoAdVerificationResults;

    Aya::remote_signal<void(int, UserInputService::Platform, bool)> sendServerRecordImpression;

    void showVideoAd();
    void videoAdClosed(bool didPlay);

    void sendAdImpression(int userId, UserInputService::Platform platform, bool didPlay);

    void verifyCanPlayVideoAdReceivedResponseNoDMLock(const std::string& response, int userId);
    void verifyCanPlayVideoAdReceivedErrorNoDMLock(const std::string& error, int userId);

    void verifyCanPlayVideoAdReceivedError(const std::string& error, int userId);

    void checkCanPlayVideoAd(int userId, UserInputService::Platform userPlatform);
    void receivedServerShowAdMessage(const bool success, int userId, const std::string& errorMessage);
};
} // namespace Aya