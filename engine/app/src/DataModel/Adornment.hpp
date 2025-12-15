

#pragma once

#include "DataModel/GuiBase3d.hpp"
#include "Utility/BrickColor.hpp"

namespace Aya
{
class PartInstance;
class PVInstance;

extern const char* const sPartAdornment;

// A base class for "adornment" of PartInstances (3D objects that adorn Instances)
class PartAdornment : public DescribedNonCreatable<PartAdornment, GuiBase3d, sPartAdornment>
{
public:
    PartAdornment(const char* name);

    const PartInstance* getAdornee() const
    {
        return adornee.lock().get();
    }
    PartInstance* getAdornee()
    {
        return adornee.lock().get();
    }
    void setAdornee(PartInstance* value);
    PartInstance* getAdorneeDangerous() const
    {
        return adornee.lock().get();
    }

protected:
    weak_ptr<PartInstance> adornee;
};

extern const char* const sPVAdornment;

// A base class for "adornment" of PartInstances (3D objects that adorn Instances)
class PVAdornment : public DescribedNonCreatable<PVAdornment, GuiBase3d, sPVAdornment>
{
public:
    PVAdornment(const char* name);

    const PVInstance* getAdornee() const
    {
        return adornee.lock().get();
    }
    PVInstance* getAdornee()
    {
        return adornee.lock().get();
    }
    void setAdornee(PVInstance* value);
    PVInstance* getAdorneeDangerous() const
    {
        return adornee.lock().get();
    }

protected:
    weak_ptr<PVInstance> adornee;
};

} // namespace Aya
