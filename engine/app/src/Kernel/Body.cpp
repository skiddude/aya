#include "Kernel/Body.hpp"
#include "Kernel/Kernel.hpp"
#include "Kernel/Constants.hpp"
#include "Utility/Units.hpp"

#include "boost/utility.hpp"
#include "boost.hpp"
#include "atomic.hpp"

#include "btBulletCollisionCommon.hpp"

namespace Aya
{

Body* Body::worldBody;

static Aya::atomic<int> gBodyStateIndex;

Body::Body(float gravity)
    : connectorUseCount(0)
    , root(NULL)
    , link(NULL)
    , leafBodyIndex(-1)
    , canThrottle(true)
    , simBody(NULL)
    , moment(Matrix3::identity())
    , mass(0.0f)
    , cofmOffset(Vector3::zero())
    , stateIndex(getNextStateIndex())
    , cofm(NULL)
    , uid(0)
{
    root = this;
    simBody = new SimBody(this);
    this->gravity = gravity;
}

Body::~Body()
{
    AYAASSERT(connectorUseCount == 0);
    AYAASSERT(!link);
    AYAASSERT(simBody);
    AYAASSERT(leafBodyIndex == -1);

    if (cofm)
    {
        delete cofm;
        cofm = NULL;
    }
    delete simBody;
    simBody = NULL;

    AYAASSERT(!cofm);
    AYAASSERT(root == this);
}

unsigned int Body::getNextStateIndex()
{
    return ++gBodyStateIndex;
}

void Body::advanceStateIndex()
{
    stateIndex = getNextStateIndex();
}

void Body::initStaticData()
{
    worldBody = new Body();
}

Body* Body::getWorldBody()
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&initStaticData, flag);
    return worldBody;
}

bool Body::validateParentCofmDirty()
{
    AYAASSERT(cofm);
    AYAASSERT(cofm->getIsDirty());

    if (getParent())
    {
        getParent()->validateParentCofmDirty();
    }
    return true;
}


// Goes up the chain, making every Cofm dirty along the way
// In addition, forces the root body to become dirty
void Body::makeCofmDirty()
{
    if (cofm && cofm->getIsDirty())
    {
        AYAASSERT(this->validateParentCofmDirty());
        AYAASSERT(this->getRootSimBody()->getDirty());
    }
    else
    {
        if (getParent())
        {
            AYAASSERT(!simBody);
            getParent()->makeCofmDirty();
        }
        else
        {
            AYAASSERT(root == this);
            if (simBody)
            {
                simBody->makeDirty();
            }
        }

        if (cofm)
        {
            cofm->makeDirty();
            AYAASSERT((numChildren() > 0) || (cofmOffset != Vector3::zero()));
        }
        else
        {
            AYAASSERT(numChildren() == 0);
        }
    }
}

void Body::resetRoot(Body* newRoot)
{
    AYAASSERT(newRoot == calcRoot());
    root = newRoot;
    for (int i = 0; i < numChildren(); ++i)
    {
        getChild(i)->resetRoot(newRoot);
    }
}


void Body::onParentChanging()
{
    AYAASSERT_VERY_FAST(!getParent() || (!(getParent()->getConstRootSimBody()->isInKernel()))); // confirm not happening to bodies in kernel

    if (link)
    {
        link->setBody(NULL); // link must always set parent first, then
        link = NULL;
    }

    if (getParent())
    {
        AYAASSERT(root == getParent()->getRoot()); // I keep my pvIndex, as do my children
        AYAASSERT(!simBody);
    }
    else
    {
        AYAASSERT(root = this);
        AYAASSERT(simBody);
        delete simBody;
        simBody = NULL;
    }
}

void Body::onParentChanged(IndexedTree* oldParent)
{
    // confirm not happening to bodies in kernel
    AYAASSERT_VERY_FAST(!getParent() || (!(getParent()->getConstRootSimBody()->isInKernel())));

    if (getParent())
    {
        ;
    }
    else
    {
        simBody = new SimBody(this);
    }

    Body* newRoot = calcRoot();
    newRoot->advanceStateIndex();
    resetRoot(newRoot);
}


void Body::onChildAdding(IndexedTree* child)
{
    makeCofmDirty();
}

