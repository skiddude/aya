#pragma once

#include "Base/IAdornable.hpp"
#include "Base/Part.hpp"
#include "DataModel/DataModel.hpp"


#include <boost/shared_ptr.hpp>

namespace Aya
{
class Workspace;
}

class SelectionHighlightAdornable : public Aya::IAdornable
{
    static bool getSelectionDimensions(
        Aya::Workspace* workspace, shared_ptr<Aya::Instance> instance, Aya::Part* out, float* lineThickness, bool* checkChildren);
    static void render(shared_ptr<Aya::Instance> instance, Aya::Part part, float lineThickness, Aya::Adorn* adorn);

protected:
    bool shouldRender3dAdorn() const override
    {
        return true;
    }

public:
    template<class T>
    static void renderSelection(Aya::DataModel* dm, T& collection, Aya::Adorn* adorn,
        boost::function<void(shared_ptr<Aya::Instance>, Aya::Part, float, Aya::Adorn* adorn)> renderer)
    {
        using namespace Aya;
        for (auto& instance : collection)
        {
            Part size;
            float lineSize;
            bool checkChildren;
            if (getSelectionDimensions(dm->getWorkspace(), instance, &size, &lineSize, &checkChildren))
            {
                renderer(instance, size, lineSize, adorn);
            }
            if (checkChildren && instance->getChildren2())
            {
                renderSelection(dm, *instance->getChildren2(), adorn, renderer);
            }
        }
    }
    void render3dAdorn(Aya::Adorn* adorn) override;
};