#include "Window.hpp"
#include <QSurfaceFormat>
#include "DataModel/GameBasicSettings.hpp"
#include "FunctionMarshaller.hpp"
#include "Application.hpp"
#include "DataModel/UserInputService.hpp"
#include "DataModel/PlayerGui.hpp"
#include "Players.hpp"
#include "API.hpp"
#include "Tree/Service.hpp"
#include "Utility/AyaService.hpp"
#include "AvatarViewService.hpp"
#include "Utility/StandardOut.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/Workspace.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QVBoxLayout>
#include <codecvt>
#include <locale>
#include <boost/algorithm/string.hpp>
#include <qnamespace.h>

#ifdef __linux
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#endif

extern QApplication* qtAppPtr;
extern Aya::Application* appPtr;

OgreWidget::OgreWidget(QWidget* ogreWindow, QScreen* parent)
    : QWindow(parent)
{
    this->ogreWindow = ogreWindow;

    disableClosures = false;

    setMinimumSize(QSize(100, 100));
    setMaximumSize(QSize(3800, 2100));

    setCursor(Qt::BlankCursor);

    firstClosure = false;

    connect(dynamic_cast<OgreWindow*>(ogreWindow)->getTextInput(), &GrayChatBar::enteredText, this, &OgreWidget::onChatBarEnteredText);
}

void OgreWidget::resizeEvent(QResizeEvent* event)
{
    QSize size = event->size();
    appPtr->onResize(size.width(), size.height());
}

void OgreWidget::onChatBarEnteredText(const QString& _text)
{
    if (Aya::Network::Players* players = Aya::ServiceProvider::create<Aya::Network::Players>(appPtr->getDM().get()))
    {
        try
        {
            std::string text = _text.toStdString();
            boost::trim(text);

            players->chat(text);
        }
        catch (std::exception& e)
        {
            printf("%s\n", e.what());
        }
        // return focus to the ogre widget...
        // setFocus();
    }
}

OgreWindow::OgreWindow(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QWidget* innerWidget = new QWidget(this);
    QHBoxLayout* innerLayout = new QHBoxLayout(innerWidget);
    launcherView = new LauncherView(innerWidget);

    setWindowIcon(QIcon(":/icon.ico"));

    textInput = new GrayChatBar(this);
    myOgreWidget = new OgreWidget(this, QGuiApplication::primaryScreen());
    // myOgreWidget->setSurfaceType(QWindow::RasterSurface);
    myOgreWidget->create();
    myOgreWidgetContainer = QWidget::createWindowContainer(myOgreWidget);

    myOgreWidgetContainer->setAttribute(Qt::WA_PaintOnScreen, true);
    myOgreWidgetContainer->setAttribute(Qt::WA_NativeWindow, true);
    myOgreWidgetContainer->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    myOgreWidgetContainer->setAttribute(Qt::WA_OpaquePaintEvent, true);
    myOgreWidgetContainer->setAttribute(Qt::WA_NoSystemBackground, true);
    myOgreWidgetContainer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    myOgreWidgetContainer->setContextMenuPolicy(Qt::PreventContextMenu);
    myOgreWidgetContainer->setMouseTracking(true);
    myOgreWidgetContainer->setFocusPolicy(Qt::StrongFocus);
    myOgreWidgetContainer->setAutoFillBackground(true);
    myOgreWidgetContainer->setAcceptDrops(true);

    setMinimumSize(800, 600);
    setMaximumSize(800, 600);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    myOgreWidgetContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    textInput->setFocusPolicy(Qt::NoFocus);

    innerLayout->addWidget(launcherView, 2);
    innerLayout->addWidget(myOgreWidgetContainer, 1);
    innerLayout->setContentsMargins(0, 0, 0, 0);
    innerLayout->setSpacing(0);

    layout->addWidget(innerWidget, 1);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(textInput, 1);

    launcherView->setVisible(false);

    Aya::GameBasicSettings::singleton().virtualVersionChangedSignal.connect(boost::bind(&OgreWindow::handleVirtualVersionChange, this, _1));
    connect(launcherView->getJsHelpers(), &JsHelpers::onStartAvatarView, this, &OgreWindow::startAvatarView);
    connect(launcherView->getJsHelpers(), &JsHelpers::onStopAvatarView, this, &OgreWindow::stopAvatarView);
    connect(launcherView->getJsHelpers(), &JsHelpers::onLaunchingGame, this, &OgreWindow::launchGame);
    connect(launcherView->getJsHelpers(), &JsHelpers::onStartRcc, this, &OgreWindow::startRcc);

    QSettings settings;
    restoreGeometry(settings.value("Player/Geometry").toByteArray());
}

