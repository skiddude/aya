


#include "World/PolyContact.hpp"
#include "Kernel/PolyConnectors.hpp"
#include "Kernel/Kernel.hpp"
#include "World/Primitive.hpp"

namespace Aya
{

PolyContact::~PolyContact()
{
    deleteConnectors(polyConnectors);

    AYAASSERT(polyConnectors.size() == 0);
}

ContactConnector* PolyContact::getConnector(int i)
{
    return polyConnectors[i];
}

void PolyContact::deleteAllConnectors()
{
    deleteConnectors(polyConnectors);
}


void PolyContact::deleteConnectors(ConnectorArray& deleteConnectors)
{
    removeAllConnectorsFromKernel();

    for (size_t i = 0; i < deleteConnectors.size(); ++i)
    {
        AYAASSERT(!deleteConnectors[i]->isInKernel());
        delete deleteConnectors[i];
    }

    deleteConnectors.fastClear();
}


void PolyContact::removeAllConnectorsFromKernel()
{
    Kernel* kernel = NULL;
    for (size_t i = 0; i < polyConnectors.size(); ++i)
    {
        if (polyConnectors[i]->isInKernel())
        {
            kernel = kernel ? kernel : getKernel(); // small optimization - getKernel walks the IPipelines
            kernel->removeConnector(polyConnectors[i]);
        }
    }
}

void PolyContact::putAllConnectorsInKernel()
{
    Kernel* kernel = NULL;
    for (size_t i = 0; i < polyConnectors.size(); ++i)
    {
        if (!polyConnectors[i]->isInKernel() && polyConnectors[i]->getContactPoint().length < -ContactConnector::overlapGoal())
        {
            kernel = kernel ? kernel : getKernel(); // small optimization - getKernel walks the IPipelines
            kernel->insertConnector(polyConnectors[i]);
        }
    }
}

bool PolyContact::stepContact()
{
    if (computeIsColliding(0.0f))
    {
        if (inKernel())
        {
            updateContactPoints();
            putAllConnectorsInKernel();
        }
        return true;
    }
    else
    {
        removeAllConnectorsFromKernel();
        return false;
    }
}

bool PolyContact::computeIsColliding(float overlapIgnored)
{
    if (Primitive::aaBoxCollide(*getPrimitive(0), *getPrimitive(1)))
    {
        updateClosestFeatures();
        if (polyConnectors.size() > 0)
        {
            float overlap = worstFeatureOverlap();
            if (overlap > overlapIgnored)
            {
                return true;
            }
        }
    }
    return false;
}

void PolyContact::updateClosestFeatures()
{
    ConnectorArray newConnectors;

    findClosestFeatures(newConnectors);

    matchClosestFeatures(newConnectors); // new Connectors is now the deal!

    AYAASSERT(newConnectors.size() <= 10);

    deleteConnectors(polyConnectors); // any remaining not matched

    polyConnectors = newConnectors; // transfer over the pointers

    AYAASSERT(polyConnectors.size() <= 10);
}

float PolyContact::worstFeatureOverlap()
{
    float worstOverlap = -FLT_MAX; // i.e. not overlapping
    AYAASSERT(polyConnectors.size() > 0);
    for (size_t i = 0; i < polyConnectors.size(); ++i)
    {                                                        // may not have any overlapping features!
        float overlap = polyConnectors[i]->computeOverlap(); // computeLength returns negative
        worstOverlap = std::max(worstOverlap, overlap);
    }
    return worstOverlap;
}

// TODO - turn optimizer back on here after fixed
void PolyContact::matchClosestFeatures(ConnectorArray& newConnectors)
{
    for (size_t i = 0; i < newConnectors.size(); ++i)
    {
        if (PolyConnector* match = matchClosestFeature(newConnectors[i]))
        {
            delete newConnectors[i];
            newConnectors.replace(i, match);
        }
    }
}

PolyConnector* PolyContact::matchClosestFeature(PolyConnector* newConnector)
{
    for (size_t i = 0; i < polyConnectors.size(); ++i)
    {
        PolyConnector* answer = polyConnectors[i];
        if (PolyConnector::match(answer, newConnector))
        {
            polyConnectors.fastRemove(i);
            return answer;
        }
    }
    return NULL;
}

void PolyContact::updateContactPoints()
{
    for (size_t i = 0; i < polyConnectors.size(); ++i)
        polyConnectors[i]->updateContactPoint();
}

} // namespace Aya