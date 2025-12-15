

#include "Script/LuaInstanceBridge.hpp"
#include "Script/ScriptContext.hpp"
#include "Utility/BrickColor.hpp"
#include "DataModel/Decal.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Folder.hpp"
#include "Script/script.hpp"
#include "GUI/GUI.hpp"
#include "DataModel/Stats.hpp"
#include "DataModel/PlayerGui.hpp"
#include "Script/LuaSignalBridge.hpp"
#include "Script/LuaArguments.hpp"
#include "Script/LuaEnum.hpp"
#include "Script/ThreadRef.hpp"
#include "Utility/Sound.hpp"
#include "Utility/MeshId.hpp"
#include "Utility/AnimationId.hpp"
#include "Utility/StandardOut.hpp"
#include "Players.hpp"
#include "Player.hpp"
#include "DataModel/Camera.hpp"
#include "Tree/Service.hpp"
#include "DataModel/Hopper.hpp"

#include "DataModel/DebugSettings.hpp"
#include "Reflection/EnumConverter.hpp"
#include "Reflection/Type.hpp"

#include "Script/DebuggerManager.hpp"

#include "Profiler.hpp"

FASTFLAG(LuaDebugger)

void Aya::Lua::newweaktable(lua_State* L, const char* mode)
{
    lua_newtable(L);
    lua_pushvalue(L, -1); // table is its own metatable
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "__mode");
    lua_pushstring(L, mode);
    lua_settable(L, -3); // metatable.__mode = mode
}

using namespace Aya;
using namespace Aya::Reflection;
using namespace Aya::Lua;

template<>
const char* Bridge<shared_ptr<Instance>, false>::className("Object");


const luaL_reg ObjectBridge::classLibrary[] = {{"new", newInstance}, {"Lock", lockInstance}, {"Unlock", unlockInstance}, {NULL, NULL}};

// Returns a new Instance.
// param1: the name of the Instance requested
// param2: the Parent of the Instance (optional)
int ObjectBridge::newInstance(lua_State* thread)
{
    const char* name = throwable_lua_tostring(thread, 1);
    const Aya::Name& className = Aya::Name::lookup(name);

    shared_ptr<Instance> instance = Creatable<Instance>::createByName(className, Aya::ScriptingCreator);
    if (!instance)
        throw Aya::runtime_error("Unable to create an Instance of type \"%s\"", name);

    if (lua_gettop(thread) >= 2)
    {
        shared_ptr<Instance> parent = ObjectBridge::getInstance(thread, 2);
        if (parent)
        {
            // only allow parenting to roblox locked objects if we are in the right permission level
            if (parent->getRobloxLocked())
                Aya::Security::Context::current().requirePermission(Aya::Security::Plugin, "Parent in Instance.new()");

            instance->setParent(parent.get());
        }
    }

    ObjectBridge::push(thread, instance);
    return 1;
}

int ObjectBridge::lockInstance(lua_State* thread)
{
    // This function is deprecated. Just pretend it succeeded
    lua_pushboolean(thread, true);
    return 1;
}


int ObjectBridge::unlockInstance(lua_State* thread)
{
    // This function is deprecated. Do nothing
    return 0;
}



