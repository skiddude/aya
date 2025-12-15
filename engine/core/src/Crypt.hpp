#pragma once

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
#include <Windows.h>
#include <wincrypt.h>
#endif

#include <string>

namespace Aya
{
class Crypt
{

public:
    Crypt();
    ~Crypt();
    void verifySignatureBase64(std::string message, std::string signatureBase64);
};
} // namespace Aya