OgreWindow::~OgreWindow()
{
    QSettings settings;
    settings.setValue("Player/Geometry", saveGeometry());

    if (runRcc)
        shutdownRcc();
}

void OgreWindow::closeGame()
{
    printf("OgreWindow::closeGame\n");


    myOgreWidgetContainer->setVisible(false);
    appPtr->prepareToShutdown();
    appPtr->shutdown();

    if (runRcc)
        shutdownRcc();

    setMaximumSize(800, 600);

    launcherView->setVisible(true);
    launcherView->repaint();

#ifdef _WIN32
    HWND hwnd = (HWND)winId();
    LONG style = GetWindowLong(hwnd, GWL_STYLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX);

    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
#endif
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
}

void OgreWindow::closeEvent(QCloseEvent* ev)
{
    appPtr->prepareToShutdown();
    qtAppPtr->quit();
}

void OgreWindow::shutdownRcc()
{
    if (!runRcc)
        return;

    runRcc = false;
    if (rccThread.joinable())
        rccThread.join();
}

#ifdef _WIN32
std::wstring MultiByteToWideString(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

#define CAPTURE_SERVER_STDOUT

void OgreWindow::rccThreadProc(std::atomic<bool>& runAtomic, std::string password, OgreWindow* window, int port, int virtualVersion)
{
    LauncherView* lview = window->getLauncherView();
    JsHelpers* jhp = lview->getJsHelpers();

    Aya::GameBasicSettings::VirtualVersion vv = static_cast<Aya::GameBasicSettings::VirtualVersion>(virtualVersion);

    std::string vvStr = std::to_string(virtualVersion);
    std::vector<char*> argv;
#ifdef _WIN32
    // argv.push_back("Aya.Server.exe");
#else
    argv.push_back("Aya.Server");
#endif
    argv.push_back("--contentPath");
    argv.push_back("content");
    argv.push_back("--port");
    argv.push_back("-1"); // disable SOAP http port
    argv.push_back("--virtualVersion");
    argv.push_back((char*)vvStr.c_str()); // disable SOAP http port
    argv.push_back("--localServer");
    argv.push_back("--noconsole");
    if (!password.empty())
    {
        argv.push_back("--password");
        argv.push_back((char*)password.c_str());
    }

    // TODO: msHost, msName, msMotdPreview, msMotdFile
    QSettings settings;
    std::string key = settings.value("JsHelpers/masterServerAuthorization").toString().toStdString();
    std::string url = settings.value("JsHelpers/masterServerUrl").toString().toStdString();
    std::string place = jhp->getFilePath();
    std::string username = settings.value("JsHelpers/Username").toString().toStdString();
    if (url != "")
    {
        argv.push_back("--masterServerUrl");
        argv.push_back((char*)url.c_str());
        argv.push_back("--baseUrl");
        argv.push_back((char*)url.c_str());
        if (key != "")
        {
            argv.push_back("--masterServerAuthorization");
            argv.push_back((char*)key.c_str());
        }
    }
    if (place != "")
    {
        place = "\"file://" + place + "\"";
        argv.push_back("--localServerPlace");
        argv.push_back((char*)place.c_str());
    }
    argv.push_back("--localServerPort");
    std::string _port = std::to_string(port);
    argv.push_back((char*)_port.c_str());
    argv.push_back("--msHost");
    argv.push_back((char*)username.c_str());

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "dmkmm: %s", username.c_str());


#if defined(__linux__) && !defined(__APPLE__)
    FILE* xterm = fopen("/usr/bin/xterm", "r");
    if (!xterm)
    {
        throw std::runtime_error("You do not have xterm installed, or it is not readable by your current user");
    }
    fclose(xterm);

    pid_t pid;
    pid = fork();
    if (pid == -1)
    {
        throw std::runtime_error("fork() returned -1");
    }
    else if (pid == 0)
    {
        pthread_setname_np(pthread_self(), "Server Execution Thread");

        char currentPath[PATH_MAX];
        char serverPath[PATH_MAX];
        std::string cmdline;

        getcwd(currentPath, PATH_MAX);
        snprintf(serverPath, PATH_MAX, "%s/Aya.Server", currentPath);

        argv[0] = serverPath;
        for (auto p : argv)
            cmdline += std::string(p) + " ";
        printf("%s\n", cmdline.c_str());

        argv.clear();

        argv.push_back("xterm");
        argv.push_back("-e");
        argv.push_back((char*)cmdline.c_str());

        std::string xterm_cmdline = "";
        for (auto p : argv)
            xterm_cmdline += std::string(p) + " ";
        printf("%s\n", xterm_cmdline.c_str());

        snprintf(serverPath, PATH_MAX, "/usr/bin/xterm");

        argv.push_back(NULL);

        int r = execv(serverPath, argv.data());

        printf("Error (%s) %i\n", serverPath, r);
    }
    else
    {
        pthread_setname_np(pthread_self(), "Server Manager Thread");

        while (runAtomic)
        {
            sleep(1);
            std::this_thread::yield();
        }

        kill(pid, SIGKILL);
        printf("Sent sigkill to pid %i\n", pid);
    }
#elif defined(AYA_OS_WINDOWS)
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE hReadPipe, hWritePipe;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Set up the security attributes struct
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
#ifdef CAPTURE_SERVER_STDOUT
    // Create a pipe for the child process's STDOUT
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        throw std::runtime_error("CreatePipe failed");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
#endif
    wchar_t currentPath[MAX_PATH];
    wchar_t serverPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentPath);
    _snwprintf_s(serverPath, MAX_PATH, _TRUNCATE, L"%s\\Aya.Server.exe", currentPath);

    wchar_t cmdLine[MAX_PATH];

    std::string args;
    for (const auto& arg : argv)
    {
        args += arg;
        args += " ";
    }
    std::wstring widestr = MultiByteToWideString(args);

    std::wstring fullCommandLine = serverPath;
    fullCommandLine += L" ";
    fullCommandLine += MultiByteToWideString(args);

    printf("args %ls", fullCommandLine.c_str());
    int dwProcess = CreateProcessW(serverPath, // Module name
        fullCommandLine.data(),                // Command line
        NULL,                                  // Process handle not inheritable
        NULL,                                  // Thread handle not inheritable
        TRUE,                                  // Set handle inheritance to TRUE
        CREATE_UNICODE_ENVIRONMENT,            // Creation flags
        NULL,                                  // Use parent's environment block
        NULL,                                  // Use parent's starting directory
        &si,                                   // Pointer to STARTUPINFO structure
        &pi                                    // Pointer to PROCESS_INFORMATION structure
    );
    if (dwProcess)
    {
        wprintf(L"Process created successfully with PID %lu\n", pi.dwProcessId);
    }
    else
    {
        DWORD dwLastErrorCode = GetLastError();
        wprintf(L"CreateProcessW failed with error code: %d\n", dwLastErrorCode);
    }
    // Set thread name
    SetThreadDescription(GetCurrentThread(), L"Server Manager Thread");
