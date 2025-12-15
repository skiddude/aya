
#include "Lua/LuaBridge.hpp"
#include "CoordinateFrame.hpp"
#include "Color3.hpp"
#include "Vector3.hpp"
#include "Vector2.hpp"
#include "RbxRay.hpp"
#include "Utility/BrickColor.hpp"
#include "Utility/Random.hpp"
#include "Utility/Region3.hpp"
#include "Utility/Region3Int16.hpp"
#include "Utility/Faces.hpp"
#include "Utility/Axes.hpp"
#include "Utility/UDim.hpp"
#include "Utility/PhysicalProperties.hpp"
#include "Script/ThreadRef.hpp"
#include "Script/ScriptEvent.hpp"
#include "Script/LuaSignalBridge.hpp"
#include "Script/LuaEnum.hpp"
#include "Script/LuaLibrary.hpp"
#include "Script/LuaInstanceBridge.hpp"
#include "DataModel/NumberSequence.hpp"
#include "DataModel/ColorSequence.hpp"
#include "DataModel/NumberRange.hpp"
#include "Utility/TweenInfo.hpp"

namespace Aya
{
namespace Lua
{
template<class Class, bool __eq>
int Bridge<Class, __eq>::on_tostring(const Class& object, lua_State* L)
{
    std::string s = Aya::StringConverter<Class>::convertToString(object);
    lua_pushstring(L, s);
    return 1;
}


/// This is to force the above implementation to be generated for following class Types

/// If you are doing a specialization for the on_tostring() then you do *not* need to add the following line.
/// Basically the specialization is needed if your class does not have the Aya::StringConverter<YourClass>::convertToString() implementation
/// To see an e.g for specialization, just look up LuaEnum.h LuaEnum.cpp, search for on_tostring()

/// We need the forward declaration here because we want the above default on_tostring() implementation to be generated for the following class types
/// If we do *not* add the following following declaration then the Microsoft C++ compiler complains that it cannot find the implementation of
/// on_string for this classes On GCC I have *not* seen a need for this forward declaration. If we keep the above on_string implementation in header
/// then the forward declaration is *not* needed. But then on GCC we get duplicate implementation for the specialized classes, one with default & one
/// with specialization.

/// For more details refer to http://www.parashift.com/c++-faq-lite/templates.html#faq-35.13
/// Also this case has been documented in the Porting Error Cases Document found at Roblox Documents
/// https://docs0.google.com/a/roblox.com/document/d/100HFz6lRHJKP88UOshPrrnRoHavNZ5upT6ePIkgcYFc/edit?hl=en#

template int Bridge<G3D::Color3>::on_tostring(const G3D::Color3&, lua_State*);
template int Bridge<Aya::RbxRay>::on_tostring(const Aya::RbxRay&, lua_State*);
template int Bridge<G3D::Vector3int16>::on_tostring(const G3D::Vector3int16&, lua_State*);
template int Bridge<G3D::Vector2int16>::on_tostring(const G3D::Vector2int16&, lua_State*);
template int Bridge<G3D::Vector3>::on_tostring(const G3D::Vector3&, lua_State*);
template int Bridge<Aya::Vector2>::on_tostring(const Aya::Vector2&, lua_State*);
template int Bridge<Aya::Rect2D>::on_tostring(const Aya::Rect2D&, lua_State*);
template int Bridge<PhysicalProperties>::on_tostring(const Aya::PhysicalProperties&, lua_State*);
template int Bridge<Aya::TweenInfo>::on_tostring(const Aya::TweenInfo&, lua_State*);
template int Bridge<Aya::BrickColor>::on_tostring(const Aya::BrickColor&, lua_State*);
template int Bridge<G3D::CoordinateFrame>::on_tostring(const G3D::CoordinateFrame&, lua_State*);
template int Bridge<Aya::Faces>::on_tostring(const Aya::Faces&, lua_State*);
template int Bridge<Aya::Axes>::on_tostring(const Aya::Axes&, lua_State*);
template int Bridge<Aya::UDim>::on_tostring(const Aya::UDim&, lua_State*);
template int Bridge<Aya::UDim2>::on_tostring(const Aya::UDim2&, lua_State*);
template int Bridge<Aya::Random>::on_tostring(const Aya::Random&, lua_State*);

template<class Class, bool __eq>
void Bridge<Class, __eq>::registerClass(lua_State* L)
{
    // Register the object events
    luaL_newmetatable(L, className);
    Lua::protect_metatable(L, -1);

    lua_pushstring(L, "__type");
    lua_pushstring(L, className);
    lua_settable(L, -3);

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, on_index);
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, on_newindex);
    lua_settable(L, -3);

    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, on_gc);
    lua_settable(L, -3);

    if (__eq)
    {
        lua_pushstring(L, "__eq");
        lua_pushcfunction(L, on_eq);
        lua_settable(L, -3);
    }

    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, on_tostring);
    lua_settable(L, -3);

    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

