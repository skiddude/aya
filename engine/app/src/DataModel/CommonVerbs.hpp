

#pragma once

#include "DataModel/DataModel.hpp"
#include "DataModel/Commands.hpp"
#include "DataModel/ToolsPart.hpp"
#include "DataModel/ToolsSurface.hpp"
#include "DataModel/ToolsModel.hpp"
#include "Tool/ToolsArrow.hpp"
#include "Tool/ResizeTool.hpp"
#include "Tool/HammerTool.hpp"
#include "Tool/GrabTool.hpp"
#include "Tool/CloneTool.hpp"
#include "Tool/NullTool.hpp"
#include "Tool/GameTool.hpp"
#include "Tool/AxisMoveTool.hpp"
#include "Tool/AxisRotateTool.hpp"
#include "Tool/MoveResizeJoinTool.hpp"
#include "Tool/AdvMoveTool.hpp"
#include "Tool/AdvRotateTool.hpp"
#include "DataModel/UndoRedo.hpp"
#include "DataModel/InputObject.hpp"
#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "Utility/RunStateOwner.hpp"

#include "Utility/InsertMode.hpp"

class XmlElement;


namespace Aya
{

class Fonts;
class GuiRoot;
class GuiItem;
class ContentProvider;
class TimeState;
class Hopper;
class PlayerHopper;
class StarterPackService;
class Adorn;

// Contain a set of verbs used by Roblox
class CommonVerbs
{
public:
    CommonVerbs(DataModel* dataModel);
    ///////////////////////////////////////////////////////////////////////
    //
    // Play Mode Commands
    PlayDeleteSelectionVerb playDeleteSelectionVerb;

    // Edit Menu
    DeleteSelectionVerb deleteSelectionVerb;
    SelectAllCommand selectAllCommand;
    SelectChildrenVerb selectChildrenVerb;
    SnapSelectionVerb snapSelectionVerb;
    UnlockAllVerb unlockAllVerb;
    ColorVerb colorVerb;
    MaterialVerb materialVerb;

    // Format Menu
    AnchorVerb anchorVerb;
    TranslucentVerb translucentVerb;
    CanCollideVerb canCollideVerb;
    CanNotSelectCommand canNotSelectCommand;
    AllCanSelectCommand allCanSelectCommand;
    MoveUpPlateVerb moveUpPlateVerb;
    MoveUpBrickVerb moveUpBrickVerb;
    MoveDownSelectionVerb moveDownSelectionVerb;
    RotateSelectionVerb rotateSelectionVerb;
    TiltSelectionVerb tiltSelectionVerb;

    // Run Menu
    RunCommand runCommand;
    StopCommand stopCommand;
    ResetCommand resetCommand;

    // Test Menu
    FirstPersonCommand firstPersonCommand;
    StatsCommand statsCommand;
    RenderStatsCommand renderStatsCommand;
    EngineStatsCommand engineStatsCommand;
    NetworkStatsCommand networkStatsCommand;
    PhysicsStatsCommand physicsStatsCommand;
    SummaryStatsCommand summaryStatsCommand;
    CustomStatsCommand customStatsCommand;
    JoinCommand joinCommand;

    // Adv Build Related
    TurnOnManualJointCreation turnOnManualJointCreationVerb;

    SetDragGridToOne setDragGridToOneVerb;
    SetDragGridToOneFifth setDragGridToOneFifthVerb;
    SetDragGridToOff setDragGridToOffVerb;

    SetGridSizeToTwo setGridSizeToTwoVerb;
    SetGridSizeToFour setGridSizeToFourVerb;
    SetGridSizeToSixteen setGridSizeToSixteenVerb;

    SetManualJointToWeak setManualJointToWeakVerb;
    SetManualJointToStrong setManualJointToStrongVerb;
    SetManualJointToInfinite setManualJointToInfiniteVerb;

    // Tools
    TToolVerb<AxisRotateTool> axisRotateToolVerb;
    TToolVerb<AdvMoveTool> advMoveToolVerb;
    TToolVerb<AdvRotateTool> advRotateToolVerb;
    TToolVerb<AdvArrowTool> advArrowToolVerb;
    TToolVerb<MoveResizeJoinTool> resizeToolVerb;

    TToolVerb<FlatTool> flatToolVerb;
    TToolVerb<GlueTool> glueToolVerb;
    TToolVerb<WeldTool> weldToolVerb;
    TToolVerb<StudsTool> studsToolVerb;
    TToolVerb<InletTool> inletToolVerb;
    TToolVerb<UniversalTool> universalToolVerb;
    TToolVerb<HingeTool> hingeToolVerb;
    TToolVerb<RightMotorTool> rightMotorToolVerb;
    TToolVerb<LeftMotorTool> leftMotorToolVerb;
    TToolVerb<OscillateMotorTool> oscillateMotorToolVerb;
    TToolVerb<SmoothNoOutlinesTool> smoothNoOutlinesToolVerb;

    TToolVerb<AnchorTool> anchorToolVerb;
    TToolVerb<LockTool> lockToolVerb;

    TToolVerb<FillTool> fillToolVerb;
    TToolVerb<MaterialTool> materialToolVerb;
    TToolVerb<DropperTool> dropperToolVerb;

    // Runtime Tools
    TToolVerb<GameTool> gameToolVerb;
    TToolVerb<GrabTool> grabToolVerb;
    TToolVerb<CloneTool> cloneToolVerb;
    TToolVerb<HammerTool> hammerToolVerb;
};

} // namespace Aya