void Body::refreshCofm()
{
    bool hasChildren = (numChildren() > 0);
    bool hasCofmOffset = (cofmOffset != Vector3::zero());

    bool needsCofm = (hasChildren || hasCofmOffset);

    if (needsCofm)
    {
        if (!cofm)
        {
            cofm = new Cofm(this);
            AYAASSERT(cofm->getIsDirty());
        }
    }
    else
    { // doesn't need Cofm
        if (cofm)
        {
            delete cofm;
            cofm = NULL;
        }
    }
}


void Body::setCofmOffset(const Vector3& _centerOfMassInBody)
{

    if (cofmOffset != _centerOfMassInBody)
    {
        cofmOffset = _centerOfMassInBody;
        refreshCofm();
        makeCofmDirty(); // yes - we need to make dirty down the chain
    }
}

void Body::updatePV()
{
    AYAASSERT((getParent() != NULL) != (getRoot() == this));

    if (!getParent() || (stateIndex == getRoot()->getStateIndex()))
    {
        ;
    }
    else
    {
        getParent()->getPvUnsafe(); // do this first - prevent infinite recursion
        PV::pvAtLocalCoord(getParent()->getPvUnsafe(), getMeInParent(), pv);

        AYAASSERT(stateIndex != getRoot()->getStateIndex()); // concurrency check
        stateIndex = getRoot()->getStateIndex();

        AYAASSERT_SLOW(Math::isOrthonormal(pv.position.rotation));
        AYAASSERT_SLOW(!Math::isNanInfDenormVector3(pv.position.translation)); // - asserting all the time - John
    }
}

void Body::onChildAdded(IndexedTree* child)
{
    if (!cofm)
    {
        AYAASSERT(numChildren() == 1);
        refreshCofm();
        AYAASSERT(cofm);
    } // no need to make dirty - adding anyways
}

void Body::onChildRemoved(IndexedTree* child)
{
    AYAASSERT(cofm);
    if (numChildren() == 0)
    {
        refreshCofm();
    }
    makeCofmDirty();
}

// Note - need to reset the root state index, because this is what everyone uses to determine if they're in synch
//
//
void Body::setMeInParent(const CoordinateFrame& _meInParent)
{
    AYAASSERT_FISHING(Math::longestVector3Component(_meInParent.translation) < 1e6);

    AYAASSERT_FISHING(Math::isOrthonormal(_meInParent.rotation));

    if (link)
    {
        AYAASSERT(0);
        link->setBody(NULL);
        link = NULL;
    }

    if (getParent())
    {
        meInParent = _meInParent;
        makeCofmDirty();
        makeStateDirty();
    }
    else
    {
        AYAASSERT(0);
    }
}

void Body::setMeInParent(Link* _link)
{
    AYAASSERT(_link);

    if (link && (link != _link))
    {
        link->setBody(NULL);
    }

    if (getParent())
    {
        link = _link;
        link->setBody(this);
        makeCofmDirty();
        makeStateDirty();
    }
    else
    {
        AYAASSERT(0);
    }
}


void Body::setPv(const PV& _pv, const BodyPvSetter& bpv)
{
    if (getParent())
    {
        AYAASSERT(0); // bad news here - this object has a parent, so setting it's position should only be done
                      // by changing the parent's position, or meInParent
    }
    else
    {
        pv = _pv;
        if (simBody)
        {
            simBody->makeDirty();
        }
        advanceStateIndex(); // I'm the root
    }

    // AYAASSERT(Math::longestVector3Component(pv.position.translation) < 1e6);
    AYAASSERT_FISHING(Math::isOrthonormal(pv.position.rotation));
    AYAASSERT_SLOW(!Math::isNanInfDenormVector3(pv.position.translation));
    //	AYAASSERT(!Math::isNanInfDenormMatrix3(pv.position.rotation)); -- This was asserting all the time. I had to comment out - John
    AYAASSERT_SLOW(!Math::isNanInfDenormVector3(pv.velocity.linear));
    AYAASSERT_SLOW(!Math::isNanInfDenormVector3(pv.velocity.rotational));
}

void Body::setCoordinateFrame(const CoordinateFrame& worldCoord, const BodyPvSetter& bpv)
{
    setPv(PV(worldCoord, getVelocity()), bpv);
}

