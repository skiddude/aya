

#pragma once

#include "Debug.hpp"
#include <vector>


namespace Aya
{

// only fast for very short vectors - should do no allocation
// returns index of the item removed

template<class T>
void fastRemoveIndex(std::vector<T>& vec, size_t index)
{
    AYAASSERT(index >= 0);
    AYAASSERT(index < vec.size());
    AYAASSERT(!vec.empty());
    AYAASSERT(vec.size() < 32); // Note - possibly should be using some other container here - to find item requires N time
#ifdef _DEBUG
    size_t currentCapacity = vec.capacity();
#endif
    size_t newSize = vec.size() - 1;

    if (index < newSize)
    {
        vec[index] = vec.back();
    }
    vec.resize(newSize); // hopefully, don't do a memory realloc/shrink

#ifdef _DEBUG
    AYAASSERT(currentCapacity == vec.capacity()); // confirm no reallocation
#endif
}


// only fast for very short vectors - should do no allocation
// returns index of the item removed
template<class T>
size_t fastRemoveShort(std::vector<T>& vec, const T& item)
{
    typename std::vector<T>::iterator it = std::find(vec.begin(), vec.end(), item);

    size_t answer = it - vec.begin();

    AYAASSERT(vec[answer] == item);
    AYAASSERT(it != vec.end());
    AYAASSERT(vec.size() < 32); // Note - possibly should be using some other container here - to find item requires N time
#ifdef _DEBUG
    size_t currentCapacity = vec.capacity();
#endif

    typename std::vector<T>::iterator lastOne(vec.end());
    --lastOne;

    AYAASSERT(*lastOne == vec.back());

    if (it != lastOne)
    {
        *it = *lastOne; // move back item into the place once held by item
    }
    vec.resize(vec.size() - 1); // hopefully, don't do a memory realloc/shrink

#ifdef _DEBUG
    AYAASSERT(currentCapacity == vec.capacity()); // confirm no reallocation
#endif
    return answer;
}



} // namespace Aya
