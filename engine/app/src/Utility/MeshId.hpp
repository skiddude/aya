

#pragma once

#include "Tree/Service.hpp"
#include "Utility/RunStateOwner.hpp"
#include "Reflection/Event.hpp"


namespace Aya
{

class PartInstance;

class MeshId : public ContentId
{
public:
    MeshId(const ContentId& id)
        : ContentId(id)
    {
    }
    MeshId(const char* id)
        : ContentId(id)
    {
    }
    MeshId(const std::string& id)
        : ContentId(id)
    {
    }
    MeshId() {}
};
} // namespace Aya
