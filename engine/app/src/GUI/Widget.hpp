

#pragma once

#include "GUI/GUI.hpp"
#include "Tree/Verb.hpp"
#include "DataModel/GuiCore.hpp"
#include "DataModel/InputObject.hpp"

namespace Aya
{

class Widget : public GuiItem
{
private:
    typedef GuiItem Super;
    GuiResponse processMouse(const shared_ptr<InputObject>& event);
    GuiResponse processKey(const shared_ptr<InputObject>& event);

protected:
    Gui::WidgetState widgetState;

    // This should be standard for all widgets, verb widets, etc.
    /*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
    /*override*/ void onLoseFocus()
    {
        widgetState = Gui::NOTHING;
        Super::onLoseFocus();
    }

    /////////////////////////////////////////////////////////
    //
    // Override these to make new widgets

    // GUI ITEM
    /*override*/ void render2d(Adorn* adorn);

    // WIDGET
    virtual void onClick(const shared_ptr<InputObject>& event) {}
    virtual int getFontSize() const
    {
        return 10;
    }
    virtual G3D::Color4 getFontColor()
    {
        return G3D::Color3::white();
    }
    virtual bool isEnabled()
    {
        return isVisible();
    }

public:
    Widget();
};


} // namespace Aya