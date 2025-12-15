

#pragma once

#include <string>
#include <map>
#include "RakNet/BitStream.hpp"
#include "boost/noncopyable.hpp"
#include "Reflection/Property.hpp"
#include "Utility/Guid.hpp"

#include "Utility/BinaryString.hpp"

#include "StreamingUtil.hpp"

namespace Aya
{

namespace Network
{
// Must be a power of 2
#define DICTIONARY_SIZE 128

// A very simple dictionary of up to 127 items.
// Sending the same item a 2nd time costs only 8 bits.
// The algorithm is extremely simple - there's no fancy cache expiration.
// When we run out of dictionary space we simply replace the oldest item
// first.  (It's a cyclic buffer)
// To use, pair a SenderDictionary with a ReceiverDictionary.
template<class T>
class SenderDictionary
{
    typedef std::map<T, unsigned char> Dictionary;
    Dictionary dictionary;    // item --> id
    T items[DICTIONARY_SIZE]; // id --> item
    int nextIndex;
    static bool isDefaultValue(const T& value);

public:
    SenderDictionary()
        : nextIndex(1)
    {
    }
    bool canSend(const T& value)
    {
        typename Dictionary::const_iterator iter = dictionary.find(value);
        return (iter != dictionary.end());
    }
    bool trySend(RakNet::BitStream& stream, const T& value)
    {
        if (isDefaultValue(value))
        {
            // id==0 is reserved for the empty item
            unsigned char id = 0;
            stream.Write(id);
            return true;
        }

        typename Dictionary::const_iterator iter = dictionary.find(value);
        if (iter == dictionary.end())
            return false;

        // The item already exists in the dictionary. Simply send the ID
        unsigned char id = iter->second;
        AYAASSERT((id & 0x80) == 0);
        AYAASSERT((id & 0x7F) < DICTIONARY_SIZE);

        stream.Write(id);
        return true;
    }

    void send(RakNet::BitStream& stream, const T& value)
    {
        if (isDefaultValue(value))
        {
            // id==0 is reserved for the empty item
            unsigned char id = 0;
            stream.Write(id);
            return;
        }

        std::pair<typename Dictionary::iterator, bool> pair = dictionary.insert(typename Dictionary::value_type(value, nextIndex));
        if (!pair.second)
        {
            // The item already exists in the dictionary. Simply send the current ID
            unsigned char id = pair.first->second;
            stream.Write(id);
        }
        else
        {
            // Remove the pre-existing entry from dictionary
            dictionary.erase(items[nextIndex]);

            // Put the new entry into the item table
            items[nextIndex] = value;

            // Send the id with 0x80 bit set to indicate this is a new item
            unsigned char id = nextIndex | 0x80;
            stream.Write(id); // TODO: We could get away with just sending a single bit. The receiver knows what the new index should be
            stream << value;

            // Advance nextIndex == [1..127]
            nextIndex %= (DICTIONARY_SIZE - 1);
            ++nextIndex;
        }
    }

    void sendEmptyItem(RakNet::BitStream& stream)
    {
        // id==0 is reserved for the empty item
        unsigned char id = 0;
        stream.Write(id);
        return;
    }
};

template<>
class SenderDictionary<const Aya::Name*>
{
    typedef boost::unordered_map<const Aya::Name*, unsigned char> Dictionary;
    Dictionary dictionary;             // item --> id
    Aya::Name* items[DICTIONARY_SIZE]; // id --> item
    int nextIndex;
    static bool isDefaultValue(const std::string& value)
    {
        return value.empty();
    }

public:
    SenderDictionary()
        : nextIndex(1)
    {
    }
    bool canSend(const Aya::Name* value)
    {
        Dictionary::const_iterator iter = dictionary.find(value);
        return (iter != dictionary.end());
    }
    bool trySend(RakNet::BitStream& stream, const Aya::Name* value)
    {
        if (isDefaultValue(value->toString()))
        {
            // id==0 is reserved for the empty item
            unsigned char id = 0;
            stream.Write(id);
            return true;
        }

        Dictionary::const_iterator iter = dictionary.find(value);
        if (iter == dictionary.end())
            return false;

        // The item already exists in the dictionary. Simply send the ID
        unsigned char id = iter->second;
        AYAASSERT((id & 0x80) == 0);
        AYAASSERT((id & 0x7F) < DICTIONARY_SIZE);

        stream.Write(id);
        return true;
    }

    void send(RakNet::BitStream& stream, const Aya::Name* value);

    void sendEmptyItem(RakNet::BitStream& stream)
    {
        // id==0 is reserved for the empty item
        unsigned char id = 0;
        stream.Write(id);
        return;
    }
};

template<class T>
class ReceiverDictionary
{
    T dictionary[DICTIONARY_SIZE];
    static void setDefault(T& t);

public:
    /*void skip(RakNet::BitStream& stream)
    {
            unsigned char id;
            stream.Read(id);

            if (id == 0)
            {
            }
            else if (id & 0x80)
            {
                    T value;
                    // The item is not in the dictionary
                    stream >> value;
            }
    }*/
    // d9mz - this is a hack to get around a compiler bug in VS2005
    //        (BUT WE'RE NOT USING VS2005 ARE WE)
    // error: '#pragma optimize' can only appear at file scope
    // #pragma optimize( "", off )
    void learn(unsigned char id, const T& value)
    {
        dictionary[id] = value;
    }
    bool get(unsigned char id, T& value)
    {
        value = dictionary[id];
        return true;
    }
    bool receive(RakNet::BitStream& stream, T& value)
    {
        unsigned char id;
        stream.Read(id);

        if (id == 0)
        {
            setDefault(value);
            return true;
        }
        else if (id & 0x80)
        {
            // The item is not in the dictionary
            stream >> value;
            learn(id & 0x7F, value);
            return true;
        }
        else
        {
            // the item is in the dictionary
            return get(id, value);
        }
    }
    // d9mz - this is a hack to get around a compiler bug in VS2005
    //        (BUT WE'RE NOT USING VS2005 ARE WE)
    // error: '#pragma optimize' can only appear at file scope
    // #pragma optimize( "", on )
};

class ReceiverStringDictionary
{
    std::string dictionary[DICTIONARY_SIZE];
    std::auto_ptr<std::size_t> hashTable;
    static void setDefault(std::string& t);

