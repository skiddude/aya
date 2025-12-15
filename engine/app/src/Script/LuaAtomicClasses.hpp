#pragma once

#include "Lua/LuaBridge.hpp"
#include "Utility/G3DCore.hpp"
#include "Color3.hpp"
#include "CoordinateFrame.hpp"
#include "Vector3.hpp"
#include "Vector3int16.hpp"
#include "RbxRay.hpp"
#include "Utility/BrickColor.hpp"
#include "Utility/UDim.hpp"
#include "Utility/Region3.hpp"
#include "Utility/Region3Int16.hpp"
#include "Utility/Faces.hpp"
#include "Utility/Axes.hpp"
#include "Utility/PhysicalProperties.hpp"
#include "Utility/TweenInfo.hpp"
#include "DataModel/NumberSequence.hpp"
#include "DataModel/ColorSequence.hpp"
#include "DataModel/NumberRange.hpp"
#include "Utility/TweenInfo.hpp"
#include "Utility/Random.hpp"

namespace Aya
{
namespace Lua
{

class RandomBridge : public Bridge<Aya::Random>
{
    friend class Bridge<Aya::Random>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushRandom(lua_State* L, const Random& RD)
    {
        pushNewObject(L, RD);
    }

private:
    static int newRandom(lua_State* L);
    static int on_nextInteger(lua_State* L);
    static int on_nextNumber(lua_State* L);
    static int on_shuffle(lua_State* L);
    static int on_nextUnitVector(lua_State* L);
    static int on_clone(lua_State* L);

