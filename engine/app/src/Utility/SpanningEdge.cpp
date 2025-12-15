


#include "Utility/SpanningEdge.hpp"
#include "Utility/SpanningNode.hpp"

namespace Aya
{


const SpanningNode* SpanningEdge::getConstParentSpanningNode() const
{
    const SpanningNode* child = getConstChildSpanningNode();
    return otherConstNode(child);
}

const SpanningNode* SpanningEdge::getConstChildSpanningNode() const
{
    for (int i = 0; i < 2; ++i)
    {
        const SpanningNode* n = getConstNode(i);
        if (n && (n->getConstEdgeToParent() == this))
        {
            return n;
        }
    }
    AYAASSERT(0);
    return NULL;
}

SpanningNode* SpanningEdge::getChildSpanningNode()
{
    return const_cast<SpanningNode*>(getConstChildSpanningNode());
}

SpanningNode* SpanningEdge::getParentSpanningNode()
{
    return const_cast<SpanningNode*>(getConstParentSpanningNode());
}


void SpanningEdge::removeFromSpanningTree()
{
    SpanningNode* child = getChildSpanningNode();
    AYAASSERT(child);

    child->setIndexedTreeParent(NULL);
    child->setEdgeToParent(NULL);
}

void SpanningEdge::addToSpanningTree(SpanningNode* newParent)
{
    SpanningNode* child = this->otherNode(newParent);
    AYAASSERT(this->otherNode(child) == newParent);
    AYAASSERT(child);
    AYAASSERT(!child->getParent());
    AYAASSERT(!child->getEdgeToParent());

    child->setEdgeToParent(this); // do first so sort will work
    child->setIndexedTreeParent(newParent);
}

bool SpanningEdge::inSpanningTree() const
{
    for (int i = 0; i < 2; ++i)
    {
        const SpanningNode* n = getConstNode(i);
        if (n && (n->getConstEdgeToParent() == this))
        {
            return true;
        }
    }
    return false;
}


} // namespace Aya
