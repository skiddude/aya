

// Qt Headers
#include <QDesktopServices>
#include <QStandardPaths>
#include <qmath.h>
#include <QCoreApplication>
// Roblox Headers
#include "DataModel/Camera.hpp"

#include "Tool/ToolsArrow.hpp"

#include "Tool/AdvRunDragger.hpp"

#include "Draw.hpp"
#include "Script/script.hpp"

// Roblox Studio Headers
#include "AuthoringSettings.hpp"
#include "RobloxSettings.hpp"
#include "QFontBoundProp.hpp"
#include "QDirBoundProp.hpp"

FASTFLAG(LuaDebugger)
FASTFLAG(LuaDebuggerBreakOnError)

FASTFLAG(StudioUseDraggerGrid)

AYA_REGISTER_CLASS(AuthoringSettings);
AYA_REGISTER_ENUM(AuthoringSettings::PermissionLevelShown);
AYA_REGISTER_ENUM(AuthoringSettings::HoverAnimateSpeed);
AYA_REGISTER_ENUM(AuthoringSettings::OutputLayoutMode);
AYA_REGISTER_ENUM(AuthoringSettings::ListDisplayMode);
AYA_REGISTER_ENUM(AuthoringSettings::UIStyle);
AYA_REGISTER_ENUM(AuthoringSettings::TestServerAudioBehavior);

const char* const sAuthoringSettings = "Studio";
const char* const sScriptCategoryName = "Script Editor";
const char* const sScriptColorsCategoryName = "Script Editor Colors";

// AutoSave
Aya::Reflection::BoundProp<QDir> prop_AutoSaveDir("Auto-Save Path", "Auto-Save", &AuthoringSettings::autoSaveDir);
Aya::Reflection::BoundProp<bool> prop_AutoSaveEnable("Auto-Save Enabled", "Auto-Save", &AuthoringSettings::autoSaveEnabled);
Aya::Reflection::BoundProp<int> prop_AutoSaveMinutes("Auto-Save Interval (Minutes)", "Auto-Save", &AuthoringSettings::autoSaveMinutesInterval);

// Browsing
Aya::Reflection::BoundProp<bool> prop_ShowDepricatedItems("DeprecatedObjectsShown", "Browsing", &AuthoringSettings::showDeprecated);
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::PermissionLevelShown> prop_PermissionLevelShown(
    "PermissionLevelShown", "Browsing", &AuthoringSettings::getPermissionLevelShown, &AuthoringSettings::setPermissionLevelShown);

// Scripting - all script properties must be in sScriptCategoryName or ScriptTextEditor::onPropertyChange will not update properly
Aya::Reflection::BoundProp<int> prop_EditorTabWidth("Tab Width", sScriptCategoryName, &AuthoringSettings::editorTabWidth);
Aya::Reflection::BoundProp<bool> prop_EditorAutoIndent("Auto Indent", sScriptCategoryName, &AuthoringSettings::editorAutoIndent);
Aya::Reflection::BoundProp<QFont> prop_EditorFont("Font", sScriptCategoryName, &AuthoringSettings::editorFont);
Aya::Reflection::BoundProp<bool> prop_EditorTextWrap("Text Wrapping", sScriptCategoryName, &AuthoringSettings::editorTextWrap);