    static const luaL_reg classLibrary[];
};

class CoordinateFrameBridge : public Bridge<G3D::CoordinateFrame>
{
    friend class Bridge<G3D::CoordinateFrame>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushCoordinateFrame(lua_State* L, const G3D::CoordinateFrame& CF)
    {
        pushNewObject(L, CF);
    }

private:
    static int newCoordinateFrame(lua_State* L);
    static int fromEulerAnglesXYZ(lua_State* L);
    static int fromAxisAngle(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_mul(lua_State* L);
    static int on_inverse(lua_State* L);
    static int on_lerp(lua_State* L);

    // Implementation of G3D::CoordinateFrame help functions
    static int on_toWorldSpace(lua_State* L);
    static int on_toObjectSpace(lua_State* L);
    static int on_pointToWorldSpace(lua_State* L);
    static int on_pointToObjectSpace(lua_State* L);
    static int on_vectorToWorldSpace(lua_State* L);
    static int on_vectorToObjectSpace(lua_State* L);
    static int on_toEulerAnglesXYZ(lua_State* L);
    static int on_components(lua_State* L);

    static const luaL_reg classLibrary[];
};

class TweenInfoBridge : public Bridge<TweenInfo>
{
    friend class Bridge<TweenInfo>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushTweenInfo(lua_State* L, const TweenInfo& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newTweenInfo(lua_State* L);
    static const luaL_reg classLibrary[];
};

class PhysicalPropertiesBridge : public Bridge<PhysicalProperties>
{
    friend class Bridge<PhysicalProperties>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushPhysicalProperties(lua_State* L, const PhysicalProperties& v)
    {
        if (v.getCustomEnabled() == true)
        {
            pushNewObject(L, v);
        }
        else
        {
            lua_pushnil(L);
        }
    }

private:
    static int newPhysicalProperties(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Rect2DBridge : public Bridge<G3D::Rect2D>
{
    friend class Bridge<G3D::Rect2D>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushRect2D(lua_State* L, const G3D::Rect2D& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newRect2D(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Region3Bridge : public Bridge<Aya::Region3>
{
    friend class Bridge<Aya::Region3>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushRegion3(lua_State* L, const Aya::Region3& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newRegion3(lua_State* L);
    static int expandToGrid(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Region3int16Bridge : public Bridge<Aya::Region3int16>
{
    friend class Bridge<Aya::Region3int16>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushRegion3int16(lua_State* L, const Aya::Region3int16& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newRegion3int16(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Vector3Bridge : public Bridge<G3D::Vector3>
{
    friend class Bridge<G3D::Vector3>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushVector3(lua_State* L, const G3D::Vector3& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newVector3(lua_State* L);
    static int newVector3FromNormalId(lua_State* L);
    static int newVector3FromAxis(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_mul(lua_State* L);
    static int on_div(lua_State* L);
    static int on_unm(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Vector3int16Bridge : public Bridge<G3D::Vector3int16>
{
    friend class Bridge<G3D::Vector3int16>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushVector3int16(lua_State* L, const G3D::Vector3int16& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newVector3int16(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_mul(lua_State* L);
    static int on_div(lua_State* L);
    static int on_unm(lua_State* L);
    static const luaL_reg classLibrary[];
};

class RbxRayBridge : public Bridge<Aya::RbxRay>
{
    friend class Bridge<Aya::RbxRay>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushRay(lua_State* L, const Aya::RbxRay& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newRbxRay(lua_State* L);
    // static int on_add(lua_State *L);
    // static int on_sub(lua_State *L);
    // static int on_mul(lua_State *L);
    // static int on_div(lua_State *L);
    // static int on_unm(lua_State *L);
    static const luaL_reg classLibrary[];
};


class Vector2Bridge : public Bridge<Aya::Vector2>
{
    friend class Bridge<Aya::Vector2>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushVector2(lua_State* L, const Aya::Vector2& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newVector2(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_mul(lua_State* L);
    static int on_div(lua_State* L);
    static int on_unm(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Vector2int16Bridge : public Bridge<Aya::Vector2int16>
{
    friend class Bridge<Aya::Vector2int16>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushVector2int16(lua_State* L, const Aya::Vector2int16& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newVector2int16(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_mul(lua_State* L);
    static int on_div(lua_State* L);
    static int on_unm(lua_State* L);
    static const luaL_reg classLibrary[];
};

class Color3Bridge : public Bridge<G3D::Color3>
{
    friend class Bridge<G3D::Color3>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushColor3(lua_State* L, const G3D::Color3& color);

private:
    static int newColor3(lua_State* L);
    static int newHSVColor3(lua_State* L);
    static int newRGBColor3(lua_State* L);
    static int newHexColor3(lua_State* L);
    static int toHSVColor3(lua_State* L);
    static int toRGBColor3(lua_State* L);
    static const luaL_reg classLibrary[];
};

class UDimBridge : public Bridge<Aya::UDim>
{
    friend class Bridge<Aya::UDim>;

public:
    static void registerClassLibrary(lua_State* L);

    static void pushUDim(lua_State* L, const Aya::UDim& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newUDim(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_unm(lua_State* L);
    static const luaL_reg classLibrary[];
};

class UDim2Bridge : public Bridge<Aya::UDim2>
{
    friend class Bridge<Aya::UDim2>;

public:
    static void registerClassLibrary(lua_State* L);

private:
    static int newUDim2(lua_State* L);
    static int on_add(lua_State* L);
    static int on_sub(lua_State* L);
    static int on_unm(lua_State* L);

    static const luaL_reg classLibrary[];
};

class FacesBridge : public Bridge<Aya::Faces>
{
    friend class Bridge<Aya::Faces>;

public:
    static void registerClassLibrary(lua_State* L);

private:
    static int newFaces(lua_State* L);
    static const luaL_reg classLibrary[];
};

class AxesBridge : public Bridge<Aya::Axes>
{
    friend class Bridge<Aya::Axes>;

public:
    static void registerClassLibrary(lua_State* L);

private:
    static int newAxes(lua_State* L);
    static const luaL_reg classLibrary[];
};

class BrickColorBridge : public Bridge<Aya::BrickColor>
{
    friend class Bridge<Aya::BrickColor>;

public:
    static void registerClassLibrary(lua_State* L);

private:
    static int newBrickColor(lua_State* L);
    static int randomBrickColor(lua_State* L);
    static int paletteBrickColor(lua_State* L);
    static const luaL_reg classLibrary[];
};

// Number sequence for particle props
class NumberSequenceBridge : public Bridge<NumberSequence>
{
    friend class Bridge<NumberSequence>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushNumberSequence(lua_State* L, const NumberSequence& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newNumberSequence(lua_State* L);
    static const luaL_reg classLibrary[];
};

// Number sequence for particle props
class ColorSequenceBridge : public Bridge<ColorSequence>
{
    friend class Bridge<ColorSequence>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushColorSequence(lua_State* L, const ColorSequence& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newColorSequence(lua_State* L);
    static const luaL_reg classLibrary[];
};

class NumberSequenceKeypointBridge : public Bridge<NumberSequenceKeypoint>
{
    friend class Bridge<NumberSequenceKeypoint>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushNumberSequenceKeypoint(lua_State* L, const NumberSequenceKeypoint& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newNumberSequenceKeypoint(lua_State* L);
    static const luaL_reg classLibrary[];
};

class ColorSequenceKeypointBridge : public Bridge<ColorSequenceKeypoint>
{
    friend class Bridge<ColorSequenceKeypoint>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushColorSequenceKeypoint(lua_State* L, const ColorSequenceKeypoint& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newColorSequenceKeypoint(lua_State* L);
    static const luaL_reg classLibrary[];
};

class NumberRangeBridge : public Bridge<NumberRange>
{
    friend class Bridge<NumberRange>;

public:
    static void registerClassLibrary(lua_State* L);
    static void pushNumberRange(lua_State* L, const NumberRange& v)
    {
        pushNewObject(L, v);
    }

private:
    static int newNumberRange(lua_State* L);
    static const luaL_reg classLibrary[];
};

// Specialization to implement arithmatic operators
template<>
void Bridge<G3D::Vector3int16>::registerClass(lua_State* L);

template<>
void Bridge<G3D::Vector3>::registerClass(lua_State* L);

template<>
void Bridge<Aya::Vector2>::registerClass(lua_State* L);

template<>
void Bridge<G3D::CoordinateFrame>::registerClass(lua_State* L);

// Specialization to implement arithmatic operators
template<>
void Bridge<Aya::UDim>::registerClass(lua_State* L);

// Specialization to implement arithmatic operators
template<>
void Bridge<Aya::UDim2>::registerClass(lua_State* L);

} // namespace Lua
} // namespace Aya