void Body::setVelocity(const Velocity& worldVelocity, const BodyPvSetter& bpv)
{
    if (!getParent())
    {
        pv.velocity = worldVelocity;
        advanceStateIndex();
        if (simBody)
        {
            simBody->makeDirty();
        }
    }
}

void Body::setCanThrottle(bool value, const BodyPvSetter& bpv)
{
    canThrottle = value;
}


void Body::setMass(float _mass)
{
    if (mass != _mass)
    {
        makeCofmDirty();
        mass = _mass;
    }
}

void Body::setMoment(const Matrix3& _momentInBody)
{
    if (moment != _momentInBody)
    {
        makeCofmDirty();
        moment = _momentInBody;
    }
}

Matrix3 Body::getIBodyAtPoint(const Vector3& point)
{
    return Math::getIBodyAtPoint(point, getIBody(), getMass());
}

Matrix3 Body::getIWorldAtPoint(const Vector3& point)
{
    return Math::getIWorldAtPoint(getPos(), point, getIWorld(), getMass());
}

Matrix3 Body::getBranchIWorldAtPoint(const Vector3& point)
{
    return Math::getIWorldAtPoint(getBranchCofmPos(), point, getBranchIWorld(), getBranchMass());
}

const Vector3& Body::getBranchCofmOffset()
{
    AYAASSERT(simBody);
    AYAASSERT((numChildren() > 0) || !getCofm() || getCofm()->getCofmInBody() == cofmOffset);

    return getCofm() ? getCofm()->getCofmInBody() : cofmOffset;
}

Vector3 Body::getBranchCofmPos()
{
    return cofm ? getCoordinateFrame().pointToWorldSpace(cofm->getCofmInBody()) : getPos();
}

CoordinateFrame Body::getBranchCofmCoordinateFrame()
{
    return CoordinateFrame(getCoordinateFrame().rotation, getBranchCofmPos());
}


float Body::potentialEnergy()
{
    PV tempPv = getPvUnsafe();

    float rbxGravity = Units::kmsAccelerationToRbx(this->gravity);

    return -rbxGravity * tempPv.position.translation.y * mass;
}

float Body::kineticEnergy()
{
    PV tempPv = getPvUnsafe();

    // E = 0.5 (Iw) dot w
    Vector3 Iw = getIWorld() * tempPv.velocity.rotational;
    float out = 0.5f * Iw.dot(tempPv.velocity.rotational) + 0.5f * tempPv.velocity.linear.dot(tempPv.velocity.linear) * mass;
    return out;
}

void Body::setUID(boost::uint64_t _uid)
{
    uid = _uid;
    if (simBody)
    {
        simBody->setUID(_uid);
    }
}

void Body::updateBulletCollisionObject(btCollisionObject* object)
{
    btTransform transform = getPvUnsafe().position.transformFromCFrame();

    if (btCollisionShape* shape = object->getCollisionShape())
    {
        btMatrix3x3& basis = transform.getBasis();

        if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
        {
            basis.setIdentity();
        }
        else if (shape->getShapeType() == CYLINDER_SHAPE_PROXYTYPE)
        {
            // column 0 = axis X - cylinder is rotationally invariant around this axis
            btVector3 axisX = basis.getColumn(0);

            // find a basis with axisX as the axis
            // unfortunately any basis like that has a discontinuity so sometimes the rotation will flip
            // we'll try to pick the axis so that the discontinuity is when the cylinder mostly stands upright
            // in this case it's less apparent since the cylinder is unlikely to spin fast while standing on its top
            btVector3 axisP = fabsf(axisX.getY()) < 0.9f ? btVector3(0, 1, 0) : btVector3(1, 0, 0);
            btVector3 axisZ = axisX.cross(axisP).normalized();
            btVector3 axisY = axisZ.cross(axisX).normalized();

            basis.setValue(
                axisX.getX(), axisY.getX(), axisZ.getX(), axisX.getY(), axisY.getY(), axisZ.getY(), axisX.getZ(), axisY.getZ(), axisZ.getZ());
        }
    }

    object->setWorldTransform(transform);
}

} // namespace Aya