    bool protection;

public:
    ReceiverStringDictionary(bool protection)
        : protection(protection)
    {
    }
    /*void skip(RakNet::BitStream& stream)
    {
            unsigned char id;
            stream.Read(id);

            if (id == 0)
            {
            }
            else if (id & 0x80)
            {
                    T value;
                    // The item is not in the dictionary
                    stream >> value;
            }
    }*/
    // d9mz - this is a hack to get around a compiler bug in VS2005
    //        (BUT WE'RE NOT USING VS2005 ARE WE)
    // error: '#pragma optimize' can only appear at file scope
    // #pragma optimize( "", off )
    void learn(unsigned char id, const std::string& value);
    bool get(unsigned char id, std::string& value);
    template<class T>
    bool receive(RakNet::BitStream& stream, T& value)
    {
        unsigned char id;
        stream.Read(id);

        if (id == 0)
        {
            setDefault(value);
            return true;
        }
        else if (id & 0x80)
        {
            // The item is not in the dictionary
            stream >> value;
            learn(id & 0x7F, value);
            return true;
        }
        else
        {
            // the item is in the dictionary
            return get(id, value);
        }
    }
    // d9mz - this is a hack to get around a compiler bug in VS2005
    //        (BUT WE'RE NOT USING VS2005 ARE WE)
    // error: '#pragma optimize' can only appear at file scope
    // #pragma optimize( "", on )
};


template<class T>
class SharedDictionary
    : public SenderDictionary<T>
    , public ReceiverDictionary<T>
    , boost::noncopyable
{
};

class SharedStringDictionary
    : public SenderDictionary<std::string>
    , public ReceiverDictionary<std::string>
    , boost::noncopyable
{
public:
    void serializeString(const std::string& value, RakNet::BitStream& bitStream);
    void serializeString(const Reflection::ConstProperty& property, RakNet::BitStream& bitStream);
    void send(RakNet::BitStream& stream, const Aya::Name& value)
    {
        SenderDictionary<std::string>::send(stream, value.toString());
    }
    void send(RakNet::BitStream& stream, const char* value)
    {
        SenderDictionary<std::string>::send(stream, std::string(value));
    }
    bool trySend(RakNet::BitStream& stream, const Aya::Name& value)
    {
        return SenderDictionary<std::string>::trySend(stream, value.toString());
    }
    bool trySend(RakNet::BitStream& stream, const char* value)
    {
        return SenderDictionary<std::string>::trySend(stream, std::string(value));
    }
    void deserializeString(std::string& value, RakNet::BitStream& bitStream);
    void deserializeString(Reflection::Property& property, RakNet::BitStream& bitStream);
    void receive(RakNet::BitStream& stream, const Aya::Name*& value)
    {
        std::string s;
        ReceiverDictionary<std::string>::receive(stream, s);
        value = &Aya::Name::declare(s.c_str());
    }
};


class SharedStringProtectedDictionary
    : public SenderDictionary<std::string>
    , public ReceiverStringDictionary
    , boost::noncopyable
{
public:
    SharedStringProtectedDictionary(bool protection);
    void serializeString(const std::string& value, RakNet::BitStream& bitStream);
    void serializeString(const Reflection::ConstProperty& property, RakNet::BitStream& bitStream);
    void send(RakNet::BitStream& stream, const Aya::Name& value)
    {
        SenderDictionary<std::string>::send(stream, value.toString());
    }
    void send(RakNet::BitStream& stream, const char* value)
    {
        SenderDictionary<std::string>::send(stream, std::string(value));
    }
    bool trySend(RakNet::BitStream& stream, const Aya::Name& value)
    {
        return SenderDictionary<std::string>::trySend(stream, value.toString());
    }
    bool trySend(RakNet::BitStream& stream, const char* value)
    {
        return SenderDictionary<std::string>::trySend(stream, std::string(value));
    }
    bool deserializeString(std::string& value, RakNet::BitStream& bitStream);
    bool deserializeString(Reflection::Property& property, RakNet::BitStream& bitStream);
    void receive(RakNet::BitStream& stream, const Aya::Name*& value)
    {
        std::string s;
        ReceiverStringDictionary::receive(stream, s);
        // TODO: If there's a bug, then this could grow the Aya::Name database in
        // and unbounded fashion. Use lookup() instead
        value = &Aya::Name::declare(s.c_str());
    }
};

class SharedBinaryStringDictionary
    : public SenderDictionary<BinaryString>
    , public ReceiverDictionary<BinaryString>
    , boost::noncopyable
{
public:
    void serializeString(const BinaryString& value, RakNet::BitStream& bitStream);
    void serializeString(const Reflection::ConstProperty& property, RakNet::BitStream& bitStream);

    void deserializeString(BinaryString& value, RakNet::BitStream& bitStream);
    void deserializeString(Reflection::Property& property, RakNet::BitStream& bitStream);
};
} // namespace Network
} // namespace Aya
