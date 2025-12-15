#include "ShaderBGFX.hpp"
#include "DeviceBGFX.hpp"
#include "GeometryBGFX.hpp"

#include <bgfx/bgfx.h>

LOGGROUP(Graphics)

namespace Aya
{
namespace Graphics
{

VertexShaderBGFX::VertexShaderBGFX(Device* device, const std::vector<char>& bytecode)
    : VertexShader(device)
{
    const bgfx::Memory* mem = bgfx::copy(bytecode.data(), bytecode.size());
    handle = bgfx::createShader(mem);

    if (!bgfx::isValid(handle))
    {
        throw std::runtime_error("Failed to create vertex shader");
    }
}

VertexShaderBGFX::~VertexShaderBGFX()
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }
}

void VertexShaderBGFX::reloadBytecode(const std::vector<char>& bytecode)
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }

    const bgfx::Memory* mem = bgfx::copy(bytecode.data(), bytecode.size());
    handle = bgfx::createShader(mem);

    if (!bgfx::isValid(handle))
    {
        throw std::runtime_error("Failed to reload vertex shader");
    }
}

FragmentShaderBGFX::FragmentShaderBGFX(Device* device, const std::vector<char>& bytecode)
    : FragmentShader(device)
{
    const bgfx::Memory* mem = bgfx::copy(bytecode.data(), bytecode.size());
    handle = bgfx::createShader(mem);

    if (!bgfx::isValid(handle))
    {
        throw std::runtime_error("Failed to create fragment shader");
    }
}

FragmentShaderBGFX::~FragmentShaderBGFX()
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }
}

void FragmentShaderBGFX::reloadBytecode(const std::vector<char>& bytecode)
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }

    const bgfx::Memory* mem = bgfx::copy(bytecode.data(), bytecode.size());
    handle = bgfx::createShader(mem);

    if (!bgfx::isValid(handle))
    {
        throw std::runtime_error("Failed to reload fragment shader");
    }
}

