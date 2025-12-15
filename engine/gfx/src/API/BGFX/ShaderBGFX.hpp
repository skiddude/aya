#pragma once

#include "Core/Shader.hpp"
#include <bgfx/bgfx.h>
#include <vector>
#include <map>

namespace Aya
{
namespace Graphics
{

class DeviceBGFX;

class VertexShaderBGFX : public VertexShader
{
public:
    VertexShaderBGFX(Device* device, const std::vector<char>& bytecode);
    ~VertexShaderBGFX();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

    bgfx::ShaderHandle getHandle() const
    {
        return handle;
    }

private:
    bgfx::ShaderHandle handle;
};

class FragmentShaderBGFX : public FragmentShader
{
public:
    FragmentShaderBGFX(Device* device, const std::vector<char>& bytecode);
    ~FragmentShaderBGFX();

    virtual void reloadBytecode(const std::vector<char>& bytecode);

    bgfx::ShaderHandle getHandle() const
    {
        return handle;
    }

private:
    bgfx::ShaderHandle handle;
};

class ShaderProgramBGFX : public ShaderProgram
{
public:
    ShaderProgramBGFX(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
    ~ShaderProgramBGFX();

    virtual int getConstantHandle(const char* name) const;

    virtual unsigned int getMaxWorldTransforms() const;
    virtual unsigned int getSamplerMask() const;

    void updateGlobalConstants(const void* globalData, unsigned int globalVersion);
    void setWorldTransforms4x3(const float* data, size_t matrixCount);
    void setConstant(int handle, const float* data, size_t vectorCount);

    bgfx::ProgramHandle getHandle() const
    {
        return handle;
    }

private:
    bgfx::ProgramHandle handle;

    struct UniformInfo
    {
        bgfx::UniformHandle handle;
        bgfx::UniformType::Enum type;
        unsigned int size;
    };

    std::map<std::string, UniformInfo> uniforms;
    std::map<int, std::string> handleToName;

    bgfx::UniformHandle worldMatrixUniform;
    bgfx::UniformHandle worldMatrixArrayUniform;

    unsigned int cachedGlobalVersion;
    unsigned int maxWorldTransforms;
    unsigned int samplerMask;

    int nextConstantHandle;
};

} // namespace Graphics
} // namespace Aya
