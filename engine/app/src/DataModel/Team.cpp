

#include "DataModel/Team.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Teams.hpp"
#include "Humanoid/Humanoid.hpp"
#include "Players.hpp"
#include "Player.hpp"


namespace Aya
{

const char* const sTeam = "Team";


static const Reflection::PropDescriptor<Team, int> prop_Score(
    "Score", category_Data, &Team::getScore, &Team::setScore, Reflection::PropertyDescriptor::Attributes::deprecated());
static const Reflection::PropDescriptor<Team, BrickColor> prop_Color("TeamColor", category_Data, &Team::getTeamColor, &Team::setTeamColor);
static const Reflection::PropDescriptor<Team, bool> prop_AutoAssignable(
    "AutoAssignable", category_Data, &Team::getAutoAssignable, &Team::setAutoAssignable);
Reflection::BoundProp<bool> Team::prop_AutoColorCharacters(
    "AutoColorCharacters", category_Data, &Team::autoColorCharacters, Reflection::PropertyDescriptor::Attributes::deprecated());

static Reflection::BoundFuncDesc<Team, shared_ptr<const Instances>()> func_GetPlayers(&Team::getPlayers, "GetPlayers", Security::None);
static Reflection::EventDesc<Team, void(shared_ptr<Instance>)> event_PlayerAdded(&Team::playerAddedSignal, "PlayerAdded", "player");
static Reflection::EventDesc<Team, void(shared_ptr<Instance>)> event_PlayerRemoved(&Team::playerRemovedSignal, "PlayerRemoved", "player");

REFLECTION_END();

Team::Team()
    : autoAssignable(true)
    , score(0)
    , autoColorCharacters(true)
{
    setName(sTeam);
    color = BrickColor::brickWhite();
}

Team::~Team() {}

shared_ptr<const Instances> Team::getPlayers()
{
    shared_ptr<Instances> teamPlayers(new Instances());

    shared_ptr<const Instances> allPlayers = ServiceProvider::find<Network::Players>(this)->getPlayers();

    Instances::const_iterator iter = allPlayers->begin();
    Instances::const_iterator end = allPlayers->end();
    for (; iter != end; ++iter)
    {
        Network::Player* player = boost::polymorphic_downcast<Network::Player*>(iter->get());
        if (player->getTeam() == this)
        {
            teamPlayers->push_back(*iter);
        }
    }

    return teamPlayers;
}

bool Team::askSetParent(const Instance* parent) const
{
    return Instance::fastDynamicCast<Teams>(parent) != NULL;
}

int Team::getScore() const
{
    return score;
}

void Team::setScore(int score)
{
    if (this->score != score)
    {
        this->score = score;
        raisePropertyChanged(prop_Score);
    }
}

BrickColor Team::getTeamColor() const
{
    return color;
}

void Team::setTeamColor(BrickColor color)
{
    if (this->color != color)
    {
        this->color = color;
        raisePropertyChanged(prop_Color);
    }
}

bool Team::getAutoAssignable() const
{
    return autoAssignable;
}
void Team::setAutoAssignable(bool autoAssign)
{
    if (this->autoAssignable != autoAssign)
    {
        this->autoAssignable = autoAssign;
        raisePropertyChanged(prop_AutoAssignable);
    }
}



} // namespace Aya
