

#pragma once

#include "DataModel/JointInstance.hpp"
#include "DataModel/Workspace.hpp"
#include "Humanoid/Humanoid.hpp"
#include "World/Primitive.hpp"
#include "World/Assembly.hpp"
#include "time.hpp"

DYNAMIC_FASTINT(ActionStationDebounceTime)

namespace Aya
{


template<class Base>
class ActionStation : public Base
{
private:
    typedef Base Super;

protected:
    Time sleepTime;
    Time debounceTime;

    bool sleepTimeUp() const
    {
        return (Time::now<Time::Fast>() - sleepTime).seconds() > 3.0;
    }

    bool debounceTimeUp() const
    {
        return (Time::now<Time::Fast>() - debounceTime).seconds() > DFInt::ActionStationDebounceTime;
    }

    // Instance
    // TODO: Ultra mega super hack - setName is setting the internal Primitive::sizeMultiplier
    void setName(const std::string& value)
    {
        Super::setName(value);
        this->getPartPrimitive()->setSizeMultiplier(Primitive::SEAT_SIZE);
    }

public:
    ActionStation()
        : sleepTime(Time::now<Time::Fast>() - Time::Interval(4.0))
        , debounceTime(Time::now<Time::Fast>())
    {
        AYAASSERT(this->sleepTimeUp());
        AYAASSERT(this->getPartPrimitive());
        this->getPartPrimitive()->setSizeMultiplier(Primitive::SEAT_SIZE);
    }

    virtual ~ActionStation() {}
};


} // namespace Aya