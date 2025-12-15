

#pragma once

namespace Aya
{

// assumed to be descended from Instance when used...
// RootInstance is descended from this
class Camera;
class ModelInstance;

class AyaBaseClass ICameraOwner
{
public:
    ICameraOwner() {}
    virtual ~ICameraOwner() {}

    virtual Camera* getCamera() = 0;
    virtual const Camera* getConstCamera() const = 0;

    virtual const ModelInstance* getCameraOwnerModel() const = 0;
};

} // namespace Aya
