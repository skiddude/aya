

#include "DirectPhysicsReceiver.hpp"
#include "Compressor.hpp"
#include "RakNet/GetTime.hpp"
#include "NetworkSettings.hpp"

#include "Streaming.hpp"
#include "Replicator.hpp"

#include "DataModel/PartInstance.hpp"

#include "DataModel/Workspace.hpp"


#include "World/World.hpp"
#include "World/Primitive.hpp"
#include "World/Assembly.hpp"
#include "Utility/StandardOut.hpp"


#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include "NetworkProfiler.hpp"

DYNAMIC_FASTFLAG(PhysicsSenderUseOwnerTimestamp)
DYNAMIC_FASTFLAG(CleanUpInterpolationTimestamps)
SYNCHRONIZED_FASTFLAG(PhysicsPacketSendWorldStepTimestamp)

using namespace Aya;
using namespace Aya::Network;

void DirectPhysicsReceiver::receivePacket(RakNet::BitStream& inBitstream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* _stats)
{
    RakNet::Time interpolationTimestamp;
    Aya::RemoteTime remoteSendTime;
    if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
    {
        inBitstream >> interpolationTimestamp;
        remoteSendTime = ((double)interpolationTimestamp / 1000.0f);
    }
    else
    {
        remoteSendTime = ((double)timeStamp / 1000.0f);
    }

    stats = _stats;

    RakNet::Time now = RakNet::GetTime();

    // AYAASSERT(now >= timeStamp); // Defect filed: DE6810
    if (!SFFlag::getPhysicsPacketSendWorldStepTimestamp() && DFFlag::PhysicsSenderUseOwnerTimestamp && timeStamp > now)
    {
        timeStamp = now;
    }

    RakNet::Time deltaTime = SFFlag::getPhysicsPacketSendWorldStepTimestamp() ? now - interpolationTimestamp : now - timeStamp; //

    // local Aya time when the packet was sent
    Time localTime = replicator->raknetTimeToRbxTime(timeStamp);

    while (true)
    {
        shared_ptr<PartInstance> part;

        if (replicator->isStreamingEnabled())
        {
            bool done;
            inBitstream >> done;
            if (done)
                break; // packet ended

            bool cframeOnly;
            inBitstream >> cframeOnly;
            if (cframeOnly)
            {
                receiveMechanismCFrames(inBitstream, timeStamp, remoteSendTime);
                continue;
            }
            else if (!receiveRootPart(part, inBitstream))
            {
                continue; // mechanism ended
            }
        }
        else // non-streaming
        {
            if (!receiveRootPart(part, inBitstream))
            {
                break; // packet ended
            }
            BOOST_STATIC_ASSERT(sizeof(RakNet::Time) == sizeof(part->raknetTime));
        }

        // read mechanism if non-streaming OR (streaming and not cframe only)
        if (part)
        {
            if (!iAmServer && localTime < part->getLastUpdateTime())
            {
                // old packet, discard
                if (replicator->settings().printPhysicsErrors)
                {
                    Aya::StandardOut::singleton()->print(Aya::MESSAGE_INFO, "Discard old packet");
                }
                part.reset();
            }

            if (part)
            {
#if !defined(__linux) && !defined(__APPLE__)
                RakNet::Time& partTime = part->raknetTime;
                if (partTime > (SFFlag::getPhysicsPacketSendWorldStepTimestamp() ? interpolationTimestamp : timeStamp))
                {
                    if (replicator->settings().printPhysicsErrors)
                    {
                        Aya::StandardOut::singleton()->print(Aya::MESSAGE_INFO, "Physics-in old packet");
                    }
                    part.reset();
                }
                else
                {
                    if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
                    {
                        // RakNetTime will no longer hold some arbitrary send time, but instead
                        // adjusted Physics Timestamp
                        partTime = interpolationTimestamp;
                    }
                    else
                    {
                        partTime = timeStamp;
                    }
                }
#else
                RakNet::Time* partTime = &(part->raknetTime);
                if (*partTime > (SFFlag::getPhysicsPacketSendWorldStepTimestamp() ? interpolationTimestamp : timeStamp))
                {
                    if (replicator->settings().printPhysicsErrors)
                    {
                        Aya::StandardOut::singleton()->print(Aya::MESSAGE_INFO, "Physics-in old packet");
                    }
                    part.reset();
                }
                else
                {
                    if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
                    {
                        // RakNetTime will no longer hold some arbitrary send time, but instead
                        // adjusted Physics Timestamp
                        *partTime = interpolationTimestamp;
                    }
                    else
                    {
                        *partTime = timeStamp;
                    }
                }
#endif
            }
        }

        RakNet::BitSize_t characterBits = inBitstream.GetReadOffset();
        int numNodesInHistory;
        receiveMechanism(inBitstream, part.get(), tempItem, remoteSendTime, numNodesInHistory);
        if (stats && part && "Torso" == part->getName())
        {
            stats->details.characterAnim.increment();
            stats->details.characterAnimSize.sample((inBitstream.GetReadOffset() - characterBits) / 8);
        }

        if (part)
        {
            setPhysics(tempItem, remoteSendTime, deltaTime, numNodesInHistory);

            if (!iAmServer)
            {
                if (DFFlag::CleanUpInterpolationTimestamps)
                {
                    // Interpolation Delay has been re-purposed for use with relation to NetworkPing
                    // Interpolation Delay depends greatly on NetworkPing and Frequency of Updates.
                    part->setInterpolationDelay((double)deltaTime / 1000);
                }
                else
                {
                    part->setInterpolationDelay((localTime - part->getLastUpdateTime()).seconds());
                }
                part->setLastUpdateTime(localTime);
            }
        }
    }
}