// Script Colors - all script properties must be in sScriptColorsCategoryName or ScriptSyntaxHighlighter::onPropertyChange will not update properly
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorTextColor("Text Color", sScriptColorsCategoryName, &AuthoringSettings::editorTextColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorBackgroundColor(
    "Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorBackgroundColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorSelectionColor(
    "Selection Color", sScriptColorsCategoryName, &AuthoringSettings::editorSelectionColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorSelectionBackgroundColor(
    "Selection Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorSelectionBackgroundColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorOperatorColor(
    "Operator Color", sScriptColorsCategoryName, &AuthoringSettings::editorOperatorColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorNumberColor("Number Color", sScriptColorsCategoryName, &AuthoringSettings::editorNumberColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorStringColor("String Color", sScriptColorsCategoryName, &AuthoringSettings::editorStringColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorCommentColor("Comment Color", sScriptColorsCategoryName, &AuthoringSettings::editorCommentColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorPreprocessorColor(
    "Preprocessor Color", sScriptColorsCategoryName, &AuthoringSettings::editorPreprocessorColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorKeywordColor("Keyword Color", sScriptColorsCategoryName, &AuthoringSettings::editorKeywordColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorErrorColor("Error Color", sScriptColorsCategoryName, &AuthoringSettings::editorErrorColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorWarningColor("Warning Color", sScriptColorsCategoryName, &AuthoringSettings::editorWarningColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_EditorFindSelectionBackgroundColor(
    "Find Selection Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorFindSelectionBackgroundColor);

// General
Aya::Reflection::BoundProp<bool> prop_AlwaysSaveScriptChangesWhileRunning(
    "Always Save Script Changes", "General", &AuthoringSettings::alwaysSaveScriptChangesWhileRunning);
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::ListDisplayMode> prop_BasicObjectsDisplayMode(
    "Basic Objects Display Mode", "General", &AuthoringSettings::getBasicObjectsDisplayMode, &AuthoringSettings::setBasicObjectsDisplayMode);
Aya::Reflection::BoundProp<int> prop_MaximumOutputLines("Maximum Output Lines", "General", &AuthoringSettings::maximumOutputLines);
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::OutputLayoutMode> prop_OutputLayoutMode(
    "Output Layout Mode", "General", &AuthoringSettings::getOutputLayoutMode, &AuthoringSettings::setOutputLayoutMode);
Aya::Reflection::BoundProp<int> prop_RenderThrottlePercentage("Render Throttle Percentage", "General", &AuthoringSettings::renderThrottlePercentage);
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::UIStyle> prop_UIStyle(
    "UI Style", "General", &AuthoringSettings::getUIStyle, &AuthoringSettings::setUIStyle);
Aya::Reflection::BoundProp<bool> prop_ClearOuputOnStart("Clear Output On Start", "General", &AuthoringSettings::clearOutputOnStart);

// Directories
Aya::Reflection::BoundProp<QDir> prop_DefaultScriptFileDir("DefaultScriptFileDir", "Directories", &AuthoringSettings::defaultScriptFileDir);
Aya::Reflection::BoundProp<QDir> prop_PluginsDir("PluginsDir", "Directories", &AuthoringSettings::pluginsDir);
Aya::Reflection::PropDescriptor<AuthoringSettings, QDir> prop_CoreScriptsDir(
    "OverrideCoreScriptsDir", "Directories", &AuthoringSettings::getCoreScriptsDir, &AuthoringSettings::setCoreScriptsDir);
Aya::Reflection::PropDescriptor<AuthoringSettings, bool> prop_useCoreScriptsDir(
    "OverrideCoreScripts", "Directories", &AuthoringSettings::getOverrideCoreScripts, &AuthoringSettings::setOverrideCoreScripts);
Aya::Reflection::BoundProp<QDir> prop_RecentSavesDir("RecentSavesDir", "Directories", &AuthoringSettings::recentSavesDir);

// Colors
Aya::Reflection::BoundProp<G3D::Color3> prop_SelectColor("Select Color", "Colors", &AuthoringSettings::selectColor);
Aya::Reflection::BoundProp<G3D::Color3> prop_HoverOverColor("Hover Over Color", "Colors", &AuthoringSettings::hoverOverColor);
Aya::Reflection::PropDescriptor<AuthoringSettings, G3D::Color3> prop_PrimaryPartSelectColor(
    "Select/Hover Color", "Primary Part", &AuthoringSettings::getPrimaryPartSelectColor, &AuthoringSettings::setPrimaryPartSelectColor);
Aya::Reflection::PropDescriptor<AuthoringSettings, float> prop_PrimaryPartLineThickness(
    "Line Thickness", "Primary Part", &AuthoringSettings::getPrimaryPartLineThickness, &AuthoringSettings::setPrimaryPartLineThickness);

// Undo
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, Aya::ChangeHistoryService::RuntimeUndoBehavior> prop_RuntimeUndoBehavior(
    "RuntimeUndoBehavior", "Undo", &AuthoringSettings::getRuntimeUndoBehavior, &AuthoringSettings::setRuntimeUndoBehavior);

// Camera
Aya::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraKeyMoveSpeed(
    "Camera Speed", "Camera", &AuthoringSettings::getCameraKeyMoveSpeed, &AuthoringSettings::setCameraKeyMoveSpeed);
Aya::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraShiftMoveSpeed(
    "Camera Shift Speed", "Camera", &AuthoringSettings::getCameraShiftMoveSpeed, &AuthoringSettings::setCameraShiftMoveSpeed);
Aya::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraMouseWheelMoveSpeed(
    "Camera Mouse Wheel Speed", "Camera", &AuthoringSettings::getCameraMouseWheelMoveSpeed, &AuthoringSettings::setCameraMouseWheelMoveSpeed);

// Tools
Aya::Reflection::PropDescriptor<AuthoringSettings, bool> prop_showDraggerGrid(
    "Show Dragger Grid", "Tools", &AuthoringSettings::getShowDraggerGrid, &AuthoringSettings::setShowDraggerGrid);
Aya::Reflection::PropDescriptor<AuthoringSettings, bool> prop_showHoverOver(
    "Show Hover Over", "Tools", &AuthoringSettings::getShowHoverOver, &AuthoringSettings::setShowHoverOver);
Aya::Reflection::BoundProp<bool> prop_AnimateHoverOver("Animate Hover Over", "Tools", &AuthoringSettings::animateHoverOver);
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::HoverAnimateSpeed> prop_HoverAnimateSpeed(
    "Hover Animate Speed", "Tools", &AuthoringSettings::getHoverAnimateSpeed, &AuthoringSettings::setHoverAnimateSpeed);

// Lua Debugger
Aya::Reflection::PropDescriptor<AuthoringSettings, bool> prop_LuaDebuggerEnabled(
    "LuaDebuggerEnabled", "Lua Debugger", &AuthoringSettings::getLuaDebuggerEnableState, &AuthoringSettings::setLuaDebuggerEnableState);

// Advanced
Aya::Reflection::BoundProp<bool> prop_diagnosticsBarEnabled("Show Diagnostics Bar", "Advanced", &AuthoringSettings::diagnosticsBarEnabled);
Aya::Reflection::BoundProp<bool> prop_intellisenseEnabled("Enable Intellisense", "Advanced", &AuthoringSettings::intellisenseEnabled);
Aya::Reflection::PropDescriptor<AuthoringSettings, bool> prop_dragMultiPartsAsSinglePartEnabled("Drag Multiple Parts As Single Part", "Advanced",
    &AuthoringSettings::getDragMutliplePartsAsSinglePart, &AuthoringSettings::setDragMutliplePartsAsSinglePart);

// Audio
Aya::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::TestServerAudioBehavior> prop_testServerAudioBehavior(
    "Server Audio Behavior", "Audio", &AuthoringSettings::getTestServerAudioBehavior, &AuthoringSettings::setTestServerAudioBehavior);
Aya::Reflection::BoundProp<bool> prop_onlyPlayAudioInFocus(
    "Only Play Audio from Window in Focus", "Audio", &AuthoringSettings::onlyPlayFocusWindowAudio);

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::PermissionLevelShown>::EnumDesc()
    : EnumDescriptor("PermissionLevelShown")
{
    addPair(AuthoringSettings::Game, "Game");
    addPair(AuthoringSettings::RobloxGame, "RobloxGame");
    addPair(AuthoringSettings::RobloxScript, "RobloxScript");
    addPair(AuthoringSettings::Studio, "Studio");
    addPair(AuthoringSettings::Roblox, "Roblox");
}
} // namespace Reflection
} // namespace Aya

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::HoverAnimateSpeed>::EnumDesc()
    : EnumDescriptor("HoverAnimateSpeed")
{
    addPair(AuthoringSettings::VerySlow, "VerySlow");
    addPair(AuthoringSettings::Slow, "Slow");
    addPair(AuthoringSettings::Medium, "Medium");
    addPair(AuthoringSettings::Fast, "Fast");
    addPair(AuthoringSettings::VeryFast, "VeryFast");
}
} // namespace Reflection
} // namespace Aya

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::ListDisplayMode>::EnumDesc()
    : EnumDescriptor("ListDisplayMode")
{
    addPair(AuthoringSettings::Horizontal, "Horizontal");
    addPair(AuthoringSettings::Vertical, "Vertical");
}
} // namespace Reflection
} // namespace Aya

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::OutputLayoutMode>::EnumDesc()
    : EnumDescriptor("OutputLayoutMode")
{
    addPair(AuthoringSettings::OutputLayoutHorizontal, "Horizontal");
    addPair(AuthoringSettings::OutputLayoutVertical, "Vertical");
}
} // namespace Reflection
} // namespace Aya

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::UIStyle>::EnumDesc()
    : EnumDescriptor("UIStyle")
{
    addPair(AuthoringSettings::Ribbon, "RibbonBar");
    addPair(AuthoringSettings::Menu, "SystemMenu");
}
} // namespace Reflection
} // namespace Aya

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<AuthoringSettings::TestServerAudioBehavior>::EnumDesc()
    : EnumDescriptor("ServerAudioBehavior")
{
    addPair(AuthoringSettings::Enabled, "Enabled");
    addPair(AuthoringSettings::Muted, "Muted");
    addPair(AuthoringSettings::OnlineGame, "OnlineGame");
}
} // namespace Reflection
} // namespace Aya

