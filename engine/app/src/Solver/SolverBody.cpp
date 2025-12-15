

#include "Solver/SolverBody.hpp"
#include "Solver/DebugSerializer.hpp"

namespace Aya
{

void SolverBodyDynamicProperties::serialize(DebugSerializer& s) const
{
    s & integratedLinearVelocity & integratedAngularVelocity & orientation & position & linearVelocity & angularVelocity;
}

void SolverBodyStaticProperties::serialize(DebugSerializer& s) const
{
    s & bodyUID & guid & isStatic;
}

void SymmetricMatrix::serialize(DebugSerializer& s) const
{
    s & diagonals & offDiagonals;
}

void SolverBodyMassAndInertia::serialize(DebugSerializer& s) const
{
    s & massInvVelStage;
    SymmetricMatrix invInertia;
    invInertia.diagonals = inertiaDiagonal;
    invInertia.offDiagonals = inertiaOffDiagonal;
    s & invInertia;
}

} // namespace Aya
