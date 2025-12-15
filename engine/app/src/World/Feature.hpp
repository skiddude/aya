

#pragma once

#include "Utility/G3DCore.hpp"
#include "Debug.hpp"

namespace Aya
{

class Primitive;

namespace GEO
{

class AyaBaseClass Feature
{
private:
    Primitive* primitive;
    int index;

public:
    typedef enum
    {
        VERTEX,
        EDGE,
        FACE
    } FeatureType;

    Feature(Primitive* primitive, int index)
        : primitive(primitive)
        , index(index)
    {
    }

    virtual FeatureType getFeatureType() const = 0;
};

class Vertex : public Feature
{
public:
    Vertex(Primitive* primitive, int index)
        : Feature(primitive, index)
    {
    }

    /*override*/ FeatureType getFeatureType() const
    {
        return VERTEX;
    }
};

class Edge : public Feature
{
public:
    Edge(Primitive* primitive, int index)
        : Feature(primitive, index)
    {
    }

    /*override*/ FeatureType getFeatureType() const
    {
        return EDGE;
    }
};

class Face : public Feature
{
public:
    Face(Primitive* primitive, int index)
        : Feature(primitive, index)
    {
    }

    /*override*/ FeatureType getFeatureType() const
    {
        return FACE;
    }
};

} // namespace GEO

} // namespace Aya