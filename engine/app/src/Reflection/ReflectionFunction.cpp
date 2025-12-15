

#include "Reflection/Function.hpp"
#include "Reflection/YieldFunction.hpp"
#include "Reflection/Object.hpp"

using namespace Aya;
using namespace Aya::Reflection;

FunctionDescriptor::FunctionDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes)
    : MemberDescriptor(classDescriptor, name, "Function", attributes, security)
    , kind(Kind_Default)
{
    // TODO: This could be moved up into MemberDescriptor if it were templated...
    classDescriptor.MemberDescriptorContainer<FunctionDescriptor>::declare(this);
}

YieldFunctionDescriptor::YieldFunctionDescriptor(
    ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes)
    : MemberDescriptor(classDescriptor, name, "YieldFunction", attributes, security)
{
    // TODO: This could be moved up into MemberDescriptor if it were templated...
    classDescriptor.MemberDescriptorContainer<YieldFunctionDescriptor>::declare(this);
}
