#pragma once


namespace Aya
{

enum Confidence
{
    C90,
    C95,
    C99,
    C99p9,
    ConfidenceMax
};

double IsValueOutlier(double value, unsigned int count, double average, double std, Confidence conf);

void GetConfidenceInterval(double average, double variance, Confidence conf, double* minV, double* maxV);

} // namespace Aya