AuthoringSettings::AuthoringSettings()
    : showDeprecated(false)
    , permissionLevelShown(Game)
    , defaultScriptFileDir(QCoreApplication::applicationDirPath() + "/data/studio/scripts")
    , pluginsDir(QCoreApplication::applicationDirPath() + "/data/studio/plugins")
    , modelPluginsDir(QCoreApplication::applicationDirPath() + "/data/studio/plugins")
    , coreScriptsDir(QCoreApplication::applicationDirPath() + "/data/core-script-overrides")
    , recentSavesDir(QCoreApplication::applicationDirPath() + "/data/studio/recent-saves")
    , overrideCoreScripts(false)
    , basicObjectsDisplayMode(Vertical)
    , maximumOutputLines(5000)
    , outputLayoutMode(OutputLayoutVertical)
    , renderThrottlePercentage(75)
    , alwaysSaveScriptChangesWhileRunning(false)
    , clearOutputOnStart(true)
// Script Editor
#ifdef Q_WS_WIN32
    , editorFont("Courier New", 10)
#else
    , editorFont("Courier New", 14)
#endif
    , editorTabWidth(4)
    , editorAutoIndent(true)
    , editorTextWrap(false)
    , editorTextColor(G3D::Color3::black())
    , editorBackgroundColor(G3D::Color3::white())
    , editorSelectionColor(G3D::Color3::white())
    , editorSelectionBackgroundColor(G3D::Color3uint8(0x6E, 0xA1, 0xF1))
    , editorOperatorColor(G3D::Color3uint8(0x7F, 0x7F, 0x00))
    , editorNumberColor(G3D::Color3uint8(0x00, 0x7F, 0x7F))
    , editorStringColor(G3D::Color3uint8(0x7F, 0x00, 0x7F))
    , editorCommentColor(G3D::Color3uint8(0x00, 0x7F, 0x00))
    , editorPreprocessorColor(G3D::Color3uint8(0x7F, 0x00, 0x00))
    , editorKeywordColor(G3D::Color3uint8(0x00, 0x00, 0x7F))
    , editorErrorColor(G3D::Color3::red())
    , editorWarningColor(G3D::Color3::blue())
    , editorFindSelectionBackgroundColor(G3D::Color3uint8(0xF6, 0xB9, 0x3F))

    // HoverOver
    , hoverOverColor(Aya::Draw::hoverOverColor().rgb())
    , selectColor(Aya::Draw::selectColor().rgb())
    , animateHoverOver(true)
    , hoverAnimateSpeed(Slow)

    // AutoSave
    , autoSaveEnabled(true)
    , autoSaveMinutesInterval(5)
    , autoSaveDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/AutoSaves")
    // Advanced
    , diagnosticsBarEnabled(false)
    , intellisenseEnabled(true)
    // Lua debugger
    , luaDebuggerEnabled(true)
    // Ribbon
    , uiStyle(AuthoringSettings::Ribbon)
    // Device Deployment
    , deploymentPairingCode(0)
    // Audio Settings
    , onlyPlayFocusWindowAudio(true)
    , testServerAudioBehavior(AuthoringSettings::OnlineGame)
    , dragMultiPartsAsSinglePart(false)
{
}