#ifdef CAPTURE_SERVER_STDOUT
    char buffer[4096];
    DWORD bytesRead;
    DWORD bytesAvail;
#endif
    while (runAtomic)
    {
#ifdef CAPTURE_SERVER_STDOUT
        // Check if there's any output to read
        if (PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0)
        {
            if (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                // Convert to wide string for Unicode-correct output
                int wideSize = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
                std::wstring wideStr(wideSize, 0);
                MultiByteToWideChar(CP_UTF8, 0, buffer, -1, &wideStr[0], wideSize);
                wprintf(L"%s", wideStr.c_str());
            }
        }
#endif
        Sleep(1000); // Sleep for 1 second
        std::this_thread::yield();
    }

#ifdef CAPTURE_SERVER_STDOUT
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
#endif

    // Terminate the child process
    TerminateProcess(pi.hProcess, 0);
    wprintf(L"Terminated process with PID %lu\n", pi.dwProcessId);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#elif defined(__APPLE__)
    /// Show Message Box TTo User displaying different options of fruits and produce to choose from.

#endif
}

void OgreWindow::startRcc(const QString& serverConfiguration, int port, int virtualVersion, bool forceVirtualVersion)
{
    Aya::GameBasicSettings::VirtualVersion vv = static_cast<Aya::GameBasicSettings::VirtualVersion>(virtualVersion);

    myOgreWidget->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    enterWidget();

    QSettings settings;
    std::string password = settings.value("JsHelpers/serverHostPassword").toString().toStdString();

    runRcc = true;
    rccThread = std::thread(&OgreWindow::rccThreadProc, std::ref(runRcc), password, this, port, virtualVersion);

    appPtr->InitializeNewGame(vv);
    appPtr->SendScript(serverConfiguration.toStdString(), vv);

    Aya::AyaService::localServer = true;

    if (password != "")
    {
        Aya::Network::setPassword(password.c_str());
    }
}

