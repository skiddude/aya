#pragma once

#include "boost.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include <string>
#include <boost/unordered_map.hpp>

namespace Aya
{
namespace Graphics
{

class VertexShader;
class FragmentShader;
class ShaderProgram;
class VisualEngine;

class ShaderManager
{
public:
    ShaderManager(VisualEngine* visualEngine);
    ~ShaderManager();

    void loadShaders(const std::string& language, bool consoleOutput, bool reloadShaders);
    void toggleClassicRendering(bool on);

    std::string getClosestVersionSwapKey(Aya::GameBasicSettings::VirtualVersion currentVersion,
        const std::vector<std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>>& swaps);
    shared_ptr<ShaderProgram> getProgram(const std::string& vsName, const std::string& fsName);

    shared_ptr<ShaderProgram> getProgramOrFFP(const std::string& vsName, const std::string& fsName);
    shared_ptr<ShaderProgram> getProgramFFP();

private:
    VisualEngine* visualEngine;
    bool classicRendering;

    typedef boost::unordered_map<std::string, shared_ptr<VertexShader>> VertexShaders;
    VertexShaders vertexShaders;

    typedef boost::unordered_map<std::string, shared_ptr<FragmentShader>> FragmentShaders;
    FragmentShaders fragmentShaders;

    typedef boost::unordered_map<std::string, shared_ptr<ShaderProgram>> ShaderPrograms;
    ShaderPrograms shaderPrograms;

    shared_ptr<ShaderProgram> shaderProgramFFP;

    shared_ptr<ShaderProgram> createProgram(const std::string& name, const std::string& vsName, const std::string& fsName);
};

} // namespace Graphics
} // namespace Aya