#include "Application.hpp"
#include "InitializationError.hpp"
#include "FunctionMarshaller.hpp"
#include "Utility/Statistics.hpp"
#include "DataModel/FastLogSettings.hpp"
#include "Utility/MicrophoneInput.hpp"
#include "Utility/CefIntegration.hpp"
#include "CoordinateFrame.hpp"
#include <boost/format.hpp>
#include <stdlib.h>
#include <QSurfaceFormat>
#include <QApplication>
#include "Window.hpp"

Aya::Application* appPtr;
QApplication* qtAppPtr;
OgreWindow* qtOgreWidget;

int main(int argc, char** argv)
{
    qputenv("QT_XCB_GL_INTEGRATION", "none");
#ifdef AYA_TEST_BUILD
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--remote-debugging-port=9222 --remote-allow-origins=*");
#endif

    QApplication qtApp(argc, argv);

    qtAppPtr = &qtApp;
    QApplication::setApplicationName(AYA_PROJECT_NAME);
    Aya::Application app;
    appPtr = &app;

    app.loadAppSettings();

    try
    {
        app.parseCommandLine(argc, argv);
    }
    catch (std::exception& e)
    {
        printf("app.parseCommandLine: %s\n", e.what());
    }

    // need client settings here before we create window
    std::string clientSettingsString;
    FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &clientSettingsString);
    // Apply client settings
    LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, clientSettingsString, &Aya::ClientAppSettings::singleton());

    OgreWindow widget(NULL);
    widget.setFocus();
    widget.show();
    widget.setWindowTitle("Aya");

    qtOgreWidget = &widget;

    try
    {
        app.Initialize(widget.getOgreWidget());
    }
    catch (const Aya::initialization_error& e)
    {
        printf("Error initailzie: %s", e.what());
        return -1;
    }

#if defined(ENABLE_CHROMIUM_FRAMES)
    if (app.getVm().count("nochromium") == 0)
    {
        Aya::CefIntegration::initialize();
    }
#endif
#if defined(ENABLE_VOICE_CHAT)
    Aya::MicrophoneInput::initialize();
#endif

    int r = -1;
    try
    {
        if (app.isGameReady())
            widget.enterWidget();
        else
            widget.hideOgreWidget();
        r = qtApp.exec();
    }
    catch (std::runtime_error const& exp)
    {
        printf("Error: %s\n", exp.what());
    }

    if (r != 0)
    {
        printf("Error: %d", r);
    }

    printf("Shutting down...\n");

    widget.shutdownRcc();
    app.shutdown();

#if defined(ENABLE_VOICE_CHAT)
    Aya::MicrophoneInput::shutdown();
#endif
#if defined(ENABLE_CHROMIUM_FRAMES)
    Aya::CefIntegration::shutdown();
#endif

    return r;
}
