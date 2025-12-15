#pragma once

#include "Tree/Instance.hpp"


namespace Aya
{
namespace Network
{

extern const char* const sMarker;

class Marker : public Aya::DescribedNonCreatable<Marker, Instance, sMarker>
{
    bool returned; // == the marker has made the round-trip
public:
    Marker();
    const long id;
    Aya::signal<void()> receivedSignal;
    void fireReturned();
};


} // namespace Network
} // namespace Aya
