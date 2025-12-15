


#include "DataModel/CommonVerbs.hpp" // TODO - minimize these includes, and in the .h file
#include "DataModel/Workspace.hpp"
#include "DataModel/Camera.hpp"
#include "DataModel/UserController.hpp"
#include "DataModel/JointsService.hpp"
#include "DataModel/Sky.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/Teams.hpp"
#include "DataModel/Hopper.hpp"
#include "DataModel/Backpack.hpp"
#include "DataModel/Tool.hpp"

#include "Humanoid/Humanoid.hpp"
#include "Player.hpp"
#include "Players.hpp"

#include "Tree/Verb.hpp"
#include "World/World.hpp"
#include "Kernel/Kernel.hpp"
#include "GUI/GUI.hpp"
#include "Script/LuaInstanceBridge.hpp"
#include "Script/ScriptContext.hpp"
#include "Utility/SoundWorld.hpp"
#include "Utility/Sound.hpp"
#include "Utility/StandardOut.hpp"
#include "Log.hpp"
#include "DrawPrimitives.hpp"
#include "Utility/Http.hpp"

#include "format.hpp"

#include "Xml/Serializer.hpp"
#include "boost/cast.hpp"
#include "boost/scoped_ptr.hpp"



namespace Aya
{

CommonVerbs::CommonVerbs(DataModel* dataModel)
    : joinCommand(dataModel)
    , statsCommand(dataModel)
    , renderStatsCommand(dataModel)
    , engineStatsCommand(dataModel)
    , networkStatsCommand(dataModel)
    , physicsStatsCommand(dataModel)
    , summaryStatsCommand(dataModel)
    , customStatsCommand(dataModel)
    , runCommand(dataModel)
    , stopCommand(dataModel)
    , resetCommand(dataModel)
    , axisRotateToolVerb(dataModel)
    , resizeToolVerb(dataModel)
    , advMoveToolVerb(dataModel)
    , advRotateToolVerb(dataModel)
    , advArrowToolVerb(dataModel, false)
    , turnOnManualJointCreationVerb(dataModel)
    , setDragGridToOneVerb(dataModel)
    , setDragGridToOneFifthVerb(dataModel)
    , setDragGridToOffVerb(dataModel)
    , setGridSizeToTwoVerb(dataModel)
    , setGridSizeToFourVerb(dataModel)
    , setGridSizeToSixteenVerb(dataModel)
    , setManualJointToWeakVerb(dataModel)
    , setManualJointToStrongVerb(dataModel)
    , setManualJointToInfiniteVerb(dataModel)
    ,

    flatToolVerb(dataModel)
    , glueToolVerb(dataModel)
    , weldToolVerb(dataModel)
    , studsToolVerb(dataModel)
    , inletToolVerb(dataModel)
    , universalToolVerb(dataModel)
    , hingeToolVerb(dataModel)
    , rightMotorToolVerb(dataModel)
    , leftMotorToolVerb(dataModel)
    , oscillateMotorToolVerb(dataModel)
    , smoothNoOutlinesToolVerb(dataModel)
    ,

    anchorToolVerb(dataModel)
    , lockToolVerb(dataModel)
    , fillToolVerb(dataModel, false)
    , materialToolVerb(dataModel, false)
    , materialVerb(dataModel)
    , colorVerb(dataModel)
    , anchorVerb(dataModel)
    , dropperToolVerb(dataModel)
    ,

    firstPersonCommand(dataModel)
    , selectChildrenVerb(dataModel)
    , snapSelectionVerb(dataModel)
    , playDeleteSelectionVerb(dataModel)
    , deleteSelectionVerb(dataModel)
    , moveUpPlateVerb(dataModel)
    , moveUpBrickVerb(dataModel)
    , moveDownSelectionVerb(dataModel)
    , rotateSelectionVerb(dataModel)
    , tiltSelectionVerb(dataModel)
    , selectAllCommand(dataModel)
    , allCanSelectCommand(dataModel)
    , canNotSelectCommand(dataModel)
    , translucentVerb(dataModel)
    , canCollideVerb(dataModel)
    , unlockAllVerb(dataModel)
    , gameToolVerb(dataModel)
    , grabToolVerb(dataModel)
    , cloneToolVerb(dataModel)
    , hammerToolVerb(dataModel)
{
}


} // namespace Aya
