#ifndef AYA_TIME_H
#define AYA_TIME_H
#include "G3DGameUnits.hpp"

namespace Aya
{
class RbxTime
{
public:
    static G3D::RealTime getTick();

private:
    static G3D::RealTime m_startTime;
};
} // namespace Aya

#endif