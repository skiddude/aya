

#pragma once

#include "DataModel/BasicPartInstance.hpp"
#include "Vector3.hpp"
#include "Tree/Service.hpp"
#include "DataModel/JointInstance.hpp"
#include "Utility/SteppedInstance.hpp"
#include "Utility/BrickColor.hpp"

#include <vector>

namespace Aya
{

extern const char* const sFlagStand;

class Flag;
class Stepped;

class FlagStand : public DescribedCreatable<FlagStand, BasicPartInstance, sFlagStand>
{
private:
    typedef DescribedCreatable<FlagStand, BasicPartInstance, sFlagStand> Super;
    Aya::signals::scoped_connection_logged standTouched;
    void onEvent_standTouched(shared_ptr<Instance> other);

    shared_ptr<Flag> watchingFlag;
    shared_ptr<Flag> clonedReplacementFlag;

    void affixFlagToRandomEmptyStand(Flag* flag);

public:
    FlagStand();
    Aya::signal<void(shared_ptr<Instance>)> flagCapturedSignal;
    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    void onStepped();

    void affixFlag(Flag* flag);
    Flag* getJoinedFlag();

    BrickColor teamColor;
    BrickColor getTeamColor() const;
    void setTeamColor(BrickColor color);
};


extern const char* const sFlagStandService;

class FlagStandService
    : public DescribedNonCreatable<FlagStandService, Instance, sFlagStandService>
    , public IStepped
    , public Service
{
private:
    typedef DescribedNonCreatable<FlagStandService, Instance, sFlagStandService> Super;
    std::list<FlagStand*> flagStands;

    ///////////////////////////////////////////////////////////////////////////
    // Instance
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
        Super::onServiceProvider(oldProvider, newProvider);
        onServiceProviderIStepped(oldProvider, newProvider);
    }

    // Istepped
    /*override*/ void onStepped(const Stepped& event);

    FlagStand* findRandomEmptyStandForFlag(Flag* f);

public:
    FlagStandService();
    ~FlagStandService();

    void affixFlagToRandomEmptyStand(Flag* flag);

    FlagStand* FindStandWithFlag(Flag* f);

    void RegisterFlagStand(FlagStand* fs);
    void UnregisterFlagStand(FlagStand* fs);
};


} // namespace Aya