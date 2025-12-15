#pragma once

#include "Tree/Verb.hpp"

#include "CEvent.hpp"


namespace Aya
{

class Game;
class View;
class ViewBase;
class Document;
class VideoControl;
class WebBrowserDialog;
class Application;

// Request to leave the game.  Results in process shutdown.
class LeaveGameVerb : public Verb
{
    View& v;

public:
    LeaveGameVerb(View& v, VerbContainer* container);
    virtual ~LeaveGameVerb() {}
    virtual void doIt(IDataState* dataState);
};

// Request to toggle fullscreen
class ToggleFullscreenVerb : public Aya::Verb
{
private:
    View& view;

public:
    ToggleFullscreenVerb(View& view, VerbContainer* container);
    virtual bool isChecked() const;
    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
};

} // namespace Aya
