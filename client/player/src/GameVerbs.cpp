#include "intrusive_ptr_target.hpp"

#include "GameVerbs.hpp"

#include "Document.hpp"
#include "FunctionMarshaller.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "Utility/FileSystem.hpp"

#include "Utility/Http.hpp"

#include "Utility/Statistics.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "View.hpp"
#include "Application.hpp"

extern OgreWindow* qtOgreWidget;

namespace Aya
{

LeaveGameVerb::LeaveGameVerb(View& view, VerbContainer* container)
    : Verb(container, "Exit")
    , v(view)
{
}

void LeaveGameVerb::doIt(IDataState* dataState)
{
    // MainLogManager::getMainLogManager()->setLeaveGame();
    FunctionMarshaller::GetWindow()->Submit(
        [this]
        {
            v.CloseWindow();

            OgreWindow* _ogreWindow = dynamic_cast<OgreWindow*>(qtOgreWidget);
            if (!_ogreWindow)
                return;

            GrayChatBar* textInput = dynamic_cast<OgreWindow*>(qtOgreWidget)->getTextInput();
            textInput->setVisible(false);
        });
}

ToggleFullscreenVerb::ToggleFullscreenVerb(View& view, VerbContainer* container)
    : Verb(container, "ToggleFullScreen")
    , view(view)
{
}

bool ToggleFullscreenVerb::isChecked() const
{
    return false;
}

bool ToggleFullscreenVerb::isEnabled() const
{
    return false;
}

void ToggleFullscreenVerb::doIt(Aya::IDataState* dataState)
{
    FASTLOG(FLog::Verbs, "Gui:ToggleFullscreen");

    // view.SetFullscreen(!view.IsFullscreen());
}

} // namespace Aya
