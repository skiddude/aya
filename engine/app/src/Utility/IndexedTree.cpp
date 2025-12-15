


#include "Utility/IndexedTree.hpp"


namespace Aya
{


IndexedTree::IndexedTree()
    : parent(NULL)
    , index(-1)
{
}

IndexedTree::~IndexedTree()
{
    AYAASSERT(children.size() == 0);
    AYAASSERT(!parent);
    AYAASSERT(index == -1);
}



// Make sure newAncestor and all parents do not have child as their parent
bool IndexedTree::circularReference(IndexedTree* newAncestor, IndexedTree* child)
{
    AYAASSERT(child);
    if (!newAncestor)
    {
        return false;
    }
    else if (newAncestor->parent == child)
    {
        return true;
    }
    else
    {
        return circularReference(newAncestor->parent, child);
    }
}


void IndexedTree::setIndexedTreeParent(IndexedTree* newParent)
{
    AYAASSERT(!newParent || (newParent->parent != this));
    AYAASSERT(newParent != this);
    AYAASSERT_VERY_FAST(!circularReference(newParent, this));

    if (parent != newParent)
    {
        IndexedTree* oldParent = parent;

        onParentChanging();

        if (parent)
        {
            parent->onChildRemoving(this);
            parent->children.fastRemove(this);
            parent->onChildRemoved(this); // parent Cofm will be set dirty here - mine shouldn't change!
        }

        parent = newParent;

        if (parent)
        {
            parent->onChildAdding(this);
            parent->children.fastAppend(this);
            parent->onChildAdded(this);
        }

        onParentChanged(oldParent);
    }
}


} // namespace Aya
