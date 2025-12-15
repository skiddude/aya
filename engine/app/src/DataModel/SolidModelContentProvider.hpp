
#pragma once

#include "DataModel/CacheableContentProvider.hpp"

namespace Aya
{

extern const char* const sSolidModelContentProvider;
class SolidModelContentProvider
    : public DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider,
          Aya::Reflection::ClassDescriptor::RUNTIME_LOCAL>
{
    typedef DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider,
        Aya::Reflection::ClassDescriptor::RUNTIME_LOCAL>
        Super;

public:
    SolidModelContentProvider();
    ~SolidModelContentProvider() {}

private:
    virtual TaskScheduler::StepResult ProcessTask(const std::string& id, shared_ptr<const std::string> data);
    virtual void updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> mesh);
};


} // namespace Aya