void OgreWindow::launchGame(const QString& gameConfiguration, const int& virtualVersion)
{ // will be static casted to enum.
    Aya::GameBasicSettings::VirtualVersion vv = static_cast<Aya::GameBasicSettings::VirtualVersion>(virtualVersion);

    myOgreWidget->show();
    myOgreWidget->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    enterWidget();

    appPtr->InitializeNewGame(vv);
    appPtr->SendScript(gameConfiguration.toStdString(), vv);

    std::string password = launcherView->getJsHelpers()->getCurrentServerAuthorization();
    if (password != "")
    {
        Aya::Network::setPassword(password.c_str());
    }
}

void OgreWindow::startAvatarView()
{
    myOgreWidget->show();
    myOgreWidgetContainer->setMaximumWidth(QWIDGETSIZE_MAX);
    myOgreWidget->setCursor(Qt::OpenHandCursor);

    isShowingAvatarPage = true;

    appPtr->InitializeNewEmptyGame();

    Aya::Workspace* w = Aya::ServiceProvider::find<Aya::Workspace>(appPtr->getDM().get());
    w->setImageServerView(false);
    w->getCamera()->zoom(-2);

    Aya::AvatarViewService* avs = Aya::ServiceProvider::create<Aya::AvatarViewService>(appPtr->getDM().get());
    avs->setJsHelpers(launcherView->getJsHelpers());

    Aya::Lighting* lighting = Aya::ServiceProvider::create<Aya::Lighting>(appPtr->getDM().get());
    lighting->suppressSky(true);
    lighting->setClearAlpha(0);
}

void OgreWindow::stopAvatarView()
{
    Aya::AvatarViewService* avs = Aya::ServiceProvider::create<Aya::AvatarViewService>(appPtr->getDM().get());
    avs->setJsHelpers(NULL);

    myOgreWidgetContainer->setVisible(false);

    appPtr->prepareToShutdown();
    appPtr->shutdown();

    launcherView->setVisible(true);

    isShowingAvatarPage = false;
}

void OgreWindow::enterWidget()
{
    printf("OgreWindow::enterWidget\n");
    launcherView->setVisible(false);
}

void OgreWindow::hideOgreWidget()
{
    printf("OgreWindow::hideOgreWidget\n");

    launcherView->setVisible(true);
    myOgreWidgetContainer->setVisible(false);
}

void OgreWindow::handleVirtualVersionChange(Aya::GameBasicSettings::VirtualVersion version)
{
    if (version == Aya::GameBasicSettings::VERSION_2012)
    {
        if (Aya::StarterGuiService* gui = Aya::ServiceProvider::create<Aya::StarterGuiService>(appPtr->getDM().get()))
        {
            if (gui->getCoreGuiEnabled(Aya::StarterGuiService::COREGUI_CHATGUI) && !isShowingAvatarPage)
            {
                textInput->setVisible(true);
            }
        }
    }
    else
    {
        textInput->setVisible(false);
    }
}

bool OgreWindow::event(QEvent* event)
{
    if (event->type() == Aya::TYPE_FUNCTION_MARSHALLER)
    {
        return myOgreWidget->event(event);
    }
    return QWidget::event(event);
}

void OgreWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::PaletteChange)
    {
        QPalette palette = this->palette();
        QString mode = palette.color(QPalette::Window).lightness() < 128 ? "dark" : "light";
        launcherView->page()->runJavaScript(QString("document.documentElement.setAttribute('data-mode', '%1');").arg(mode));
    }
    QWidget::changeEvent(event);
}
// SCARY LAND OF HANDLING EVENTS