/// This is to force the above implementation to be generated for following class Types

/// If you are doing a specialization for the registerClass() then you do *not* need to add the following line.

/// We need the forward declaration here because we want the above default registerClass() implementation to be generated for the following class
/// types If we do *not* add the following following declaration then the Microsoft C++ compiler complains that it cannot find the implementation of
/// registerClass for this classes On GCC I have *not* seen a need for this forward declaration. If we keep the above registerClass implementation in
/// header then the forward declaration is *not* needed. But then on GCC we get duplicate implementation for the specialized classes, one with default
/// & one with specialization.

/// For more details refer to http://www.parashift.com/c++-faq-lite/templates.html#faq-35.13
/// Also this case has been documented in the Porting Error Cases Document found at Roblox Documents
/// https://docs0.google.com/a/roblox.com/document/d/100HFz6lRHJKP88UOshPrrnRoHavNZ5upT6ePIkgcYFc/edit?hl=en#

template void Bridge<Aya::Axes>::registerClass(lua_State*);
template void Bridge<Aya::Faces>::registerClass(lua_State*);
template void Bridge<Aya::BrickColor>::registerClass(lua_State*);
template void Bridge<Aya::RbxRay>::registerClass(lua_State*);
template void Bridge<Aya::Region3>::registerClass(lua_State*);
template void Bridge<Aya::Region3int16>::registerClass(lua_State*);
template void Bridge<G3D::Color3>::registerClass(lua_State*);
template void Bridge<boost::intrusive_ptr<class WeakThreadRef::Node>>::registerClass(lua_State*);
template void Bridge<shared_ptr<GenericFunction>>::registerClass(lua_State*);
template void Bridge<shared_ptr<GenericAsyncFunction>>::registerClass(lua_State*);
template void Bridge<shared_ptr<class Aya::Instance>, false>::registerClass(lua_State*);
template void Bridge<Aya::signals::connection>::registerClass(lua_State*);
template void Bridge<Library>::registerClass(lua_State*);
template void Bridge<EventInstance>::registerClass(lua_State*);
template void Bridge<AllEnumDescriptorsPtr, false>::registerClass(lua_State*);
template void Bridge<EnumDescriptorPtr, false>::registerClass(lua_State*);
template void Bridge<EnumDescriptorItemPtr, false>::registerClass(lua_State*);
template void Bridge<Aya::NumberSequenceKeypoint>::registerClass(lua_State*);
template void Bridge<Aya::ColorSequenceKeypoint>::registerClass(lua_State*);
template void Bridge<Aya::NumberSequence>::registerClass(lua_State*);
template void Bridge<Aya::ColorSequence>::registerClass(lua_State*);
template void Bridge<Aya::NumberRange>::registerClass(lua_State*);
template void Bridge<Aya::TweenInfo>::registerClass(lua_State*);
template void Bridge<Aya::Random>::registerClass(lua_State*);
} // namespace Lua
} // namespace Aya
