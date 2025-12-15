#include "./Marker.hpp"
#include "atomic.hpp"


namespace Aya
{
namespace Network
{

const char* const sMarker = "NetworkMarker";

Reflection::EventDesc<Marker, void()> event_Returned(&Marker::receivedSignal, "Received");

static Aya::atomic<int> ctr;

Marker::Marker()
    : returned(false)
    , id(++ctr)
{
    // Markers should never be put under another object
    this->lockParent();
}

void Marker::fireReturned()
{
    // TODO: Memoize the result and handle new connections using a Future system
    receivedSignal();
}

} // namespace Network
} // namespace Aya
