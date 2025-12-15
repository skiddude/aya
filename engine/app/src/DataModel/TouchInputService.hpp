//
//  TouchInputService.h
//  App
//
//
#pragma once

#include <boost/unordered_map.hpp>

#include "Tree/Service.hpp"
#include "DataModel/InputObject.hpp"

namespace Aya
{
extern const char* const sTouchInputService;

typedef std::pair<Aya::Vector3, Aya::InputObject::UserInputState> TouchInfo;
typedef boost::unordered_map<int, std::vector<TouchInfo>> TouchBufferMap;

class TouchInputService
    : public DescribedNonCreatable<TouchInputService, Instance, sTouchInputService>
    , public Service
{
private:
    typedef DescribedNonCreatable<TouchInputService, Instance, sTouchInputService> Super;

    boost::mutex touchBufferMutex;
    int touchCount;
    TouchBufferMap touchBufferMap;
    boost::unordered_map<int, void*> countToTouchMap;
    boost::unordered_map<void*, int> touchToCountMap;
    boost::unordered_map<int, shared_ptr<InputObject>> touchIdToInputObjectMap;

    Aya::signals::scoped_connection updateInputConnection;

    void processTouchBuffer();

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldSp, ServiceProvider* newSp);

public:
    TouchInputService();

    void addTouchToBuffer(void* touch, Vector3 rbxLocation, InputObject::UserInputState newState);
};
} // namespace Aya