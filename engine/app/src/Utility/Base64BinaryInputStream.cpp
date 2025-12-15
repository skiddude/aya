
#include "Utility/Base64BinaryInputStream.hpp"

#include "Debug.hpp"

namespace Aya
{

unsigned char Base64BinaryInputStream::decode(unsigned char charFromString)
{
    if (charFromString >= 'A' && charFromString <= 'Z')
    {
        return charFromString - 'A';
    }
    if (charFromString >= 'a' && charFromString <= 'z')
    {
        return 26 + (charFromString - 'a');
    }
    if (charFromString >= '0' && charFromString <= '9')
    {
        return 52 + (charFromString - '0');
    }
    if (charFromString == '+')
    {
        return 62;
    }
    if (charFromString == '/')
    {
        return 63;
    }
    AYAASSERT(false);
    return 0;
}

Base64BinaryInputStream::Base64BinaryInputStream(const char* source)
    : source(source)
    , buffer(0)
    , readableBitsInBuffer(0)
{
}

void Base64BinaryInputStream::ReadBits(unsigned char* output, size_t numBitsToRead)
{
    AYAASSERT(numBitsToRead <= 8);
    AYAASSERT(numBitsToRead >= 1);

    while (numBitsToRead > readableBitsInBuffer)
    {
        boost::uint16_t nextValue = decode(*source);
        source++;

        // decode produced a 6 bit number. Shift that to the head (16 - 6 = 10),
        // leaving room for the readableBits
        nextValue <<= (10 - readableBitsInBuffer);
        buffer |= nextValue;
        readableBitsInBuffer += 6;
    }

    AYAASSERT(numBitsToRead <= readableBitsInBuffer);

    unsigned char tmp = buffer >> (16 - numBitsToRead);
    (*output) = tmp;
    buffer <<= numBitsToRead;
    readableBitsInBuffer -= numBitsToRead;
}

} // namespace Aya
