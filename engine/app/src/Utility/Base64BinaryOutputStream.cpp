
#include "Utility/Base64BinaryOutputStream.hpp"

#include "Debug.hpp"

namespace Aya
{

const char* Base64BinaryOutputStream::kTranslateToBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                           "abcdefghijklmnopqrstuvwxyz"
                                                           "0123456789"
                                                           "+/";

Base64BinaryOutputStream::Base64BinaryOutputStream()
    : buffer(0)
    , bitsUsed(0)
{
}

size_t Base64BinaryOutputStream::GetNumberOfBytesUsed() const
{
    // for the time being, only needed to satisfy templatization
    AYAASSERT(false);
    throw std::runtime_error("Base64BinaryOutputStream::GetNumberOfBytesUsed Not Implemented");
}

void Base64BinaryOutputStream::WriteBits(const unsigned char* doNotReadFromThisVariableAfterFirstRead, size_t numBitsToAdd)
{
    AYAASSERT(numBitsToAdd <= 8);
    AYAASSERT(numBitsToAdd >= 1);
    AYAASSERT(doNotReadFromThisVariableAfterFirstRead);

    unsigned char dataCopy = *doNotReadFromThisVariableAfterFirstRead;

    while (numBitsToAdd > 0)
    {
        // clip off higher order bits: only the "numBitsToAdd" (remaining)
        // least significant bits should be put into the stream
        dataCopy &= ((1 << numBitsToAdd) - 1);

        size_t bitsThatFit = std::min(numBitsToAdd, 6 - bitsUsed);

        buffer <<= bitsThatFit;
        buffer |= dataCopy >> (numBitsToAdd - bitsThatFit);

        bitsUsed += bitsThatFit;
        AYAASSERT(bitsUsed <= 6);
        if (bitsUsed == 6)
        {
            result << ((const char)kTranslateToBase64[buffer]);

            buffer = 0;
            bitsUsed = 0;
        }

        numBitsToAdd -= bitsThatFit;
    }
}

void Base64BinaryOutputStream::done(std::string* out)
{
    if (bitsUsed > 0)
    {
        unsigned char zero = 0;
        WriteBits(&zero, 6 - bitsUsed);
    }
    AYAASSERT(bitsUsed == 0);
    AYAASSERT(buffer == 0);

    (*out) = result.str();
}

} // namespace Aya