static int translateKeyModifiers(Qt::KeyboardModifiers state, const QString& text)
{
    // Convert modifier combos to a single integer.
    int result = 0;
    if ((state & Qt::ShiftModifier) && (text.size() == 0 || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

static Aya::KeyCode keyCodeTOUIKeyCode(int keyCode)
{
    /// Handle all albhabets
    if (keyCode >= 65 && keyCode <= 90)
        return Aya::KeyCode(keyCode - Qt::Key_A + 'a');

    // Handle Numbers
    if (keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9)
        return Aya::KeyCode(keyCode);

    // Handle F1 to F15
    if (keyCode >= Qt::Key_F1 && keyCode <= Qt::Key_F15)
        return Aya::KeyCode(keyCode - 0xFFFF16);


    Aya::KeyCode rbxKey = Aya::AYA_SDLK_UNKNOWN;

    // Handle Special Unordered Keys Here
    switch (keyCode)
    {
    case Qt::Key_CapsLock:
        rbxKey = Aya::AYA_SDLK_CAPSLOCK;
        break;
    case Qt::Key_Backspace:
        rbxKey = Aya::AYA_SDLK_BACKSPACE;
        break;
    case Qt::Key_Up:
        rbxKey = Aya::AYA_SDLK_UP;
        break;
    case Qt::Key_Down:
        rbxKey = Aya::AYA_SDLK_DOWN;
        break;
    case Qt::Key_Left:
        rbxKey = Aya::AYA_SDLK_LEFT;
        break;
    case Qt::Key_Right:
        rbxKey = Aya::AYA_SDLK_RIGHT;
        break;
    case Qt::Key_Insert:
        rbxKey = Aya::AYA_SDLK_INSERT;
        break;
    case Qt::Key_Delete:
        rbxKey = Aya::AYA_SDLK_DELETE;
        break;
    case Qt::Key_Home:
        rbxKey = Aya::AYA_SDLK_HOME;
        break;
    case Qt::Key_End:
        rbxKey = Aya::AYA_SDLK_END;
        break;
    case Qt::Key_PageUp:
        rbxKey = Aya::AYA_SDLK_PAGEUP;
        break;
    case Qt::Key_PageDown:
        rbxKey = Aya::AYA_SDLK_PAGEDOWN;
        break;
    case Qt::Key_Space:
        rbxKey = Aya::AYA_SDLK_SPACE;
        break;
    case Qt::Key_Control:
        rbxKey = Aya::AYA_SDLK_LCTRL;
        break;
    case Qt::Key_Alt:
        rbxKey = Aya::AYA_SDLK_LALT;
        break;
    case Qt::Key_Shift:
        rbxKey = Aya::AYA_SDLK_LSHIFT;
        break;
    case Qt::Key_Escape:
        rbxKey = Aya::AYA_SDLK_ESCAPE;
        break;
    case Qt::Key_Minus:
        rbxKey = Aya::AYA_SDLK_MINUS;
        break;
    case Qt::Key_Equal:
        rbxKey = Aya::AYA_SDLK_EQUALS;
        break;
    case Qt::Key_Tab:
        rbxKey = Aya::AYA_SDLK_TAB;
        break;
    case Qt::Key_Backtab:
    {
        rbxKey = Aya::AYA_SDLK_TAB;
        break;
    }
    case Qt::Key_BracketLeft:
        rbxKey = Aya::AYA_SDLK_LEFTBRACKET;
        break;
    case Qt::Key_BracketRight:
        rbxKey = Aya::AYA_SDLK_RIGHTBRACKET;
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        rbxKey = Aya::AYA_SDLK_RETURN;
        break;
    case Qt::Key_Semicolon:
        rbxKey = Aya::AYA_SDLK_SEMICOLON;
        break;
    case Qt::Key_QuoteLeft:
        rbxKey = Aya::AYA_SDLK_BACKQUOTE;
        break;
    case Qt::Key_Apostrophe:
        rbxKey = Aya::AYA_SDLK_QUOTE;
        break;
    case Qt::Key_QuoteDbl:
        rbxKey = Aya::AYA_SDLK_QUOTEDBL;
        break;
    case Qt::Key_Backslash:
        rbxKey = Aya::AYA_SDLK_BACKSLASH;
        break;
    case Qt::Key_Comma:
        rbxKey = Aya::AYA_SDLK_COMMA;
        break;
    case Qt::Key_Period:
        rbxKey = Aya::AYA_SDLK_PERIOD;
        break;
    case Qt::Key_Slash:
        rbxKey = Aya::AYA_SDLK_SLASH;
        break;
    case Qt::Key_multiply:
        rbxKey = Aya::AYA_SDLK_KP_MULTIPLY;
        break;
    case Qt::Key_NumLock:
        rbxKey = Aya::AYA_SDLK_NUMLOCK;
        break;
    case Qt::Key_ScrollLock:
        rbxKey = Aya::AYA_SDLK_SCROLLOCK;
        break;
    case Qt::Key_Asterisk:
        rbxKey = Aya::AYA_SDLK_ASTERISK;
        break;
    case Qt::Key_Plus:
        rbxKey = Aya::AYA_SDLK_PLUS;
        break;
    }

    return rbxKey;
}

static Aya::ModCode modifiersToUIModCode(int modifier)
{
    unsigned int modCode = 0;

    if (modifier & Qt::ShiftModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LSHIFT;
    }
    if (modifier & Qt::ControlModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LCTRL;
    }
    if (modifier & Qt::AltModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LALT;
    }

    return (Aya::ModCode)modCode;
}

void OgreWidget::sendMouseButtonEvents(QMouseEvent* me, QPoint delta, QPoint position)
{
    Aya::InputObject::UserInputType t;
    Qt::MouseButton mb = me->button();

    Aya::InputObject::UserInputType type;
    switch (mb)
    {
    case Qt::LeftButton:
        type = Aya::InputObject::TYPE_MOUSEBUTTON1;
        break;
    case Qt::RightButton:
        type = Aya::InputObject::TYPE_MOUSEBUTTON2;
        break;
    case Qt::MiddleButton:
        type = Aya::InputObject::TYPE_MOUSEBUTTON3;
        break;
    default:
        return;
    }

    OgreWindow* _ogreWindow = qobject_cast<OgreWindow*>(ogreWindow);
    Aya::InputObject::UserInputState state;
    switch (me->type())
    {
    case QEvent::MouseButtonPress:
        state = Aya::InputObject::INPUT_STATE_BEGIN;
        if (_ogreWindow->isShowingAvatarPage)
            setCursor(Qt::ClosedHandCursor);
        break;
    case QEvent::MouseButtonRelease:
        state = Aya::InputObject::INPUT_STATE_END;
        if (_ogreWindow->isShowingAvatarPage)
            setCursor(Qt::OpenHandCursor);
        break;
    default:
        return;
    }

    boost::shared_ptr<Aya::InputObject> inputObject1 = boost::make_shared<Aya::InputObject>(
        type, state, Aya::Vector3(position.x(), position.y(), 0), Aya::Vector3(delta.x(), delta.y(), 0), appPtr->getDM().get());

    appPtr->sendInputEvent(inputObject1);

    oldMouseButtons = me->buttons();
}

bool OgreWidget::event(QEvent* event)
{
    if (event->type() == Aya::TYPE_FUNCTION_MARSHALLER)
    {
        if (disableClosures)
            return false;
        setVisible(true);

        Aya::FunctionMarshaller::GetWindow()->handleAppEvent((static_cast<Aya::FunctionMarshallerEvent*>(event))->getClosure());
        firstClosure = true;

        return true;
    }
    else
    {
        if (!firstClosure)
            return false;

        OgreWindow* _ogreWindow = dynamic_cast<OgreWindow*>(ogreWindow);
        if (!_ogreWindow)
            return false;

        if (!isVisible())
            return false;

        GrayChatBar* textInput = dynamic_cast<OgreWindow*>(ogreWindow)->getTextInput();

        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);

            Aya::InputObject::UserInputState state;
            switch (ke->type())
            {
            case QEvent::KeyPress:
                state = Aya::InputObject::INPUT_STATE_BEGIN;
                break;
            case QEvent::KeyRelease:
                state = Aya::InputObject::INPUT_STATE_END;
                break;
            default:
                return false;
            }

            if (ke->key() == Qt::Key_Slash && textInput->isVisible())
            {
                textInput->setFocus();
                return false;
            }

            if (ke->key() == Qt::Key_Tab && ke->modifiers() & Qt::AltModifier)
            {
                return false;
            }

            if (ke->isAutoRepeat())
                return false;


            int keyCode = ke->key() | translateKeyModifiers(ke->modifiers(), ke->text());
            std::string text = ke->text().toStdString();
            boost::shared_ptr<Aya::InputObject> inputObject =
                boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_KEYBOARD, state, keyCodeTOUIKeyCode(ke->key()),
                    modifiersToUIModCode(ke->modifiers()), (text.length() > 0) ? text.c_str()[0] : NULL, appPtr->getDM().get());

            appPtr->sendInputEvent(inputObject);

            return false;
        }
        else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);

            QPoint delta = me->pos() - lastPosition;

            lastPosition = me->pos();
            QPoint position = me->pos();

            sendMouseButtonEvents(me, delta, position);

            return true;
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);

            QPoint savedLastPosition = lastPosition;
            QPoint delta = me->pos() - lastPosition;

            if (delta.isNull())
                return true;

            float scale = Aya::GameBasicSettings::singleton().getMouseSensitivity();
            lastPositionFrac += delta * scale;
            QPoint newPos = lastPosition + lastPositionFrac;
            delta = newPos - lastPosition;
            lastPositionFrac -= delta;

            lastPosition = newPos;
            QPoint position = me->pos();

            QSize _size = size();
            QPoint center = QPoint(_size.width() / 2, _size.height() / 2);

            if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(appPtr->getDM().get()))
            {
                switch (userInputService->getMouseWrapMode())
                {
                case Aya::UserInputService::WRAP_NONE: // WRAP_NONE - keep mouse where it is on right click drag
                {
                    boost::shared_ptr<Aya::InputObject> inputObject = boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_MOUSEMOVEMENT,
                        Aya::InputObject::INPUT_STATE_CHANGE, Aya::Vector3(savedLastPosition.x(), savedLastPosition.y(), 0),
                        Aya::Vector3(delta.x(), delta.y(), 0), appPtr->getDM().get());

                    appPtr->sendInputEvent(inputObject);
                }
                break;
                case Aya::UserInputService::WRAP_NONEANDCENTER: // WRAP_NONEANDCENTER - keep mouse at the center of the screen
                {
                    boost::shared_ptr<Aya::InputObject> inputObject =
                        boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE,
                            Aya::Vector3(center.x(), center.y(), 0), Aya::Vector3(delta.x(), delta.y(), 0), appPtr->getDM().get());

                    appPtr->sendInputEvent(inputObject);
                }
                break;
                case Aya::UserInputService::WRAP_AUTO: // WRAP_AUTO - control wrapping based on whatever... this is where we wrap for fullscreen
                                                       // chatbox (which has code still inside of UserInputUtil::wrapFullScreen)
                default:
                {
                    boost::shared_ptr<Aya::InputObject> inputObject =
                        boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE,
                            Aya::Vector3(position.x(), position.y(), 0), Aya::Vector3(delta.x(), delta.y(), 0), appPtr->getDM().get());

                    appPtr->sendInputEvent(inputObject);
                }
                break;
                }
            }


            boost::shared_ptr<Aya::InputObject> inputObject2 =
                boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_MOUSEDELTA, Aya::InputObject::INPUT_STATE_CHANGE,
                    Aya::Vector3(position.x(), position.y(), 0), Aya::Vector3(delta.x(), delta.y(), 0), appPtr->getDM().get());

            appPtr->sendInputEvent(inputObject2);

            if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(appPtr->getDM().get()))
            {
                OgreWindow* _ogreWindow = dynamic_cast<OgreWindow*>(ogreWindow);
                switch (userInputService->getMouseWrapMode())
                { // TODO: WRAP_CENTER, WRAP_HYBRID, WRAP_AUTO
                case Aya::UserInputService::WRAP_NONE:
                    QCursor::setPos(mapToGlobal(savedLastPosition));
                    lastPosition = savedLastPosition;
                    break;
                case Aya::UserInputService::WRAP_NONEANDCENTER:
                {
                    QCursor::setPos(mapToGlobal(center));
                    lastPosition = center;
                }
                break;
                case Aya::UserInputService::WRAP_AUTO:
                default:
                    QCursor::setPos(mapToGlobal(position));
                    break;
                }
            }
            return true;
        }
        else if (event->type() == QEvent::Wheel)
        {
            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            QPointF currentPos = we->position();
            QPoint angleDelta = we->angleDelta();
            boost::shared_ptr<Aya::InputObject> inputObject =
                boost::make_shared<Aya::InputObject>(Aya::InputObject::TYPE_MOUSEWHEEL, Aya::InputObject::INPUT_STATE_CHANGE,
                    Aya::Vector3(currentPos.x(), currentPos.y(), angleDelta.y()), Aya::Vector3(0, 0, 0), appPtr->getDM().get());

            appPtr->sendInputEvent(inputObject);
            return true;
        }
    }

    return false;
}