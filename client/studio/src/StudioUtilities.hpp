

#pragma once

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Qt Headers
#include <QString>
#include <vector>

class EntityProperties;

namespace Aya
{
class DataModel;
class Game;
class Workspace;
class PartInstance;
class Instance;
} // namespace Aya

namespace StudioUtilities
{
// Studio only command arguments
static const char* StudioWidthArgument = "-studioWidth";     // Force Studio to a width
static const char* StudioHeightArgument = "-studioHeight";   // Force Studio to a height
static const char* EmulateTouchArgument = "-touchEmulation"; // Emulate touch controls in Studio player and Play Solo

bool isFirstTimeOpeningStudio();
void setIsFirstTimeOpeningStudio(bool value);

bool isConnectedToNetwork();

bool isAvatarMode();
void setAvatarMode(bool isAvatarMode);

bool isTestMode();
void setTestMode(bool isTestMode);


void setVideoFileName(const std::string& fileName);
std::string getVideoFileName();

bool containsEditScript(const QString& url);
bool containsJoinScript(const QString& url);
bool containsVisitScript(const QString& url);
bool containsGameServerScript(const QString& url);

std::string authenticate(std::string& domain, std::string& url, std::string& ticket);
void executeURLJoinScript(boost::shared_ptr<Aya::Game> pGame, std::string urlScript);
void executeURLScript(boost::shared_ptr<Aya::DataModel> pDataModel, std::string urlScript);

void convertPhysicalPropertiesIfNeeded(std::vector<boost::shared_ptr<Aya::Instance>> instances, Aya::Workspace* workspace);

void insertModel(boost::shared_ptr<Aya::DataModel> pDataModel, QString fileName, bool insertInto);

/**
 * Inserts a script instance under the currently selected instance
 * @param fileName   code for script instance will be read from this file and embedded into script instance
 */
void insertScript(boost::shared_ptr<Aya::DataModel> pDataModel, const QString& fileName);

/**
 * Gets debug information file (external file in which breakpoint, watch etc. related information is stored)
 * @param fileName   file for which debug information file is required
 * @param debuggerFileExt extension to be added with the debug info file name
 */
QString getDebugInfoFile(const QString& fileName, const QString& debuggerFileExt = QString());

/**
 * Checks whether we are connected to network and session is authenticated
 * @param openStartPage   if true then open start page in case user is not authenticated
 */
bool checkNetworkAndUserAuthentication(bool openStartPage = true);

int translateKeyModifiers(Qt::KeyboardModifiers state, const QString& text);
} // namespace StudioUtilities
