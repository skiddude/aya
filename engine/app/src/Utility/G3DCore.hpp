

// this is just to stop multiple includes,
// but why the hell was it originally
// #define _70F7A2EE1B6E4dd0AF07E4BFA609A3D1 ????
#ifndef G3D_CORE_INCLUDED
#define G3D_CORE_INCLUDED

#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "Matrix3.hpp"
#include "Matrix4.hpp"
#include "Vector3int16.hpp"
#include "Vector2int16.hpp"
#include "Color4uint8.hpp"
#include "Color3uint8.hpp"
#include "CoordinateFrame.hpp"
#include "Plane.hpp"
#include "Line.hpp"
#include "LineSegment.hpp"

#include "AABox.hpp"
#include "Box.hpp"
#include "RbxCamera.hpp"
#include "Color3.hpp"
#include "Color4.hpp"
#include "g3dmath.hpp"
#include "Rect2D.hpp"
#include "Sphere.hpp"

#include "vectorMath.hpp"
#include "G3DDebug.hpp"

// TODO: this can cause namespace collisions:
// using G3D::Array;

namespace Aya
{
typedef G3D::Vector2 Vector2;
typedef G3D::Vector3 Vector3;
typedef G3D::Vector4 Vector4;
typedef G3D::Vector2int16 Vector2int16;
typedef G3D::Vector3int16 Vector3int16;
typedef G3D::Color4uint8 Color4uint8;
typedef G3D::Color3uint8 Color3uint8;
typedef G3D::Matrix3 Matrix3;
typedef G3D::Matrix4 Matrix4;
typedef G3D::CoordinateFrame CoordinateFrame;
typedef Aya::RbxRay Ray;
typedef G3D::Plane Plane;
typedef G3D::Line Line;
typedef G3D::LineSegment LineSegment;
typedef G3D::Color3 Color3;
typedef G3D::Color4 Color4;
typedef G3D::Rect2D Rect2D;
typedef G3D::Box Box;
typedef G3D::AABox AABox;
typedef G3D::Sphere Sphere;

enum IntersectResult
{
    irNone = 0,
    irPartial = 1,
    irFull = 2
};
} // namespace Aya

namespace G3D
{
std::size_t hash_value(const G3D::Vector3& v);
std::size_t hash_value(const G3D::Vector3int16& v);
} // namespace G3D

#endif