/**
 * Configure settings after the fast flags have loaded to do any special adjustments/handling.
 *  Must be called after Aya::GlobalAdvancedSettings::singleton()->loadState() has been called.
 */
void AuthoringSettings::configureBasedOnFastFlags()
{
    FFlag::LuaDebugger = luaDebuggerEnabled;

    // make sure we do not enable break on error if there's no debugger
    if (!FFlag::LuaDebugger)
        FFlag::LuaDebuggerBreakOnError = false;

    if (!FFlag::StudioUseDraggerGrid)
        prop_showDraggerGrid.setEditable(false);

    setDragMutliplePartsAsSinglePart(dragMultiPartsAsSinglePart);
}

void AuthoringSettings::onPropertyChanged(const Aya::Reflection::PropertyDescriptor& pDescriptor)
{
    Instance::onPropertyChanged(pDescriptor);
}

void AuthoringSettings::setRuntimeUndoBehavior(Aya::ChangeHistoryService::RuntimeUndoBehavior value)
{
    Aya::ChangeHistoryService::runtimeUndoBehavior = value;
}

Aya::Security::Permissions AuthoringSettings::getPermissionShown() const
{
    switch (permissionLevelShown)
    {
    default:
        AYAASSERT(false);
    case Game:
        return Aya::Security::None;
    case RobloxGame:
        return Aya::Security::RobloxPlace;
    case RobloxScript:
        return Aya::Security::RobloxScript;
    case Studio:
        return Aya::Security::LocalUser;
    case Roblox:
        return Aya::Security::Roblox;
    }
}

