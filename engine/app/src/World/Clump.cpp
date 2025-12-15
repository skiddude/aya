


#include "World/Clump.hpp"
#include "World/Primitive.hpp"
#include "World/MotorJoint.hpp"
#include "World/RigidJoint.hpp"
#include "DataModel/JointInstance.hpp"

namespace Aya
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

Clump::Clump() {}


Clump::~Clump() {}

bool Clump::isClumpRootPrimitive(const Primitive* p)
{
    return IndexedMesh::isUpperRoot(p);
}


Clump* Clump::getPrimitiveClump(Primitive* p)
{
    return aya_static_cast<Clump*>(p->getComputedUpper());
}


const Clump* Clump::getConstPrimitiveClump(const Primitive* p)
{
    return aya_static_cast<const Clump*>(p->getConstComputedUpper());
}

// TODO:  redundant code
void Clump::loadConstMotors(G3D::Array<const Joint*>& load, bool nonAnimatedOnly) const
{
    AYAASSERT(getConstTypedLower<Primitive>());
    const SpanningEdge* edge = getConstTypedLower<Primitive>()->getConstEdgeToParent();
    AYAASSERT(edge);

    const Joint* j = aya_static_cast<const Joint*>(edge);

    if (Joint::isMotorJoint(j))
    {
        const Motor* jointInstance = static_cast<const Motor*>(j->getJointOwner());
        if ((!nonAnimatedOnly) || (!jointInstance->getIsAnimatedJoint()))
        {
            load.append(j);
        }
    }
    for (int i = 0; i < numChildren(); ++i)
    {
        getConstTypedChild<Clump>(i)->loadConstMotors(load, nonAnimatedOnly);
    }
}

// TODO:  redundant code
void Clump::loadMotors(G3D::Array<Joint*>& load, bool nonAnimatedOnly)
{
    AYAASSERT(getTypedLower<Primitive>());
    SpanningEdge* edge = getTypedLower<Primitive>()->getEdgeToParent();
    AYAASSERT(edge);

    Joint* j = aya_static_cast<Joint*>(edge);

    if (Joint::isMotorJoint(j))
    {
        Motor* jointInstance = static_cast<Motor*>(j->getJointOwner());
        if ((!nonAnimatedOnly) || (!jointInstance->getIsAnimatedJoint()))
        {
            load.append(j);
        }
    }
    for (int i = 0; i < numChildren(); ++i)
    {
        getTypedChild<Clump>(i)->loadMotors(load, nonAnimatedOnly);
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*
bool PrimIterator::isParent(Primitive* parentCandidate, Primitive* child, SearchType searchType)
{
        AYAASSERT(child && parentCandidate);
        AYAASSERT(child->getTypedParent<Primitive>() == parentCandidate);

        Joint* joint = aya_static_cast<Joint*>(child->getEdgeToParent());

        AYAASSERT(Joint::isSpanningTreeJoint(joint));

        switch (searchType)
        {
                case IN_CLUMP:
                        {
                                bool inSameClump = Joint::isRigidJoint(joint);
                                AYAASSERT(inSameClump == (!child->getTypedUpper<Clump>()));
                                AYAASSERT(inSameClump == (child->getClump() == parentCandidate->getClump()));
                                return inSameClump;
                        }

                case IN_ASSEMBLY:
                        {
                                bool inSameAssembly = Joint::isKinematicJoint(joint);
                                AYAASSERT(inSameAssembly == (child->getAssembly() == parentCandidate->getAssembly()));
                                return inSameAssembly;
                        }

                default:
                        {
                                AYAASSERT(0);
                                return false;
                        }
        }
}



Primitive* PrimIterator::findParent(Primitive* p, SearchType searchType)
{
        Primitive* parent = p->getTypedParent<Primitive>();

        if (parent && isParent(parent, p, searchType)) {
                return parent;
        }
        else {
                return NULL;
        }
}


Primitive* PrimIterator::findFirstChild(Primitive* p, SearchType searchType)
{
        for (int i = 0; i < p->numChildren(); ++i) {
                Primitive* child = p->getTypedChild<Primitive>(i);
                if (isParent(p, child, searchType)) {
                        return child;
                }
        }
        return NULL;
}


Primitive* PrimIterator::findNextSibling(Primitive* parent, Primitive* sibling, SearchType searchType)
{
        Primitive* latched = NULL;

        for (int i = 0; i < parent->numChildren(); ++i) {
                Primitive* child = parent->getTypedChild<Primitive>(i);
                if (isParent(parent, child, searchType)) {
                        if (child == sibling) {
                                AYAASSERT(!latched);
                                latched = child;
                        }
                        else {
                                if (latched) {
                                        return child;
                                }
                        }
                }
        }
        return NULL;
}


Primitive* PrimIterator::findNextRelative(Primitive* parent, Primitive* p, SearchType searchType)
{
        if (!parent) {
                return NULL;
        }
        else if (Primitive* sibling = findNextSibling(parent, p, searchType)) {
                return sibling;
        }
        else {
                Primitive* parentParent = findParent(parent, searchType);
                return findNextRelative(parentParent, parent, searchType);
        }
}

Primitive* PrimIterator::getNextPrimitive(Primitive* p, SearchType searchType)
{
        if (Primitive* firstChild = findFirstChild(p, searchType)) {
                return firstChild;
        }
        else {
                Primitive* parent = findParent(p, searchType);
                return findNextRelative(parent, p, searchType);
        }
}


PrimIterator& PrimIterator::operator++()
{
        primitive = getNextPrimitive(primitive, searchType);
        return *this;
}

*/


} // namespace Aya
