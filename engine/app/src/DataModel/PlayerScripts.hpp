
#pragma once

#include "Tree/Instance.hpp"
#include "Base/IAdornable.hpp"

#include "DataModel/InputObject.hpp"
#include "DataModel/UserInputService.hpp"
#include "Script/IScriptFilter.hpp"
#include "GUI/GuiEvent.hpp"

namespace Aya
{
class StarterPlayerScripts;

extern const char* const sPlayerScripts;
class PlayerScripts
    : public DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public IScriptFilter
{
private:
    typedef DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

public:
    PlayerScripts();

    void CopyStarterPlayerScripts(StarterPlayerScripts* scripts);

protected:
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Instance
    /*override*/ bool askSetParent(const Instance* instance) const;
    /*override*/ bool askForbidParent(const Instance* instance) const;
    /*override*/ bool askAddChild(const Instance* instance) const;
    /*override*/ bool askForbidChild(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // IScriptFilter
    /*override*/ bool scriptShouldRun(BaseScript* script);
};


extern const char* const sStarterPlayerScripts;
class StarterPlayerScripts : public DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT>
{
private:
    typedef DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT> Super;

    void InitializeDefaultScripts();
    void InitializeDefaultScriptsRunService(RunTransition transition);
    Aya::signals::scoped_connection initializeDefaultScriptsConnection;

    Aya::signal<void()> defaultScriptsLoadedSignal;

    bool defaultScriptsLoadRequested;
    bool defaultScriptsLoaded;
    bool defaultScriptsRequested;

public:
    StarterPlayerScripts();
    bool areDefaultScriptsLoaded()
    {
        return defaultScriptsLoaded;
    }
    bool checkDefaultScriptsLoaded();

    void requestDefaultScripts();
    void requestDefaultScriptsServer(shared_ptr<Instance> player);
    void defaultScriptsSend(weak_ptr<Aya::Network::Player> p);
    void defaultScriptsReceived(int confirm);

    Aya::remote_signal<void(shared_ptr<Instance>)> requestDefaultScriptsSignal;
    Aya::remote_signal<void(int)> confirmDefaultScriptsSignal;

protected:
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Instance
    /*override*/ bool askSetParent(const Instance* instance) const;
    /*override*/ bool askForbidParent(const Instance* instance) const;
    /*override*/ bool askAddChild(const Instance* instance) const;
    /*override*/ bool askForbidChild(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};

extern const char* const sStarterCharacterScripts;
class StarterCharacterScripts
    : public DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT>
{
private:
    typedef DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT>
        Super;

public:
    StarterCharacterScripts();


    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Instance
    /*override*/ bool askAddChild(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};



} // namespace Aya