void AuthoringSettings::setPermissionLevelShown(PermissionLevelShown value)
{
    if (value == permissionLevelShown)
        return;
    permissionLevelShown = value;
    raiseChanged(prop_PermissionLevelShown);
}

float AuthoringSettings::getCameraKeyMoveSpeed() const
{
    return Aya::Camera::CameraKeyMoveFactor;
}

void AuthoringSettings::setCameraKeyMoveSpeed(float value)
{
    Aya::Camera::CameraKeyMoveFactor = value;
    raiseChanged(prop_CameraKeyMoveSpeed);
}

float AuthoringSettings::getCameraShiftMoveSpeed() const
{
    return Aya::Camera::CameraShiftKeyMoveFactor;
}

void AuthoringSettings::setCameraShiftMoveSpeed(float value)
{
    Aya::Camera::CameraShiftKeyMoveFactor = value;
    raiseChanged(prop_CameraShiftMoveSpeed);
}

float AuthoringSettings::getCameraMouseWheelMoveSpeed() const
{
    return Aya::Camera::CameraMouseWheelMoveFactor;
}

void AuthoringSettings::setCameraMouseWheelMoveSpeed(float value)
{
    Aya::Camera::CameraMouseWheelMoveFactor = value;
    raiseChanged(prop_CameraMouseWheelMoveSpeed);
}

bool AuthoringSettings::getShowDraggerGrid() const
{
    return Aya::ArrowToolBase::showDraggerGrid;
}

void AuthoringSettings::setShowDraggerGrid(bool value)
{
    Aya::ArrowToolBase::showDraggerGrid = value;
    raiseChanged(prop_showDraggerGrid);
}

bool AuthoringSettings::getShowHoverOver() const
{
    return Aya::Draw::isHoverOver();
}

