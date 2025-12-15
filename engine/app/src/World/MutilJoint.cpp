


#include "World/MultiJoint.hpp"
#include "World/Primitive.hpp"
#include "World/Tolerance.hpp"
#include "Kernel/Kernel.hpp"
#include "Kernel/Connector.hpp"
#include "Kernel/Point.hpp"

namespace Aya
{

void MultiJoint::init(int numBreaking)
{
    numBreakingConnectors = numBreaking;
    numConnector = 0;
    for (int j = 0; j < 4; j++)
    {
        point[j * 2] = NULL;
        point[j * 2 + 1] = NULL;
        connector[j] = NULL;
    }
}

MultiJoint::MultiJoint(int numBreaking)
{
    init(numBreaking);
}

MultiJoint::MultiJoint(Primitive* p0, Primitive* p1, const CoordinateFrame& jointCoord0, const CoordinateFrame& jointCoord1, int numBreaking)
    : Joint(p0, p1, jointCoord0, jointCoord1)
{
    init(numBreaking);
}

MultiJoint::~MultiJoint()
{
    AYAASSERT(connector[0] == NULL);
    AYAASSERT(numConnector == 0);
}

void MultiJoint::putInKernel(Kernel* _kernel)
{
    Super::putInKernel(_kernel);
}


float MultiJoint::getJointK()
{
    return G3D::min(getPrimitive(0)->getJointK(), getPrimitive(1)->getJointK());
}

void MultiJoint::addToMultiJoint(Point* point0, Point* point1, Connector* _connector)
{
    AYAASSERT(numConnector < 4);

    point[numConnector * 2] = point0;
    point[numConnector * 2 + 1] = point1;
    connector[numConnector] = _connector;
    getKernel()->insertConnector(_connector);

    numConnector++;

    AYAASSERT_IF_VALIDATING(validateMultiJoint());
}

Point* MultiJoint::getPoint(int id)
{
    AYAASSERT(id < 8);
    AYAASSERT(point[id] != NULL);
    AYAASSERT(point[id]->getBody() == getPrimitive(id % 2)->getBody());
    return point[id];
}

Connector* MultiJoint::getConnector(int id)
{
    AYAASSERT(id < 4);
    AYAASSERT(connector[id] != NULL);
    return connector[id];
}

#ifdef __AYA_VALIDATE_ASSERT
bool MultiJoint::pointsAligned() const
{
    AYAASSERT(numBreakingConnectors <= numConnector);

    // THIS ASSERT WILL BLOW in the situation where "unaligned" joints
    // are present on save or sleep.  For now, just watching to see how often
    // this occurs
    // If too often - will need to store joint information or implement wheel/holes
    //
    for (int i = 0; i < numBreakingConnectors; ++i)
    {
        if (Tolerance::pointsUnaligned(point[i * 2]->getWorldPos(), point[i * 2 + 1]->getWorldPos()))
        {
            return false;
        }
    }
    return true;
}
#endif


bool MultiJoint::validateMultiJoint()
{
#ifdef _DEBUG
    for (int i = 0; i < numConnector; ++i)
        for (int j = 0; j < 2; ++j)
        {
            {
                Point* p = point[i * 2 + j];
                AYAASSERT(p->getBody() == this->getPrimitive(j)->getBody());

                Connector* c = connector[i];
                AYAASSERT(c->getBody((Connector::BodyIndex)j) == this->getPrimitive(j)->getBody());
            }
        }
#endif
    return true;
}


void MultiJoint::removeFromKernel()
{
    AYAASSERT(this->inKernel());

    // TODO - unsuppress this - indicates something that will NOT rejoin!
    //	AYAASSERT_IF_VALIDATING(pointsAligned());

    AYAASSERT_IF_VALIDATING(validateMultiJoint());

    for (int i = 0; i < numConnector; i++)
    {
        AYAASSERT(connector[i]);

        getKernel()->removeConnector(connector[i]);
        delete connector[i];
        getKernel()->deletePoint(point[i * 2]);
        getKernel()->deletePoint(point[i * 2 + 1]);

        point[i * 2] = NULL;
        point[i * 2 + 1] = NULL;
        connector[i] = NULL;
    }

    numConnector = 0;

    Super::removeFromKernel();

    AYAASSERT(!this->inKernel());
}

bool MultiJoint::isBroken() const
{
    ///	int max = std::min(numConnector, numBreakingConnectors);
    AYAASSERT(numBreakingConnectors <= numConnector);
    AYAASSERT(this->inKernel());

    //	ToDo:  Saving joints, otherwise a save in this configuration could cause broken joints.
    //	AYAASSERT_IF_VALIDATING(pointsAligned());

    for (int i = 0; i < numBreakingConnectors; ++i)
    {
        AYAASSERT(connector[i]);
        if (connector[i]->getBroken())
        {
            return true;
        }
    }
    return false;
}


} // namespace Aya

// Randomized Locations for hackflags
namespace Aya
{
namespace Security
{
unsigned int hackFlag10 = 0;
};
}; // namespace Aya
