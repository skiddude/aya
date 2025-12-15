#pragma once

#include "DataModel/Workspace.hpp"

#include "MechanismItem.hpp"
#include "ReplicatorStats.hpp"
#include "Declarations.hpp"

#include "boost.hpp"
#include <vector>

#include "RakNet/RakNetTypes.hpp"
#include "RakNet/GetTime.hpp"

#include "Base/IAdornable.hpp"

namespace RakNet
{
class BitStream;
}

namespace Aya
{
class PartInstance;
class CompactCFrame;

namespace Network
{

namespace PathBasedMovementDebug
{
struct NodeDebugInfo
{
    Color3 color;
    float size;
    bool show;
};
} // namespace PathBasedMovementDebug

class DeserializedTouchItem : public DeserializedItem
{
public:
    std::vector<TouchPair> touchPairs;

    DeserializedTouchItem() {}
    ~DeserializedTouchItem() {}

    /*implement*/ void process(Replicator& replicator);
};

class Replicator;

class AyaBaseClass PhysicsReceiver : boost::noncopyable
{
    shared_ptr<PhysicsService> physicsService;

    void readMovementHistory(
        RakNet::BitStream& bitStream, RemoteTime remoteSendTime, PartInstance* rootPart, MechanismItem& mechanismItem, int& numNodesInHistory);
    void readMechanismAttributes(RakNet::BitStream& bitStream, MechanismItem& item);

    void readAssembly(RakNet::BitStream& bitstream, PartInstance* rootPart, MechanismItem& mechanismItem, bool crossPacketCompression);
    void readPV(RakNet::BitStream& bitStream, AssemblyItem& item, bool crossPacketCompression);
    void readCoordinateFrame(RakNet::BitStream& bitStream, CoordinateFrame& cFrame);
    void readVelocity(RakNet::BitStream& bitStream, Velocity& velocity);
    void readMotorAngles(RakNet::BitStream& bitStream, AssemblyItem& item);
    void readCompactCFrame(RakNet::BitStream& bitStream, CompactCFrame& cFrame);

    bool receivePart(shared_ptr<PartInstance>& part, RakNet::BitStream& inBitstream);

protected:
    struct MovementWaypointAdorn
    {
        Vector3 position;
        Aya::Color4 color;
        float size;
        std::string text;
        MovementWaypointAdorn(const Vector3& p, const Aya::Color4& c, float s, const std::string& debugText)
        {
            position = p;
            color = c;
            size = s;
            text = debugText;
        }
    };

    struct MovementVectorAdorn
    {
        Vector3 startPos;
        Vector3 endPos;
        Aya::Color4 color;
        MovementVectorAdorn(const Vector3& start, const Vector3& end, const Aya::Color4& c)
        {
            startPos = start;
            endPos = end;
            color = c;
        }
    };

    const bool iAmServer;

    Replicator* const replicator;
    ReplicatorStats::PhysicsReceiverStats* stats;
    Time now; // or reasonably close to it.

    struct TimedCF
    {
        CoordinateFrame cf;
        float timeToEnd;
    };
    std::vector<TimedCF> nodeStack;

    boost::circular_buffer<MovementWaypointAdorn> movementWaypointList;
    boost::circular_buffer<MovementVectorAdorn> movementVectorList;
    void addWayPointAdorn(const Vector3& p, const PathBasedMovementDebug::NodeDebugInfo& info, const std::string& debugText = "");
    void addVectorAdorn(const Vector3& start, const Vector3& end, const Aya::Color4& c);

    bool okDistributedReceivePart(const shared_ptr<PartInstance>& part);
    bool receiveRootPart(shared_ptr<PartInstance>& part, RakNet::BitStream& inBitstream);

public:
    PhysicsReceiver(Replicator* replicator, bool isServer);

    virtual void start(shared_ptr<PhysicsReceiver> physicsReceiver) {}

    virtual ~PhysicsReceiver() {}

    void setTime(Time now_);
    void receiveMechanism(
        RakNet::BitStream& bitStream, PartInstance* rootPart, MechanismItem& item, RemoteTime remoteSendTime, int& numNodesInHistory);
    void receiveMechanismCFrames(RakNet::BitStream& bitStream, RakNet::Time timeStamp, const Aya::RemoteTime& remoteSendTime);
    void setPhysics(const MechanismItem& item, const RemoteTime& remoteSendTime = RemoteTime(), const RakNet::TimeMS = 0, int numNodesInHistory = 0);

    virtual void receivePacket(RakNet::BitStream& bitsream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* stats) = 0;

    void deserializeTouches(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from, std::vector<TouchPair>& touchPairs);
    bool deserializeTouch(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from, TouchPair& touchPair);
    void processTouchPair(const TouchPair& tp);
    void readTouches(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from);

    void renderPartMovementPath(Adorn* adorn);
};

} // namespace Network
} // namespace Aya