static void pushLuaValue(Aya::Reflection::ConstProperty p, lua_State* L, Aya::Security::Context& securityContext)
{
    const Aya::Reflection::PropertyDescriptor& desc(p.getDescriptor());

    if (desc.security != Aya::Security::None)
        securityContext.requirePermission(desc.security, desc.name.c_str());

    if (!desc.isScriptable())
        throw Aya::runtime_error("Unable to query property %s. It is not scriptable", p.getName().c_str());

    if (desc.type == Type::singleton<int>())
    {
        lua_pushinteger(L, p.getValue<int>());
        return;
    }

    if (desc.type == Type::singleton<int64_t>())
    {
        lua_pushinteger(L, p.getValue<int64_t>());
        return;
    }

    if (desc.type == Type::singleton<bool>())
    {
        lua_pushboolean(L, p.getValue<bool>());
        return;
    }

    if (desc.type == Type::singleton<float>())
    {
        lua_pushnumber(L, p.getValue<float>());
        return;
    }

    if (desc.type == Type::singleton<double>())
    {
        lua_pushnumber(L, p.getValue<double>());
        return;
    }

    if (desc.type == Type::singleton<ProtectedString>())
    {
        lua_pushstring(L, p.getStringValue());
        return;
    }

    if (desc.type == Type::singleton<std::string>())
    {
        lua_pushstring(L, p.getStringValue());
        return;
    }

    if (desc.type == Type::singleton<shared_ptr<Instance>>())
    {
        ObjectBridge::push(L, p.getValue<shared_ptr<Instance>>());
        return;
    }

    if (desc.type == Type::singleton<G3D::Rect2D>())
    {
        Rect2DBridge::pushRect2D(L, p.getValue<G3D::Rect2D>());
        return;
    }

    if (desc.type == Type::singleton<PhysicalProperties>())
    {
        PhysicalPropertiesBridge::pushPhysicalProperties(L, p.getValue<PhysicalProperties>());
        return;
    }

    if (desc.type == Type::singleton<TweenInfo>())
    {
        TweenInfoBridge::pushTweenInfo(L, p.getValue<TweenInfo>());
        return;
    }

    if (desc.type == Type::singleton<G3D::Vector3>())
    {
        Vector3Bridge::pushVector3(L, p.getValue<G3D::Vector3>());
        return;
    }

    if (desc.type == Type::singleton<G3D::Vector2>())
    {
        Vector2Bridge::pushVector2(L, p.getValue<G3D::Vector2>());
        return;
    }

    if (desc.type == Type::singleton<Aya::RbxRay>())
    {
        RbxRayBridge::pushNewObject(L, p.getValue<Aya::RbxRay>());
        return;
    }

    if (desc.type == Type::singleton<G3D::CoordinateFrame>())
    {
        CoordinateFrameBridge::pushCoordinateFrame(L, p.getValue<G3D::CoordinateFrame>());
        return;
    }

    if (desc.type == Type::singleton<G3D::Color3>())
    {
        Color3Bridge::pushColor3(L, p.getValue<G3D::Color3>());
        return;
    }

    if (desc.type == Type::singleton<BrickColor>())
    {
        BrickColorBridge::pushNewObject(L, p.getValue<BrickColor>());
        return;
    }

    if (desc.type == Type::singleton<UDim>())
    {
        UDimBridge::pushNewObject(L, p.getValue<UDim>());
        return;
    }

    if (desc.type == Type::singleton<UDim2>())
    {
        UDim2Bridge::pushNewObject(L, p.getValue<UDim2>());
        return;
    }

    if (desc.type == Type::singleton<Faces>())
    {
        FacesBridge::pushNewObject(L, p.getValue<Faces>());
        return;
    }
    if (desc.type == Type::singleton<Axes>())
    {
        AxesBridge::pushNewObject(L, p.getValue<Axes>());
        return;
    }

    if (desc.type == Type::singleton<Region3int16>())
    {
        Region3int16Bridge::pushNewObject(L, p.getValue<Region3int16>());
        return;
    }

    if (desc.type == Type::singleton<ContentId>())
    {
        lua_pushstring(L, p.getStringValue());
        return;
    }

    if (desc.type == Type::singleton<NumberSequenceKeypoint>())
    {
        NumberSequenceKeypointBridge::pushNewObject(L, p.getValue<NumberSequenceKeypoint>());
        return;
    }

    if (desc.type == Type::singleton<ColorSequenceKeypoint>())
    {
        ColorSequenceKeypointBridge::pushNewObject(L, p.getValue<ColorSequenceKeypoint>());
        return;
    }

    if (desc.type == Type::singleton<NumberSequence>())
    {
        NumberSequenceBridge::pushNewObject(L, p.getValue<NumberSequence>());
        return;
    }

    if (desc.type == Type::singleton<ColorSequence>())
    {
        ColorSequenceBridge::pushNewObject(L, p.getValue<ColorSequence>());
        return;
    }

    if (desc.type == Type::singleton<NumberRange>())
    {
        NumberRangeBridge::pushNewObject(L, p.getValue<NumberRange>());
        return;
    }

    if (desc.type == Type::singleton<shared_ptr<const Instances>>())
    {
        // Return a table!
        shared_ptr<const Instances> objects = p.getValue<shared_ptr<const Instances>>();
        if (objects)
        {
            Instances::const_iterator iter = objects->begin();
            Instances::const_iterator end = objects->end();
            lua_createtable(L, end - iter, 0);
            for (int i = 1; iter != end; ++iter, ++i)
            {
                ObjectBridge::push(L, *iter);
                lua_rawseti(L, -2, i);
            }
        }
        else
        {
            // return an empty table (never return nil)
            lua_createtable(L, 0, 0);
        }
        return;
    }

    if (p.getDescriptor().bIsEnum)
    {
        const EnumPropertyDescriptor* enumDesc = static_cast<const EnumPropertyDescriptor*>(&p.getDescriptor());
        if (enumDesc != NULL)
        {
            if (const EnumDescriptor::Item* item = enumDesc->getEnumItem(p.getInstance()))
            {
                EnumItem::push(L, item);
                return;
            }
            throw Aya::runtime_error("Invalid value for enum %s", enumDesc->name.c_str());
        }
    }


    if (RefPropertyDescriptor::isRefPropertyDescriptor(p.getDescriptor()))
    {
        const RefPropertyDescriptor* refDesc = static_cast<const RefPropertyDescriptor*>(&p.getDescriptor());
        if (refDesc != NULL)
        {
            // Throws exception if the value is the wrong type
            Reflection::DescribedBase* value = refDesc->getRefValue(p.getInstance());

            // Currently, we only support Instance* refs.
            if (value != NULL)
                ObjectBridge::push(L, shared_from(boost::polymorphic_downcast<Instance*>(value)));
            else
                ObjectBridge::push(L, shared_ptr<Instance>());
            return;
        }
    }


    // TODO: Implement ValueArray

    throw Aya::runtime_error("Unable to get property %s, type %s", p.getName().c_str(), desc.type.name.c_str());
}


static const char* PropertyNameCorrection(const shared_ptr<Instance>& object, const char* name, lua_State* L)
{
    if (name[0] == 's')
    {
        if (strcmp(name, "size") == 0)
        {
            ScriptContext::getContext(L).camelCaseViolation(object, name, RobloxExtraSpace::get(L)->script.lock());
            return "Size";
        }
    }
    return name;
}