ShaderProgramBGFX::ShaderProgramBGFX(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
    : ShaderProgram(device, vertexShader, fragmentShader)
    , cachedGlobalVersion(0)
    , maxWorldTransforms(0)
    , samplerMask(0)
    , nextConstantHandle(0)
{
    VertexShaderBGFX* vs = static_cast<VertexShaderBGFX*>(vertexShader.get());
    FragmentShaderBGFX* fs = static_cast<FragmentShaderBGFX*>(fragmentShader.get());

    handle = bgfx::createProgram(vs->getHandle(), fs->getHandle(), false);

    if (!bgfx::isValid(handle))
    {
        throw std::runtime_error("Failed to create shader program");
    }

    // Create uniforms for world matrices
    worldMatrixUniform = bgfx::createUniform("u_worldMatrix", bgfx::UniformType::Mat4);
    worldMatrixArrayUniform = bgfx::createUniform("u_worldMatrixArray", bgfx::UniformType::Vec4);

    if (bgfx::isValid(worldMatrixArrayUniform))
    {
        maxWorldTransforms = 32; // Default maximum, can be adjusted
    }
    else if (bgfx::isValid(worldMatrixUniform))
    {
        maxWorldTransforms = 1;
    }

    // In BGFX, samplers are bound separately, so we'll assume all texture stages could be used
    samplerMask = 0xFFFF;
}

ShaderProgramBGFX::~ShaderProgramBGFX()
{
    if (bgfx::isValid(handle))
    {
        bgfx::destroy(handle);
    }

    if (bgfx::isValid(worldMatrixUniform))
    {
        bgfx::destroy(worldMatrixUniform);
    }

    if (bgfx::isValid(worldMatrixArrayUniform))
    {
        bgfx::destroy(worldMatrixArrayUniform);
    }

    // Destroy all created uniforms
    for (auto& pair : uniforms)
    {
        if (bgfx::isValid(pair.second.handle))
        {
            bgfx::destroy(pair.second.handle);
        }
    }
}

int ShaderProgramBGFX::getConstantHandle(const char* name) const
{
    // Check if we already have this uniform
    for (const auto& pair : handleToName)
    {
        if (pair.second == name)
        {
            return pair.first;
        }
    }

    // Create new handle - we need to make this non-const to modify handleToName
    // In practice, this is fine as we're just caching uniform lookups
    ShaderProgramBGFX* mutableThis = const_cast<ShaderProgramBGFX*>(this);

    // Create uniform on first use
    std::string uniformName = name;
    bgfx::UniformHandle uniformHandle = bgfx::createUniform(uniformName.c_str(), bgfx::UniformType::Vec4);

    if (bgfx::isValid(uniformHandle))
    {
        int handle = mutableThis->nextConstantHandle++;

        UniformInfo info;
        info.handle = uniformHandle;
        info.type = bgfx::UniformType::Vec4;
        info.size = 1;

        mutableThis->uniforms[uniformName] = info;
        mutableThis->handleToName[handle] = uniformName;

        return handle;
    }

    return -1;
}

unsigned int ShaderProgramBGFX::getMaxWorldTransforms() const
{
    return maxWorldTransforms;
}

unsigned int ShaderProgramBGFX::getSamplerMask() const
{
    return samplerMask;
}

void ShaderProgramBGFX::updateGlobalConstants(const void* globalData, unsigned int globalVersion)
{
    if (cachedGlobalVersion == globalVersion)
        return;

    cachedGlobalVersion = globalVersion;

    // Update global constants
    const std::vector<ShaderGlobalConstant>& globalConstants = static_cast<DeviceBGFX*>(device)->getGlobalConstants();

    for (size_t i = 0; i < globalConstants.size(); ++i)
    {
        const ShaderGlobalConstant& gc = globalConstants[i];

        // Check if we have a uniform for this constant
        auto it = uniforms.find(gc.name);
        if (it == uniforms.end())
        {
            // Create uniform on first use
            std::string uniformName = gc.name;
            bgfx::UniformHandle uniformHandle = bgfx::createUniform(uniformName.c_str(), bgfx::UniformType::Vec4);
            if (bgfx::isValid(uniformHandle))
            {
                UniformInfo info;
                info.handle = uniformHandle;
                info.type = bgfx::UniformType::Vec4;
                info.size = 1;

                uniforms[uniformName] = info;
                it = uniforms.find(uniformName);
            }
            else
            {
                continue;
            }
        }

        const float* data = reinterpret_cast<const float*>(static_cast<const char*>(globalData) + gc.offset);
        bgfx::setUniform(it->second.handle, data);
    }
}

void ShaderProgramBGFX::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    if (matrixCount == 0)
        return;

    if (matrixCount == 1 && bgfx::isValid(worldMatrixUniform))
    {
        // Convert 4x3 matrix to 4x4
        float matrix[16];
        matrix[0] = data[0];
        matrix[1] = data[1];
        matrix[2] = data[2];
        matrix[3] = 0.0f;

        matrix[4] = data[4];
        matrix[5] = data[5];
        matrix[6] = data[6];
        matrix[7] = 0.0f;

        matrix[8] = data[8];
        matrix[9] = data[9];
        matrix[10] = data[10];
        matrix[11] = 0.0f;

        matrix[12] = data[12];
        matrix[13] = data[13];
        matrix[14] = data[14];
        matrix[15] = 1.0f;

        bgfx::setUniform(worldMatrixUniform, matrix);
    }
    else if (bgfx::isValid(worldMatrixArrayUniform))
    {
        AYAASSERT(matrixCount <= maxWorldTransforms);

        // Pass array of vec4s (3 per matrix for 4x3)
        bgfx::setUniform(worldMatrixArrayUniform, data, matrixCount * 3);
    }
}

void ShaderProgramBGFX::setConstant(int handle, const float* data, size_t vectorCount)
{
    if (handle < 0)
        return;

    auto it = handleToName.find(handle);
    if (it == handleToName.end())
        return;

    auto uniformIt = uniforms.find(it->second);
    if (uniformIt == uniforms.end())
        return;

    bgfx::setUniform(uniformIt->second.handle, data, vectorCount);
}

} // namespace Graphics
} // namespace Aya
