

#pragma once

#include "Debug.hpp"

// Simple class for returning metric values - used for graphics reporting

namespace Aya
{

class AyaInterface IMetric
{
public:
    IMetric() {}
    virtual ~IMetric() {}

    virtual std::string getMetric(const std::string& metric) const = 0;
    virtual double getMetricValue(const std::string& metric) const = 0;
};
} // namespace Aya