namespace Aya
{
namespace Lua
{

template<>
int Bridge<shared_ptr<Instance>, false>::on_index(const shared_ptr<Instance>& object, const char* name, lua_State* L)
{
    if (object == NULL)
        throw std::runtime_error("The object has been deleted");

    AYAPROFILER_SCOPE("LuaBridge", "$index");
    AYAPROFILER_LABELF("LuaBridge", "%s.%s", object->getDescriptor().name.c_str(), name);

    name = PropertyNameCorrection(object, name, L);

    Aya::Security::Context& securityContext = Aya::Security::Context::current();


    // Look for a property:
    if (PropertyDescriptor* prop = object->findPropertyDescriptor(name))
    {
        object->securityCheck(securityContext);
        pushLuaValue(Property(*prop, object.get()), L, securityContext);
        return 1;
    }

    // Look for a yield function
    if (Reflection::YieldFunctionDescriptor* func = object->findYieldFunctionDescriptor(name))
    {
        void* desc = reinterpret_cast<void*>(func);

        // Once the closure is created, we store it in the registry for re-use
        // This has 2 advantages:
        //    1) I think it is faster to copy a gc object from the registry than to push a closure
        //    2) You get the same object back each time, so relational operators are meaningful

        // attempt to get the closure from the registry
        lua_pushlightuserdata(L, desc);  // Stack:   desc
        lua_rawget(L, LUA_ENVIRONINDEX); // Stack:   closure?
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // Stack:

            // push "FunctionDescriptor*" onto the stack as an up-value
            lua_pushlightuserdata(L, desc); // Stack:   desc
            lua_pushcclosure(L, ObjectBridge::callMemberYieldFunction, 1);
            // Stack:   closure
            lua_pushlightuserdata(L, desc);    // Stack:   closure, desc
            lua_pushvalue(L, -2);              // Stack:   closure, desc, closure
            lua_settable(L, LUA_ENVIRONINDEX); // Stack:   closure
        }
        AYAASSERT(lua_isfunction(L, -1)); // Stack:   closure
        return 1;
    }

    // Look for a function:
    if (Reflection::FunctionDescriptor* func = object->findFunctionDescriptor(name))
    {
        void* desc = reinterpret_cast<void*>(func);

        // Once the closure is created, we store it in the registry for re-use
        // This has 2 advantages:
        //    1) I think it is faster to copy a gc object from the registry than to push a closure
        //    2) You get the same object back each time, so relational operators are meaningful

        // attempt to get the closure from the registry
        lua_pushlightuserdata(L, desc);  // Stack:   desc
        lua_rawget(L, LUA_ENVIRONINDEX); // Stack:   closure?
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // Stack:

            // push "FunctionDescriptor*" onto the stack as an up-value
            lua_pushlightuserdata(L, desc); // Stack:   desc
            lua_pushcclosure(L, ObjectBridge::callMemberFunction, 1);
            // Stack:   closure
            lua_pushlightuserdata(L, desc);    // Stack:   closure, desc
            lua_pushvalue(L, -2);              // Stack:   closure, desc, closure
            lua_settable(L, LUA_ENVIRONINDEX); // Stack:   closure
        }
        AYAASSERT(lua_isfunction(L, -1)); // Stack:   closure
        return 1;
    }

    // HACK: GetPropertyChangedSignal
    if (strcmp(name, "GetPropertyChangedSignal") == 0)
    {
        lua_pushlightuserdata(L, object.get());
        lua_pushcclosure(L, ObjectBridge::getPropertyChangedSignalBridge, 1);
        return 1;
    }

    // Look for a Signal:
    if (EventDescriptor* signal = object->findSignalDescriptor(name))
    {
        object->securityCheck(securityContext);
        if (object->getRobloxLocked())
            securityContext.requirePermission(Aya::Security::Plugin, name);

        EventInstance evt = {signal, object};
        EventBridge::pushNewObject(L, evt);
        return 1;
    }

    // Look for a child with the same name
    {
        Aya::Instance* child = object->findFirstChildByName(name);
        if (child != NULL)
        {
            object->securityCheck(securityContext);
            if (object->getRobloxLocked())
                securityContext.requirePermission(Aya::Security::Plugin, name);
            ObjectBridge::push(L, shared_from(child));
            return 1;
        }
    }

    if (Aya::Name::lookup(name).empty())
    {
        // Short-term hack to handle old-style camelCase members
        std::string name2 = name;
        name2[0] = toupper(name[0]);
        // TODO: We should only look up members of the class in question, not generically for ALL names in the world!
        if (name2[0] != name[0] && !Aya::Name::lookup(name2).empty())
        {
            int result = on_index(object, name2.c_str(), L);
            ScriptContext::getContext(L).camelCaseViolation(object, name, RobloxExtraSpace::get(L)->script.lock());
            return result;
        }
    }

    if (object->findCallbackDescriptor(name))
    {
        throw Aya::runtime_error(
            "%s is a callback member of %s; you can only set the callback value, get is not available", name, object->getDescriptor().name.c_str());
    }

    // Failure
    throw Aya::runtime_error("%s is not a valid member of %s", name, object->getDescriptor().name.c_str());
}

} // namespace Lua
} // namespace Aya


