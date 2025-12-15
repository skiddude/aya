#pragma once
#include "intrusive_ptr_target.hpp"

#include <QWindow>
#include <QWidget>
#include <QEvent>
#include <QPoint>

#include "GrayChatBar.hpp"
#include "LauncherView.hpp"
#include "DataModel/GameBasicSettings.hpp"

// This will not work unless if it is made by an OgreWindow
class OgreWidget : public QWindow
{
    Q_OBJECT

    QPoint lastPosition;
    QPoint lastPositionFrac;
    bool firstClosure;
    bool disableClosures;
    Qt::MouseButtons oldMouseButtons;
    QWidget* ogreWindow;

    bool textInputCanGrabFocus;

    void sendMouseButtonEvents(QMouseEvent* event, QPoint delta, QPoint position);
    void onChatBarEnteredText(const QString& text);

protected:
    void resizeEvent(QResizeEvent* event) override;

public:
    explicit OgreWidget(QWidget* ogreWindow, QScreen* parent);

    bool event(QEvent* event);
    QWidget* getOgreWindow()
    {
        return ogreWindow;
    }


    bool setClosuresDisabled(bool b)
    {
        disableClosures = b;
    };
    QPaintEngine* paintEngine() const
    {
        return nullptr; // Disable QPainter
    }
};

class OgreWindow : public QWidget
{
    Q_OBJECT

    QWidget* myOgreWidgetContainer;
    OgreWidget* myOgreWidget;
    GrayChatBar* textInput;
    LauncherView* launcherView;

    std::thread rccThread;
    std::atomic<bool> runRcc;


    void launchGame(const QString& gameConfiguration, const int& virtualVersion);
    void stopAvatarView();
    void startAvatarView();
    void startRcc(const QString& serverConfiguration, int port, int virtualVersion, bool forceVirtualVersion);
    void wearAsset(int64_t assetId, const QString& category);

    static void rccThreadProc(std::atomic<bool>& runAtomic, std::string password, OgreWindow* window, int port, int virtualVersion);

    virtual void closeEvent(QCloseEvent* ev);

public:
    explicit OgreWindow(QWidget* parent);
    ~OgreWindow();
    void shutdownRcc();
    void closeGame(); // called by view gameverbs

    OgreWidget* getOgreWidget()
    {
        return myOgreWidget;
    };

    QWidget* getOgreWidgetContainer()
    {
        return myOgreWidgetContainer;
    };
    GrayChatBar* getTextInput()
    {
        return textInput;
    }
    LauncherView* getLauncherView()
    {
        return launcherView;
    }

    bool event(QEvent* event);
    void changeEvent(QEvent* event);
    void enterWidget();
    void hideOgreWidget();
    bool isShowingAvatarPage = false;

    void handleVirtualVersionChange(Aya::GameBasicSettings::VirtualVersion version);
};
