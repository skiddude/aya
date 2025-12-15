#pragma once

#include "DataModel/GameBasicSettings.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "signal.hpp"
#include <vector>
#include <string>
#include "Security/SecurityContext.hpp"

namespace Aya
{

class Verb;
class CommonVerbs;
class DataModel;
class GameConfigurer;

// Encapsulates the creation of a DataModel used by client apps
class Game : boost::noncopyable
{
protected:
    Game(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen = false,
        GameBasicSettings::VirtualVersion vv = GameBasicSettings::VERSION_2016);

    bool hasShutdown;

    shared_ptr<GameConfigurer> gameConfigurer;

    boost::shared_ptr<DataModel> dataModel;

public:
    static void globalInit(bool isStudio);
    static void globalExit();

    std::vector<Verb*> verbs;
    boost::shared_ptr<CommonVerbs> commonVerbs;

    boost::shared_ptr<DataModel> getDataModel() const
    {
        return dataModel;
    }

    void shutdown();

    virtual ~Game(void);

    void setupDataModel(const std::string& baseUrl);

    bool getSuppressNavKeys();

    void configurePlayer(Aya::Security::Identities identity, const std::string& params, int launchMode = -1);

private:
    void doClearVerbs();
    void clearVerbs(bool needsLock = true);
};

class SecurePlayerGame : public Game
{
public:
    SecurePlayerGame(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen = true, bool shouldShowCorescripts = true,
        GameBasicSettings::VirtualVersion vv = GameBasicSettings::VirtualVersion::VERSION_2016);
};

class UnsecuredStudioGame : public Game
{
public:
    UnsecuredStudioGame(Verb* lockVerb, const char* baseUrl, bool isNetworked = false, bool showLoadingScreen = false);
};

} // namespace Aya
