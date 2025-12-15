#pragma once

// #include "World/Joint.hpp"

namespace Aya
{

class Joint;
class Primitive;

class JointBuilder
{
public:
    //		static Joint* makeJoint(Primitive* p0, Primitive* p1, const CoordinateFrame& c0, const CoordinateFrame& c1, Joint::JointType
    //jointType);

    static Joint* canJoin(Primitive* p0, Primitive* p1);
};

} // namespace Aya