void AuthoringSettings::setShowHoverOver(bool value)
{
    Aya::Draw::showHoverOver(value);
    raiseChanged(prop_showHoverOver);
}

void AuthoringSettings::setHoverAnimateSpeed(HoverAnimateSpeed speed)
{
    hoverAnimateSpeed = speed;
    raiseChanged(prop_HoverAnimateSpeed);
}

void AuthoringSettings::setBasicObjectsDisplayMode(ListDisplayMode mode)
{
    basicObjectsDisplayMode = mode;
    raiseChanged(prop_BasicObjectsDisplayMode);
}

void AuthoringSettings::setEditorFont(const QFont& font)
{
    editorFont = font;
    raiseChanged(prop_EditorFont);
}

void AuthoringSettings::setOutputLayoutMode(OutputLayoutMode mode)
{
    outputLayoutMode = mode;
    raiseChanged(prop_OutputLayoutMode);
}

bool AuthoringSettings::getLuaDebuggerEnableState() const
{
    return luaDebuggerEnabled;
}

void AuthoringSettings::setLuaDebuggerEnableState(bool state)
{
    luaDebuggerEnabled = state;
}

void AuthoringSettings::setUIStyle(UIStyle value)
{
    if (value == uiStyle)
        return;
    uiStyle = value;
    raiseChanged(prop_UIStyle);
}

AuthoringSettings::UIStyle AuthoringSettings::getUIStyle() const
{
    return uiStyle;
}

void AuthoringSettings::setOverrideCoreScripts(bool value)
{
    if (value != overrideCoreScripts)
    {
        overrideCoreScripts = value;
        Aya::BaseScript::adminScriptsPath = (overrideCoreScripts) ? coreScriptsDir.absolutePath().toStdString() : "";
        raiseChanged(prop_useCoreScriptsDir);
    }
}

void AuthoringSettings::setCoreScriptsDir(QDir value)
{
    if (value != coreScriptsDir)
    {
        coreScriptsDir = value;
        if (overrideCoreScripts)
        {
            Aya::BaseScript::adminScriptsPath = coreScriptsDir.absolutePath().toStdString();
        }
        raiseChanged(prop_CoreScriptsDir);
    }
}

void AuthoringSettings::setTestServerAudioBehavior(TestServerAudioBehavior value)
{
    if (value == testServerAudioBehavior)
        return;
    testServerAudioBehavior = value;
    raiseChanged(prop_testServerAudioBehavior);
}

G3D::Color3 AuthoringSettings::getPrimaryPartSelectColor() const
{
    return Aya::ModelInstance::primaryPartSelectColor().rgb();
}

void AuthoringSettings::setPrimaryPartSelectColor(G3D::Color3 color)
{
    // as of now we are using same color for selection and hover over
    Aya::ModelInstance::setPrimaryPartSelectColor(color);
    Aya::ModelInstance::setPrimaryPartHoverOverColor(color);
}

G3D::Color3 AuthoringSettings::getPrimaryPartHoverOverColor() const
{
    return Aya::ModelInstance::primaryPartHoverOverColor().rgb();
}

void AuthoringSettings::setPrimaryPartHoverOverColor(G3D::Color3 color)
{
    Aya::ModelInstance::setPrimaryPartHoverOverColor(color);
}

float AuthoringSettings::getPrimaryPartLineThickness() const
{
    return Aya::ModelInstance::primaryPartLineThickness();
}

void AuthoringSettings::setPrimaryPartLineThickness(float thickness)
{
    if (thickness > 0.0f && thickness < 0.05f)
    {
        Aya::ModelInstance::setPrimaryPartLineThickness(thickness);
        return;
    }

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Invalid line thickness. Input value must be less than 0.05");
}

bool AuthoringSettings::getDragMutliplePartsAsSinglePart() const
{
    return dragMultiPartsAsSinglePart;
}

void AuthoringSettings::setDragMutliplePartsAsSinglePart(bool state)
{
    dragMultiPartsAsSinglePart = state;
    Aya::AdvRunDragger::dragMultiPartsAsSinglePart = state;
}