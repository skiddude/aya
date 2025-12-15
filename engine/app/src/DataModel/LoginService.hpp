//
//  LoginService.h
//  App
//
//  Created by Ben Tkacheff on 5/1/13.
//
//
#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"

namespace Aya
{

extern const char* const sLoginService;
class LoginService
    : public DescribedNonCreatable<LoginService, Instance, sLoginService>
    , public Service

{
public:
    LoginService();

    Aya::signal<void(std::string)> loginSucceededSignal;
    Aya::signal<void(std::string)> loginFailedSignal;

    Aya::signal<void()> promptLoginSignal;
    Aya::signal<void()> promptLogoutSignal;

    void promptSignup();
    void promptLogin();
    void logout();
};

} // namespace Aya