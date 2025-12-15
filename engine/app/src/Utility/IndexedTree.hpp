

#pragma once

#include "Declarations.hpp"
#include "Utility/IndexArray.hpp"
#include "Debug.hpp"

namespace Aya
{

class AyaBaseClass IndexedTree
{
private:
    IndexedTree* parent;

    int index;
    int& getIndex()
    {
        return index;
    }

    IndexArray<IndexedTree, &IndexedTree::getIndex> children;

    bool circularReference(IndexedTree* newAncestor, IndexedTree* child);

protected:
    virtual void onParentChanging() {}
    virtual void onParentChanged(IndexedTree* oldParent) {}
    virtual void onChildAdding(IndexedTree* child) {}
    virtual void onChildAdded(IndexedTree* child) {}
    virtual void onChildRemoving(IndexedTree* child) {}
    virtual void onChildRemoved(IndexedTree* child) {}
    virtual void onAncestorChanged() {}

    void setIndexedTreeParent(IndexedTree* newParent);

public:
    IndexedTree();

    virtual ~IndexedTree();

    int numChildren() const
    {
        return children.size();
    }

    template<class Type>
    Type* getTypedChild(int i)
    {
        return aya_static_cast<Type*>(children[i]);
    }

    template<class Type>
    const Type* getConstTypedChild(int i) const
    {
        return aya_static_cast<Type*>(children[i]);
    }

    template<class Type>
    Type* getTypedParent()
    {
        return aya_static_cast<Type*>(parent);
    }

    template<class Type>
    const Type* getConstTypedParent() const
    {
        return aya_static_cast<Type*>(parent);
    }

    template<class Type>
    Type* getRoot()
    {
        return (parent) ? parent->getRoot<Type>() : aya_static_cast<Type*>(this);
    }

    template<class Type>
    const Type* getRoot() const
    {
        return (parent) ? parent->getRoot<Type>() : aya_static_cast<const Type*>(this);
    }

    template<class Type>
    Type* getOneBelowRoot()
    {
        IndexedTree* above = parent;
        AYAASSERT(above != NULL);
        IndexedTree* answer = this;
        while (above->parent)
        {
            answer = above;
            above = above->parent;
        }
        return aya_static_cast<Type*>(answer);
    }



    int getDepth() const
    {
        return parent ? (parent->getDepth() + 1) : 1;
    }

    template<class Type, class Func>
    inline void visitMeAndChildren(Func func)
    {
        Type* t = aya_static_cast<Type*>(this);
        func(t);
        for (int i = 0; i < children.size(); ++i)
        {
            children[i]->visitMeAndChildren<Type, Func>(func);
        }
    }

    template<class Type, class Func>
    inline void visitConstMeAndChildren(Func func)
    {
        const Type* t = aya_static_cast<const Type*>(this);
        func(t);
        for (int i = 0; i < children.size(); ++i)
        {
            children[i]->visitConstMeAndChildren<Type, Func>(func);
        }
    }

    template<class Type, class Func>
    inline void visitDescendents(Func func)
    {
        for (int i = 0; i < children.size(); ++i)
        {
            children[i]->visitMeAndChildren<Type, Func>(func);
        }
    }

    template<class Type, class Func>
    inline void visitConstDescendents(Func func) const
    {
        for (int i = 0; i < children.size(); ++i)
        {
            children[i]->visitConstMeAndChildren<Type, Func>(func);
        }
    }
};

} // namespace Aya
