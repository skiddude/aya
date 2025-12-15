

#pragma once

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "DataModel/InputObject.hpp"

namespace Aya
{

class VirtualHardwareDevice;
class DataModel;

extern const char* const sVirtualUser;
class VirtualUser
    : public DescribedCreatable<VirtualUser, Instance, sVirtualUser, Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
private:
    boost::scoped_ptr<VirtualHardwareDevice> virtualHardwareDevice;
    typedef DescribedCreatable<VirtualUser, Instance, sVirtualUser, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

    std::stringstream recording; // the script code when recording
    Aya::signals::scoped_connection recordingConnection;

    Aya::Time lastEventTime;

public:
    VirtualUser();
    // The following functions are used to automate user input (used by test scripts, for example)
    void pressKey(std::string key);
    void setKeyDown(std::string key);
    void setKeyUp(std::string key);
    void clickButton1(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void clickButton2(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void button1Down(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void button2Down(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void button1Up(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void button2Up(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void moveMouse(Aya::Vector2 position, Aya::CoordinateFrame camera);
    void captureInputDevice();

    void startRecording();
    std::string stopRecording();

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

private:
    void onInputObject(const shared_ptr<InputObject>& event);
    void sendMouseEvent(
        InputObject::UserInputType eventType, InputObject::UserInputState eventState, Aya::Vector2 position, Aya::CoordinateFrame camera);
    KeyCode convert(const std::string& key);
    void writeWait();
    void writeKey(const char* func, const shared_ptr<InputObject>& event);
    void writeMouse(const char* func, const shared_ptr<InputObject>& event);
    DataModel* getDataModel();
};

} // namespace Aya