static void assignLuaValue(Aya::Reflection::Property p, lua_State* L, int index, Aya::Security::Context& securityContext)
{
    const Aya::Reflection::PropertyDescriptor& desc(p.getDescriptor());

    if (desc.security != Aya::Security::None)
        securityContext.requirePermission(desc.security, desc.name.c_str());

    if (!desc.isScriptable())
        throw Aya::runtime_error("Unable to assign property %s. It is not scriptable", p.getName().c_str());

    if (desc.type == Type::singleton<int>())
    {
        p.setValue<int>(lua_tointeger(L, index));
        return;
    }

    if (desc.type == Type::singleton<int64_t>())
    {
        p.setValue<int64_t>(lua_tointeger(L, index));
        return;
    }

    if (desc.type == Type::singleton<bool>())
    {
        p.setValue<bool>(lua_toboolean(L, index) ? true : false);
        return;
    }

    if (desc.type == Type::singleton<float>())
    {
        p.setValue<float>(lua_tofloat(L, index));
        return;
    }

    if (desc.type == Type::singleton<double>())
    {
        p.setValue<double>(lua_tonumber(L, index));
        return;
    }
    if (desc.type == Type::singleton<ProtectedString>())
    {
        p.setValue<ProtectedString>(ProtectedString::fromTrustedSource(throwable_lua_tostring(L, index)));
        return;
    }
    if (desc.type == Type::singleton<std::string>())
    {
        p.setValue<std::string>(throwable_lua_tostring(L, index));
        return;
    }

    if (desc.type == Type::singleton<shared_ptr<Instance>>())
    {
        p.setValue<shared_ptr<Instance>>(ObjectBridge::getInstance(L, index));
        return;
    }

    if (desc.type == Type::singleton<shared_ptr<Instance>>())
    {
        p.setValue<shared_ptr<Instance>>(ObjectBridge::getPtr(L, index));
        return;
    }

    if (desc.type == Type::singleton<G3D::Vector3>())
    {
        p.setValue<G3D::Vector3>(Vector3Bridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<G3D::Vector2>())
    {
        p.setValue<G3D::Vector2>(Vector2Bridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<G3D::Rect2D>())
    {
        p.setValue<G3D::Rect2D>(Rect2DBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<PhysicalProperties>())
    {
        if (lua_isnil(L, index))
        {
            p.setValue<PhysicalProperties>(PhysicalProperties());
            return;
        }
        else
        {
            p.setValue<PhysicalProperties>(PhysicalPropertiesBridge::getObject(L, index));
            return;
        }
    }

    if (desc.type == Type::singleton<TweenInfo>())
    {
        p.setValue<TweenInfo>(TweenInfoBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<Aya::RbxRay>())
    {
        p.setValue<Aya::RbxRay>(RbxRayBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<G3D::CoordinateFrame>())
    {
        p.setValue<G3D::CoordinateFrame>(CoordinateFrameBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<G3D::Color3>())
    {
        p.setValue<G3D::Color3>(Color3Bridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<UDim>())
    {
        p.setValue<UDim>(UDimBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<UDim2>())
    {
        p.setValue<UDim2>(UDim2Bridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<Faces>())
    {
        p.setValue<Faces>(FacesBridge::getObject(L, index));
        return;
    }
    if (desc.type == Type::singleton<Axes>())
    {
        p.setValue<Axes>(AxesBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<BrickColor>())
    {
        p.setValue<BrickColor>(BrickColorBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<NumberSequence>())
    {
        p.setValue<NumberSequence>(NumberSequenceBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<ColorSequence>())
    {
        p.setValue<ColorSequence>(ColorSequenceBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<NumberSequenceKeypoint>())
    {
        p.setValue<NumberSequenceKeypoint>(NumberSequenceKeypointBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<ColorSequenceKeypoint>())
    {
        p.setValue<ColorSequenceKeypoint>(ColorSequenceKeypointBridge::getObject(L, index));
        return;
    }

    if (desc.type == Type::singleton<NumberRange>())
    {
        p.setValue<NumberRange>(NumberRangeBridge::getObject(L, index));
        return;
    }


    const TypedPropertyDescriptor<Aya::Soundscape::SoundId>* soundProp =
        dynamic_cast<const TypedPropertyDescriptor<Aya::Soundscape::SoundId>*>(&p.getDescriptor());
    if (soundProp != NULL)
    {
        p.setValue<Aya::Soundscape::SoundId>(ContentId(throwable_lua_tostring(L, index)));
        return;
    }

    const TypedPropertyDescriptor<Aya::TextureId>* textureProp = dynamic_cast<const TypedPropertyDescriptor<Aya::TextureId>*>(&p.getDescriptor());
    if (textureProp != NULL)
    {
        p.setValue<TextureId>(ContentId(throwable_lua_tostring(L, index)));
        return;
    }

    const TypedPropertyDescriptor<Aya::MeshId>* meshProp = dynamic_cast<const TypedPropertyDescriptor<Aya::MeshId>*>(&p.getDescriptor());
    if (meshProp != NULL)
    {
        p.setValue<MeshId>(ContentId(throwable_lua_tostring(L, index)));
        return;
    }

    const TypedPropertyDescriptor<Aya::AnimationId>* animationProp =
        dynamic_cast<const TypedPropertyDescriptor<Aya::AnimationId>*>(&p.getDescriptor());
    if (animationProp != NULL)
    {
        p.setValue<AnimationId>(ContentId(throwable_lua_tostring(L, index)));
        return;
    }


    const EnumPropertyDescriptor* enumDesc = dynamic_cast<const EnumPropertyDescriptor*>(&p.getDescriptor());
    if (enumDesc != NULL)
    {
        EnumDescriptorItemPtr item;
        if (EnumItem::getItem(L, index, item))
        {
            if (!enumDesc->setEnumItem(p.getInstance(), *item))
                throw Aya::runtime_error("Invalid type %s for enum %s", item->owner.name.c_str(), enumDesc->name.c_str());
        }
        else if (lua_isnumber(L, index))
        {
            if (!enumDesc->setEnumValue(p.getInstance(), lua_tointeger(L, index)))
                throw Aya::runtime_error("Invalid value %d for enum %s", static_cast<int>(lua_tointeger(L, index)), enumDesc->name.c_str());
        }
        else if (lua_isstring(L, index))
        {
            if (!enumDesc->setStringValue(p.getInstance(), lua_tostring(L, index)))
                throw Aya::runtime_error("Invalid value \"%s\" for enum %s", lua_tostring(L, index), enumDesc->name.c_str());
        }
        else
            throw Aya::runtime_error("Invalid value for enum %s", enumDesc->name.c_str());
        return;
    }

    const RefPropertyDescriptor* refDesc = dynamic_cast<const RefPropertyDescriptor*>(&p.getDescriptor());
    if (refDesc != NULL)
    {
        shared_ptr<Instance> value = ObjectBridge::getInstance(L, index);
        refDesc->setRefValue(p.getInstance(), value.get());
        return;
    }

    throw Aya::runtime_error("Unable to set property %s, type %s", p.getName().c_str(), p.getDescriptor().type.name.c_str());
}


static void readResults(shared_ptr<Reflection::Tuple>& result, lua_State* thread, size_t returnCount)
{
    result.reset(new Reflection::Tuple(returnCount));
    for (size_t i = 0; i < returnCount; ++i)
    {
        Reflection::Variant& v = result->values.at(i);
        LuaArguments::get(thread, i + 1, v, true);
    }
}

namespace Aya
{
namespace Lua
{
shared_ptr<Tuple> callCallback(Lua::WeakFunctionRef function, shared_ptr<const Tuple> args, boost::intrusive_ptr<WeakThreadRef> cachedCallbackThread)
{
    ThreadRef functionThread = function.lock();
    if (!functionThread)
        throw std::runtime_error("Script that implemented this callback has been destroyed");

    AYAASSERT_BALLANCED_LUA_STACK(functionThread);

    // Create a child thread that will execute the callback
    ThreadRef callbackThread = cachedCallbackThread->lock();
    bool createdThread;
    if (callbackThread)
        createdThread = false;
    else
    {
        int top = lua_gettop(functionThread);

        callbackThread = lua_newthread(functionThread);
        AYAASSERT(lua_isthread(functionThread, -1));

        while (lua_gettop(functionThread) > top + 1) // oldTop, ???, slotThread
        {
            // This is because of a strange bug in Lua???
            lua_remove(functionThread, -2);
        }
        AYAASSERT(lua_isthread(functionThread, -1));
        AYAASSERT(top + 1 == lua_gettop(functionThread)); // oldTop, slotThread

        createdThread = true;
    }

    {
        AYAASSERT_BALLANCED_LUA_STACK(callbackThread);

        // Push the function onto the stack
        lua_pushfunction(functionThread, function); // createdThread ? (oldTop, function) : (oldTop, slotThread, function)

        // Transfer function to callbackThread
        lua_xmove(functionThread, callbackThread, 1); // createdThread ? (oldTop) : (oldTop, slotThread)

        const int stackSize = lua_gettop(callbackThread);

        // Push arguments onto callback's stack
        const int nargs = LuaArguments::pushTuple(*args, callbackThread);

        // HACK: Make sure we don't yield while debugging
        Aya::Scripting::ScriptDebugger* pDebugger = NULL;
        if (::FFlag::LuaDebugger)
            pDebugger = Aya::Scripting::DebuggerManager::singleton().findDebugger(callbackThread.get());

        if (pDebugger)
            pDebugger->setIgnoreDebuggerBreak(true);

        const ScriptContext::Result result = ScriptContext::getContext(callbackThread.get()).resume(callbackThread, nargs);

        if (pDebugger)
            pDebugger->setIgnoreDebuggerBreak(false);

        // Pop callbackThread from the parent's stack
        if (createdThread)
        {
            AYAASSERT(lua_isthread(functionThread, -1));
            lua_pop(functionThread, 1);
        }

        switch (result)
        {
        case ScriptContext::Success:
            // Since the slot didn't yield, we can re-use this thread the next time
            if (createdThread)
                *cachedCallbackThread = WeakThreadRef(callbackThread);
            break;

        case ScriptContext::Yield:
            // Don't re-use this thread
            cachedCallbackThread->reset();

            // Clear the stack
            lua_resetstack(callbackThread, 0);
            throw std::runtime_error("Callbacks cannot yield");

        case ScriptContext::Error:
            // Don't re-use this thread
            cachedCallbackThread->reset();

            std::string err = safe_lua_tostring(callbackThread, -1);
            // Clear the stack
            lua_resetstack(callbackThread, 0);
            throw std::runtime_error(err);
        }

        // Collect all the return arguments into a ValueArray
        const int returnCount = lua_gettop(callbackThread) - stackSize + 1;
        shared_ptr<Tuple> tuple;
        readResults(tuple, callbackThread, returnCount);
        lua_resetstack(callbackThread, 0);

        return tuple;
    }
}

void callAsyncCallbackSuccess(AsyncCallbackDescriptor::ResumeFunction resumeFunction, lua_State* L)
{
    shared_ptr<Tuple> tuple;
    readResults(tuple, L, lua_gettop(L));

    lua_resetstack(L, 0);

    resumeFunction(tuple);
}

void callAsyncCallbackError(AsyncCallbackDescriptor::ErrorFunction errorFunction, lua_State* L)
{
    std::string err = safe_lua_tostring(L, -1);

    errorFunction(err);
}

void callAsyncCallback(Lua::WeakFunctionRef function, shared_ptr<const Tuple> args, AsyncCallbackDescriptor::ResumeFunction resumeFunction,
    AsyncCallbackDescriptor::ErrorFunction errorFunction, boost::intrusive_ptr<WeakThreadRef> cachedCallbackThread)
{
    ThreadRef functionThread = function.lock();
    if (!functionThread)
        throw std::runtime_error("Script that implemented this callback has been destroyed");

    AYAASSERT_BALLANCED_LUA_STACK(functionThread);

    // Create a child thread that will execute the callback
    ThreadRef callbackThread = cachedCallbackThread->lock();
    bool createdThread;
    if (callbackThread)
        createdThread = false;
    else
    {
        callbackThread = lua_newthread(functionThread);

        lua_pop(functionThread, 1);

        createdThread = true;
    }

    // Push the function onto the stack
    AYAASSERT(lua_gettop(callbackThread) == 0);

    lua_pushfunction(callbackThread, function);

    // Push arguments onto callback's stack
    const int nargs = LuaArguments::pushTuple(*args, callbackThread);

    const ScriptContext::Result result = ScriptContext::getContext(callbackThread.get()).resume(callbackThread, nargs);

    switch (result)
    {
    case ScriptContext::Success:
    {
        // Since the slot didn't yield, we can re-use this thread the next time
        if (createdThread)
            *cachedCallbackThread = WeakThreadRef(callbackThread);

        callAsyncCallbackSuccess(resumeFunction, callbackThread);
        break;
    }

    case ScriptContext::Yield:
    {
        // Don't re-use this thread
        cachedCallbackThread->reset();

        // Schedule continuations
        Lua::Continuations continuations;
        continuations.success = boost::bind(callAsyncCallbackSuccess, resumeFunction, _1);
        continuations.error = boost::bind(callAsyncCallbackError, errorFunction, _1);

        AYAASSERT(RobloxExtraSpace::get(callbackThread.get())->continuations.get() == NULL);
        RobloxExtraSpace::get(callbackThread.get())->continuations.reset(new Lua::Continuations(continuations));

        break;
    }

    case ScriptContext::Error:
    {
        // Don't re-use this thread
        cachedCallbackThread->reset();

        callAsyncCallbackError(errorFunction, callbackThread);
        break;
    }
    }
}

} // namespace Lua
} // namespace Aya

static void assignLuaCallback(Aya::Reflection::Callback callback, lua_State* L, int index)
{
    const Aya::Reflection::CallbackDescriptor& desc(callback.getDescriptor());

    if (desc.security != Aya::Security::None)
        Aya::Security::Context::current().requirePermission(desc.security, desc.name.c_str());

    if (desc.isAsync())
    {
        const Aya::Reflection::AsyncCallbackDescriptor& adesc = static_cast<const Aya::Reflection::AsyncCallbackDescriptor&>(desc);

        if (lua_isnil(L, index))
        {
            adesc.clearCallback(callback.getInstance());
            return;
        }

        Lua::WeakFunctionRef function = lua_tofunction(L, index);

        boost::intrusive_ptr<WeakThreadRef> cachedCallbackThread(new WeakThreadRef());
        adesc.setGenericCallbackHelper(callback.getInstance(), boost::bind(callAsyncCallback, function, _1, _2, _3, cachedCallbackThread));
    }
    else
    {
        const Aya::Reflection::SyncCallbackDescriptor& sdesc = static_cast<const Aya::Reflection::SyncCallbackDescriptor&>(desc);

        if (lua_isnil(L, index))
        {
            sdesc.clearCallback(callback.getInstance());
            return;
        }

        Lua::WeakFunctionRef function = lua_tofunction(L, index);

        boost::intrusive_ptr<WeakThreadRef> cachedCallbackThread(new WeakThreadRef());
        sdesc.setGenericCallbackHelper(callback.getInstance(), boost::bind(callCallback, function, _1, cachedCallbackThread));
    }
}

template<>
void Bridge<shared_ptr<Instance>, false>::on_newindex(shared_ptr<Instance>& object, const char* name, lua_State* L)
{
    if (!object)
        throw std::runtime_error("The object has been deleted");

    AYAPROFILER_SCOPE("LuaBridge", "$newindex");
    AYAPROFILER_LABELF("LuaBridge", "%s.%s", object->getDescriptor().name.c_str(), name);

    // TODO: Ensure that this cast is safe!
    Instance* instance = boost::polymorphic_downcast<Instance*>(object.get());

    Aya::Security::Context& securityContext = Aya::Security::Context::current();

    instance->securityCheck(securityContext);

    name = PropertyNameCorrection(object, name, L);

    if (PropertyDescriptor* prop = object->findPropertyDescriptor(name))
    {
        // Special-case the Parent property
        const PropertyDescriptor& desc = *prop;
        if (desc == Instance::propParent)
        {
            if (instance->getRobloxLocked())
                securityContext.requirePermission(Aya::Security::Plugin, desc.name.c_str());

            if (!object->getDescriptor().isScriptCreatable())
                throw Aya::runtime_error("Cannot change Parent of type %s", object->getDescriptor().name.c_str());

            shared_ptr<Instance> parent = ObjectBridge::getInstance(L, 3);
            if (instance->getParent() != parent.get())
            {
                if ((parent && parent->getRobloxLocked()) || (instance->getParent() && instance->getParent()->getRobloxLocked()))
                {
                    // To add or remove a node from a locked parent, it requires permission
                    securityContext.requirePermission(Aya::Security::Plugin, desc.name.c_str());
                }

                // Was causing too much output spamming so it it being removed.
                // if (!instance->canSetParent(parent.get()))
                //	StandardOut::singleton()->printf(MESSAGE_WARNING, "%s should not be a child of %s", instance->getFullName().c_str(),
                // parent->getFullName().c_str());
                instance->setParent2(parent);
            }
        }
        else
        {
            if (instance->getRobloxLocked())
                securityContext.requirePermission(Aya::Security::Plugin, desc.name.c_str());

            assignLuaValue(Property(*prop, instance), L, 3, securityContext);
        }
        return;
    }

    if (CallbackDescriptor* callback = object->findCallbackDescriptor(name))
    {
        if (instance->getRobloxLocked())
            securityContext.requirePermission(Aya::Security::Plugin, callback->name.c_str());

        assignLuaCallback(Callback(*callback, object.get()), L, 3);
        return;
    }

    throw Aya::runtime_error("%s is not a valid member of %s", name, object->getDescriptor().name.c_str());
}

int ObjectBridge::callMemberFunction(lua_State* L)
{
    AYAPROFILER_SCOPE("LuaBridge", "$call");

    // FunctionDescriptor* is at lua_upvalueindex(1)
    AYAASSERT(lua_islightuserdata(L, lua_upvalueindex(1)));

    const Reflection::FunctionDescriptor* desc = reinterpret_cast<const Reflection::FunctionDescriptor*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (desc->security != Aya::Security::None)
        Aya::Security::Context::current().requirePermission(desc->security, desc->name.c_str());

    AYAPROFILER_LABELF("LuaBridge", "%s.%s", desc->owner.name.c_str(), desc->name.c_str());

    // Instance (self) is at arg index 1
    shared_ptr<Instance> instance;
    if (!getPtr(L, 1, instance) || !instance)
        throw Aya::runtime_error("Expected ':' not '.' calling member function %s", desc->name.c_str());

    Aya::Security::Context& securityContext = Aya::Security::Context::current();

    instance->securityCheck(securityContext);

    // only don't call roblox locked objects if they are under core gui (this is a service we control, and roblox locked was designed for this class)
    if (instance->getRobloxLocked() && instance->isDescendantOf(ServiceProvider::find<CoreGuiService>(instance.get())))
        securityContext.requirePermission(Aya::Security::Plugin, desc->name.c_str());

    // Make sure the function is truly a member of the object
    if (!desc->isMemberOf(instance.get()))
        throw Aya::runtime_error("The function %s is not a member of \"%s\"", desc->name.c_str(), instance->getDescriptor().name.c_str());

    if (desc->getKind() == FunctionDescriptor::Kind_Custom)
        return desc->executeCustom(instance.get(), L);

    LuaArguments args(L, 1); // skip the first argument, since it is "self"
    desc->execute(instance.get(), args);

    return LuaArguments::pushReturnValue(args.returnValue, L);
}

// This class holds information from a call to a yieldable function.
// If the function can return the result immediately then it does so.
// Otherwise, it waits for the result and schedules it for a later time.
class YieldFunctionStateObject
    : public boost::enable_shared_from_this<YieldFunctionStateObject>
    , boost::noncopyable
{
protected:
    static void resumeThreadWithException(boost::intrusive_ptr<WeakThreadRef> threadRef, std::string message)
    {
        if (ThreadRef thread = threadRef->lock())
        {
            if (Continuations* continuations = RobloxExtraSpace::get(thread)->continuations.get())
            {
                if (continuations->error)
                {
                    lua_pushstring(thread, message);
                    (continuations->error)(thread);
                    return;
                }
            }

            Aya::ScriptContext* context = RobloxExtraSpace::get(thread)->context();

            if (context)
            {
                lua_pushstring(thread, message);
                context->reportError(thread);
            }
            else
                StandardOut::singleton()->printf(MESSAGE_ERROR, "%s", message.c_str());

            lua_resetstack(thread, 0);
        }
    }

public:
    boost::intrusive_ptr<WeakThreadRef> thread;
    boost::weak_ptr<DataModel> dm;

    shared_ptr<Instance> instance;
    const Reflection::YieldFunctionDescriptor* const desc;

    lua_State* const L;

    long callingThreadId;
    bool isExecuting; // set to true while the execute function is running. Used only in the executing thread. Otherwise ignored

    volatile int immediateResults;

    bool reentrancyCheck; // for debugging

    static const int YIELD = -1;

    YieldFunctionStateObject(const Reflection::YieldFunctionDescriptor* desc, shared_ptr<Instance> instance, lua_State* L)
        : L(L)
        , thread(new WeakThreadRef(L))
        , desc(desc)
        , instance(instance)
        , immediateResults(YIELD)
        , reentrancyCheck(false)
    {
        dm = shared_from(DataModel::get(&ScriptContext::getContext(L)));
    }

    int execute()
    {
        isExecuting = true;
        this->callingThreadId = GetCurrentThreadId();

        LuaArguments args(L, 1); // skip the first argument, since it is "self"
        desc->execute(instance.get(), args, boost::bind(&YieldFunctionStateObject::onReturnResult, shared_from(this), _1),
            boost::bind(&YieldFunctionStateObject::onRaiseException, shared_from(this), _1));

        isExecuting = false;
        instance.reset(); // we're done with this pointer

        if (immediateResults != YIELD)
            return immediateResults; // the return already happened

        // Capture the yield. The callbacks will handle it
        RobloxExtraSpace::get(L)->yieldCaptured = true;

        return lua_yield(L, 0);
    }

private:
    void onReturnResult(Variant value)
    {
        AYAASSERT(!reentrancyCheck);
        reentrancyCheck = true;

        if (isExecuting && callingThreadId == GetCurrentThreadId())
        {
            immediateResults = LuaArguments::pushReturnValue(value, L); // L is safe to use in this thread
            return;
        }

        if (ThreadRef t = thread->lock())
        {
            shared_ptr<Reflection::Tuple> arguments = LuaArguments::convertToReturnValues(value);

            ScriptContext::getContext(t).scheduleResume(t, arguments);
        }
    }

    void onRaiseException(std::string message)
    {
        AYAASSERT(!reentrancyCheck);
        reentrancyCheck = true;

        if (isExecuting && callingThreadId == GetCurrentThreadId())
            // This unwinds the stack back to execute()
            throw std::runtime_error(message);

        // TODO: It would be much better to call something like ScriptContext::scheduleResume
        if (boost::shared_ptr<DataModel> dataModel = dm.lock())
            dataModel->submitTask(boost::bind(&resumeThreadWithException, thread, message), DataModelJob::Write);
    }
};

int ObjectBridge::callMemberYieldFunction(lua_State* L)
{
    AYAPROFILER_SCOPE("LuaBridge", "$call");

    // YieldFunctionDescriptor* is at lua_upvalueindex(1)
    AYAASSERT(lua_islightuserdata(L, lua_upvalueindex(1)));

    const Reflection::YieldFunctionDescriptor* desc =
        reinterpret_cast<const Reflection::YieldFunctionDescriptor*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (desc->security != Aya::Security::None)
        Aya::Security::Context::current().requirePermission(desc->security, desc->name.c_str());

    AYAPROFILER_LABELF("LuaBridge", "%s.%s", desc->owner.name.c_str(), desc->name.c_str());

    // Instance (self) is at arg index 1
    shared_ptr<Instance> instance;
    if (!getPtr(L, 1, instance) || !instance)
        throw Aya::runtime_error("Did you forget a colon? The first argument of member function %s must be an Object", desc->name.c_str());

    Aya::Security::Context& securityContext = Aya::Security::Context::current();

    instance->securityCheck(securityContext);

    if (instance->getRobloxLocked())
        securityContext.requirePermission(Aya::Security::Plugin, desc->name.c_str());

    // Make sure the function is truly a member of the object
    if (!desc->isMemberOf(instance.get()))
        throw Aya::runtime_error("The function %s is not a member of \"%s\"", desc->name.c_str(), instance->getDescriptor().name.c_str());

    // Construct a shared_ptr object to keep track of the thread for resuming it later
    shared_ptr<YieldFunctionStateObject> functionObject(new YieldFunctionStateObject(desc, instance, L));

    return functionObject->execute();
}

class PropertyChangedEventDescriptor : public Reflection::EventDescriptor
{
private:
    mutable std::string propertyName;

public:
    PropertyChangedEventDescriptor(const std::string& propName, Reflection::ClassDescriptor& owner)
        : EventDescriptor(owner, "PropertyChanged", Security::None, Reflection::Descriptor::Attributes())
        , propertyName(propName)
    {
        signature.resultType = &Reflection::Type::singleton<void>();
    }

    virtual Aya::signals::connection connectGeneric(EventSource* source, shared_ptr<Reflection::GenericSlotWrapper> slot) const
    {
        if (Instance* inst = boost::polymorphic_downcast<Instance*>(source))
        {
            shared_ptr<Aya::signal<void()>> sig = inst->getPropertyChangedSignal(propertyName);
            if (sig)
            {
                return sig->connect(boost::bind(&Reflection::GenericSlotWrapper::execute, slot, Reflection::EventArguments()));
            }
        }
        return Aya::signals::connection();
    }

    virtual void fireEvent(EventSource* source, const Reflection::EventArguments& args) const
    {
        // Not used for property changed signals
    }

    virtual void disconnectAll(EventSource* source) const
    {
        // Not implemented - signals manage their own connections
    }
};

int ObjectBridge::getPropertyChangedSignalBridge(lua_State* L)
{
    // Instance (self) is stored as lightuserdata in upvalue
    Instance* instance = reinterpret_cast<Instance*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!instance)
        throw Aya::runtime_error("Invalid instance in GetPropertyChangedSignal");

    // Property name is the first argument
    if (lua_type(L, 1) != LUA_TSTRING)
        throw Aya::runtime_error("GetPropertyChangedSignal expects a string argument");

    const char* propertyName = lua_tostring(L, 1);

    // Security check
    Aya::Security::Context& securityContext = Aya::Security::Context::current();
    instance->securityCheck(securityContext);

    // Verify the property exists
    if (!instance->findPropertyDescriptor(propertyName))
        throw Aya::runtime_error("%s is not a valid property of %s", propertyName, instance->getDescriptor().name.c_str());

    // Create a descriptor for this property-changed signal
    // Note: This is intentionally leaked as these are cached implicitly by Lua's behavior
    PropertyChangedEventDescriptor* desc =
        new PropertyChangedEventDescriptor(propertyName, const_cast<Reflection::ClassDescriptor&>(instance->getDescriptor()));

    // Create an EventInstance and push it to Lua as an RBXScriptSignal
    EventInstance evt = {desc, shared_from(instance)};
    EventBridge::pushNewObject(L, evt);

    return 1;
}

namespace Aya
{
namespace Lua
{
// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
template<>
int Bridge<shared_ptr<Instance>, false>::on_tostring(const shared_ptr<Instance>& object, lua_State* L)
{
    lua_pushstring(L, object->getName());
    return 1;
}
} // namespace Lua
} // namespace Aya
