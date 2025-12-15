#pragma once

#include "Tree/Service.hpp"
#include "Tree/Instance.hpp"
#include "Utility/Color.hpp"
#include "Utility/BrickColor.hpp"


namespace Aya
{

extern const char* const sTeam;
class Team : public DescribedCreatable<Team, Instance, sTeam>
{
protected:
    int score;
    BrickColor color;
    bool autoAssignable;
    bool autoColorCharacters;

public:
    Team();
    ~Team();

    int getScore() const;
    void setScore(int score);
    BrickColor getTeamColor() const;
    void setTeamColor(BrickColor color);
    bool getAutoAssignable() const;
    void setAutoAssignable(bool autoAssign);

    shared_ptr<const Instances> getPlayers();
    Aya::signal<void(shared_ptr<Instance>)> playerAddedSignal;
    Aya::signal<void(shared_ptr<Instance>)> playerRemovedSignal;

    static Reflection::BoundProp<bool> prop_AutoColorCharacters;

protected:
    /* override */ bool askSetParent(const Instance* parent) const;
    /* override */ bool askAddChild(const Instance* instance) const
    {
        return true;
    }
};
} // namespace Aya