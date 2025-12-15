#include "Random.hpp"
#include "vectorMath.hpp"
#include <random>

namespace Aya
{
Random::Random(int seed)
    : generator(seed)
{
}

Random::Random(RandomGenerator gen)
    : generator(gen)
{
}

int Random::nextInteger(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

float Random::nextNumber()
{
    std::uniform_real_distribution<float> distribution(0, 1);
    return distribution(generator);
}

float Random::nextNumber(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

G3D::Vector3 Random::nextUnitVector()
{
    G3D::Vector3 v;
    v.x = nextNumber(-1, 1);
    v.y = nextNumber(-1, 1);
    v.z = nextNumber(-1, 1);
    return G3D::normalize(v);
}

Random Random::clone() const
{
    Random newRandom(generator);
    return newRandom;
}


bool operator==(const Random& lhs, const Random& rhs)
{
    return false;
}
} // namespace Aya
