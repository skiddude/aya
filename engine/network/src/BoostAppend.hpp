#pragma once

#include "boost.hpp"


namespace Aya
{
class PartInstance;
}


namespace boost
{
std::size_t hash_value(const shared_ptr<Aya::PartInstance>& b);
}
