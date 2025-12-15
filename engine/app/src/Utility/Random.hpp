#pragma once

#include "Vector3.hpp"
#include <random>
namespace Aya
{
typedef std::default_random_engine RandomGenerator;

class Random
{
    int seed;
    RandomGenerator generator;

public:
    Random(int seed = 0);
    Random(RandomGenerator generator);

    int nextInteger(int min, int max);
    float nextNumber();
    float nextNumber(float min, float max);
    G3D::Vector3 nextUnitVector();

    Random clone() const;
};

bool operator==(const Random& lhs, const Random& rhs);

} // namespace Aya
