

#pragma once

// Standard C/C++ Headers
#include <exception>

// Qt Headers
#include <QObject>
#include <QString>
#include <QPoint>
#include <QDateTime>

// 3rd Party Headers
#include "Vector2.hpp"

// Roblox Headers
#include "Utility/KeyCode.hpp"

#include "Utility/IMetric.hpp"

#include "Utility/InsertMode.hpp"

#include "DataModel/InputObject.hpp"

#include "Tree/Instance.hpp"


// Purpose:
// Exception that means a popup should be created with the stored message.
class CreatePopupException : public std::exception
{
public:
    CreatePopupException(const QString& message)
    {
        m_message = message;
    }

    virtual ~CreatePopupException() throw() {}

    QString m_message;
};


namespace Aya
{
class DataModel;
class ViewBase;
class FunctionMarshaller;
class Game;
class Instance;
class ContentProvider;
namespace Tasks
{
class Sequence;
}
namespace Profiling
{
class CodeProfiler;
}
} // namespace Aya

class QOgreWidget;
class UserInput;
class RenderStatsItem;
class RobloxMainWindow;

class RobloxView
    : public QObject
    , public Aya::IMetric
{
    Q_OBJECT

public:
    static boost::shared_ptr<Aya::ViewBase> createGameView(QOgreWidget* pQtWrapperWindow);

    RobloxView(QOgreWidget* pQtWrapperWidget, boost::shared_ptr<Aya::Game> game, boost::shared_ptr<Aya::ViewBase> view, bool vr = false);
    virtual ~RobloxView(void);

    void init();

    // event handling
    void handleMouse(Aya::InputObject::UserInputType event, Aya::InputObject::UserInputState state, int x, int y, Aya::ModCode modifiers);

    void handleKey(Aya::InputObject::UserInputType event, Aya::InputObject::UserInputState state, Aya::KeyCode keyCode, Aya::ModCode modifiers,
        const char keyWithModifier, bool processed = false);

    void handleScrollWheel(float delta, int x, int y);
    void handleFocus(bool focus);
    void handleMouseInside(bool inside);

    // used to set our engine's key state back to all keys up (used when ogre window loses focus)
    void resetKeyState();

    // Handle the Drop from HTML Toolbox JavaScript to Ogre Widget
    shared_ptr<const Aya::Instances> handleDropOperation(const QString&, int x, int y, bool& mouseCommandInvoked);
    void handleDropOperation(boost::shared_ptr<Aya::Instance> pInstanceToInsert, int x, int y, bool& mouseCommandInvoked);
    void cancelDropOperation(bool resetMouseCommand);

    // view data handling
    void getCursorPos(G3D::Vector2* pPos);
    void setCursorPos(G3D::Vector2* pPos, bool isLMB, bool force = false, bool updatePos = true);

    void setCursorImage(bool forceShowCursor = false);

    bool hasApplicationFocus();

    void setBounds(unsigned int width, unsigned int height);
    unsigned int getWidth()
    {
        return m_Width;
    }
    unsigned int getHeight()
    {
        return m_Height;
    }

    // user input processing
    void processWrapMouse();

    // view update
    void requestUpdateView();
    void updateView();

    // IMetric override
    double getMetricValue(const std::string& metric) const;
    std::string getMetric(const std::string& metric) const;

    void buildGui(bool buildInGameGui);
    void onEvent_playerAdded(shared_ptr<Aya::Instance> playerAdded);

    Aya::ViewBase& getViewBase()
    {
        return *m_pViewBase.get();
    }

    virtual bool eventFilter(QObject* watched, QEvent* evt);

    static bool RbxViewCyclicFlagsEnabled();

private Q_SLOTS:

    void onInsertPart();
    void onInsertObject();
    void handleContextMenu();

private:
    class WrapMouseJob;
    class RenderJob;
    class RenderRequestJob;
    class RenderJobCyclic;

    void bindToWorkspace();
    void configureStats();
    bool isValidCursorPos(G3D::Vector2* pPos, bool isLMB, bool force);

    bool setCursorImageInternal(const Aya::ContentId& contentId, shared_ptr<const std::string> imageContent);

    void setDefaultCursorImage();

    void insertInstances(
        int x, int y, const Aya::Instances& instances, bool& mouseCommandInvoked, Aya::InsertMode insertMode = Aya::INSERT_TO_3D_VIEW);

    bool rightClickContextMenuAllowed();

    void handleMouseIconEnabledEvent(bool iconEnabled);
    void handleMouseCursorChange();
    void handleContextMenu(int x, int y);

    void handleTextBoxGainFocus();
    void handleTextBoxReleaseFocus();

    void updateHoverOver();

    void doRender(const double timeStamp);

    void loadContent(boost::shared_ptr<Aya::ContentProvider> cp, Aya::ContentId contentId, Aya::Instances& instances, bool& hasError);

    Aya::Instances m_DraggedItems;

    boost::shared_ptr<Aya::ViewBase> m_pViewBase;
    boost::scoped_ptr<UserInput> m_pUserInput;

    boost::shared_ptr<Aya::DataModel> const m_pDataModel;

    Aya::FunctionMarshaller* m_pMarshaller;

    QOgreWidget* m_pQtWrapperWidget;

    boost::shared_ptr<RenderJob> m_pRenderJob;
    boost::shared_ptr<RenderRequestJob> m_pRenderRequestJob;
    boost::shared_ptr<WrapMouseJob> m_pWrapMouseJob;
    boost::shared_ptr<RenderJobCyclic> m_pRenderJobCyclic;

    boost::shared_ptr<RenderStatsItem> m_renderStatsItem; // Used for displaying RenderStats in Aya::Stats
    boost::scoped_ptr<Aya::Profiling::CodeProfiler> m_profilingSection;

    Aya::signals::scoped_connection mouseCursorEnabledConnection;
    Aya::signals::scoped_connection mouseCursorChangeConnection;
    Aya::signals::scoped_connection m_textBoxGainFocus;
    Aya::signals::scoped_connection m_textBoxReleaseFocus;
    Aya::signals::scoped_connection m_itemAddedToPlayersConnection;

    G3D::Vector2 m_previousCursorPosFraction;
    G3D::Vector2 m_previousCursorPos;
    G3D::Vector2 m_mousePressedPos;
    bool m_bIgnoreCursorChange;
    bool m_rightClickContextAllowed;
    bool m_rightClickMenuPending;

    bool m_mouseCursorHidden;

    unsigned int m_Width;
    unsigned int m_Height;

    Aya::Time m_hoverOverLastTime;
    double m_hoverOverAccum;

    QPoint m_rightClickPosition;

    QDateTime m_lastRightClickBlockingEventDate;
    QDateTime m_rightClickMouseDownDate;

    bool m_DataModelHashNeeded;
    int m_updateViewStepId;
};
