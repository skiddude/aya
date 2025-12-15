#pragma once
#include "Debug.hpp"
#include <string>

namespace Aya
{
class StringReadBuffer
{
    std::string::const_iterator cur;
    const std::string& buffer;

public:
    StringReadBuffer(const std::string& str)
        : buffer(str)
    {
        cur = buffer.begin();
    }

    StringReadBuffer& operator>>(unsigned char& value)
    {
        AYAASSERT(cur != buffer.end());
        if (cur == buffer.end())
        {
            throw Aya::runtime_error("Reading past end of string");
        }
        else
        {
            value = *cur++;
        }

        return *this;
    }

    bool eof()
    {
        return cur == buffer.end();
    }
};

class StringWriteBuffer
{
    std::string buffer;

public:
    StringWriteBuffer()
        : buffer(std::string())
    {
    }

    StringWriteBuffer& operator<<(unsigned char value)
    {
        buffer.push_back(value);
        return *this;
    }

    const std::string& str()
    {
        return buffer;
    }
};
} // namespace Aya