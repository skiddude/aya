

#pragma once

#include "World/Joint.hpp"
#include "Utility/Extents.hpp"
#include "Utility/Face.hpp"

namespace Aya
{

class Kernel;
class Channel;
class Connector;
class Point;
class Primitive;

class MultiJoint : public Joint
{
private:
    typedef Joint Super;
    int numConnector;
    Point* point[8];

    bool pointsAligned() const;
    void init(int numBreaking);
    int numBreakingConnectors;

    bool validateMultiJoint();

protected:
    //////////////////////////////////////////////////////////////
    //
    // Edge
    /*override*/ void putInKernel(Kernel* kernel);
    /*override*/ void removeFromKernel();

    //////////////////////////////////////////////////////////////
    //
    // Joint
    /*override*/ bool isBroken() const;

    Connector* connector[4]; // NormalBreakConnector

    void addToMultiJoint(Point* point0, Point* point1, Connector* _connector);
    Point* getPoint(int id);
    Connector* getConnector(int id);

    float getJointK();

    MultiJoint(int numBreaking);

    MultiJoint(Primitive* p0, Primitive* p1, const CoordinateFrame& jointCoord0, const CoordinateFrame& jointCoord1, int numBreaking);

    ~MultiJoint();
};

} // namespace Aya
