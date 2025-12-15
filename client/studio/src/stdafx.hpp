

#pragma once

// Windows Headers and Defines
#undef INTSAFE_E_ARITHMETIC_OVERFLOW

#ifdef _WIN32
#ifndef STRICT
#define STRICT
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit
#define _ATL_APARTMENT_THREADED

#define _ATL_NO_COM_SUPPORT // This disables support for registering COM objects

// Include Windows SDK headers
#include <sdkddkver.h>
#include <atlbase.h>
#include <atlstr.h>
#include <shobjidl.h>
#endif

#define SDL_MAIN_HANDLED
#define G3DEXPORT __declspec(dllimport)

/*****************************************************************************/
// Pre-compiled Headers
/*****************************************************************************/

// #undef USE_PCH_HEADERS

// only include stuff here that never changes

// Standard C/C++ Headers
#include <string>
#include <vector>
#include <map>
#include <list>
#include <cmath>
#include <limits>
#include <exception>

// Windows Headers
#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#endif


// TODO - these 2 give strange compiler bugs in the DS video code
// #include <QtNetwork>
// #include <QtWebKit>

// Boost Headers
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/iostreams/copy.hpp"

// Roblox Headers
#include "Base/FrameRateManager.hpp"
#include "Base/RenderSettings.hpp"
#include "API.hpp"

#include "Players.hpp"

#include "boost.hpp"
#include "CEvent.hpp"

#include "Debug.hpp"

#include "ProcessPerfCounter.hpp"

#include "signal.hpp"

#include "TaskScheduler.hpp"

#include "TaskScheduler.Job.hpp"

#include "threadsafe.hpp"

#include "Reflection/Reflection.hpp"
#include "Reflection/Type.hpp"
#include "Script/script.hpp"
#include "Utility/ContentId.hpp"

#include "Utility/FileSystem.hpp"

#include "Utility/G3DCore.hpp"

#include "Utility/Http.hpp"

#include "Utility/IMetric.hpp"

#include "Utility/Profiling.hpp"

#include "Utility/ScopedAssign.hpp"

#include "Utility/StandardOut.hpp"

#include "Utility/Statistics.hpp"

#include "DataModel/ChangeHistory.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/FastLogSettings.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/GlobalSettings.hpp"

#include "DataModel/PartInstance.hpp"

#include "DataModel/Selection.hpp"

#include "DataModel/UserController.hpp"

#include "DataModel/Workspace.hpp"

#include "Tree/Instance.hpp"

#include "Tree/Service.hpp"

#include "Xml/Serializer.hpp"
#include "Xml/XmlElement.hpp"
#include "Xml/XmlSerializer.hpp"

#include "FastLog.hpp"
#include "NetworkSettings.hpp"
#include "Reflection/ReflectionMetadata.hpp"

// Qt Headers
#include <QtCore>
#include <QtGui>
#include <QtDebug>
