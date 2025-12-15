

#pragma once

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

class QString;

namespace Aya
{
class DataModel;
}

class IExternalHandler
{
public:
    virtual QString className() = 0;
    virtual QString handlerId() = 0;
    virtual void setDataModel(boost::shared_ptr<Aya::DataModel> pDataModel) = 0;
    virtual bool handle() = 0;
};
