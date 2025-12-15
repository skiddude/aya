

#include "BoostAppend.hpp"

#include "intrusive_ptr_target.hpp"

#include "DataModel/PartInstance.hpp"

#include <boost/multi_index/hashed_index.hpp>

std::size_t boost::hash_value(const shared_ptr<Aya::PartInstance>& b)
{
    boost::hash<void*> hasher;
    return hasher(b.get());
}
