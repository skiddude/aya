
#include "DataModel/ContentProvider.hpp"
#include "Render/ShaderManager.hpp"

#include "Render/VisualEngine.hpp"

#include "Core/Device.hpp"

#include "time.hpp"

#include "rapidjson/document.h"
#include "Utility/MD5Hasher.hpp"

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "StringConv.hpp"

#include "Utility/StandardOut.hpp"

#include "DataModel/GameBasicSettings.hpp"

LOGGROUP(Graphics)



namespace Aya
{
namespace Graphics
{

std::map<std::string, std::vector<std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>>> swaps;

struct PackEntryFile
{
    char name[64];
    char md5[16];
    unsigned int offset;
    unsigned int size;
    char reserved[8];
};

struct PackData
{
    std::vector<char> data;

    const PackEntryFile* findEntry(const char* name) const
    {
        unsigned int count = *reinterpret_cast<const unsigned int*>(&data[4]);
        const PackEntryFile* entries = reinterpret_cast<const PackEntryFile*>(&data[8]);

        for (unsigned int i = 0; i < count; ++i)
            if (strncmp(name, entries[i].name, sizeof(entries[i].name)) == 0)
                return &entries[i];

        return NULL;
    }
};

static std::string loadFile(const std::string& folder, const std::string& path)
{
    std::ifstream file(utf8_decode(folder + "/" + path).c_str(), std::ios::in | std::ios::binary);

    if (!file)
        throw Aya::runtime_error("Error opening file %s", (folder + "/" + path.c_str()).c_str());

    std::ostringstream data;
    data << file.rdbuf();

    return data.str();
}

static PackData readPack(const std::string& folder, const std::string& path)
{
    std::string data = loadFile(folder, path);

    if (data.compare(0, 4, "RBXS") != 0)
        throw Aya::runtime_error("Error: shader pack %s is corrupted", path.c_str());

    PackData result;
    result.data.assign(data.begin(), data.end());

    return result;
}

static std::string getPackName(const std::string& language)
{
    return language;
}

static bool isExcludedFromPack(const std::string& exclude, const std::string& packName)
{
    if (!exclude.empty())
    {
        std::istringstream iss(exclude);
        std::string item;

        while (iss >> item)
            if (item == packName)
                return true;
    }

    return false;
}

static std::string getStringOrEmpty(const rapidjson::Value& value)
{
    return value.IsString() ? value.GetString() : "";
}

static void computeShaderSignature(char (&sig)[16], const std::string& shaderSource, const std::string& target, const std::string& entrypoint)
{
    scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());

    hasher->addData(shaderSource);
    hasher->addData(target);
    hasher->addData(entrypoint);

    hasher->toBuffer(sig);
}

ShaderManager::ShaderManager(VisualEngine* visualEngine)
    : visualEngine(visualEngine)
{
    this->classicRendering = false;
}

ShaderManager::~ShaderManager() {}

void ShaderManager::toggleClassicRendering(bool on)
{
    this->classicRendering = on;
}

void ShaderManager::loadShaders(const std::string& language, bool consoleOutput, bool reloadShaders)
{
    Device* device = visualEngine->getDevice();

    if (device->getCaps().supportsFFP)
    {
        FASTLOG(FLog::Graphics, "Skipping shader loading since device does not support them");
        return;
    }

    if (reloadShaders)
    {
        shaderPrograms.clear();
    }

    using namespace rapidjson;

    Timer<Time::Precise> timer;

    std::string packName = getPackName(language);
    std::string folder = ContentProvider::shadersFolder();
    PackData shaderPack = readPack(folder, "shaders_" + packName + ".pack");

    std::string shaderDb = loadFile(folder, "shaders.json");

    Document root;
    root.Parse<kParseDefaultFlags>(shaderDb.c_str());

    if (root.HasParseError())
        throw Aya::runtime_error("Failed to load shader.json: %s", root.GetParseError());

    AYAASSERT(root.IsArray());

    for (Value::ValueIterator it = root.Begin(); it != root.End(); ++it)
    {
        const Value& name = (*it)["name"];

        if (!name.IsString())
        {
            FASTLOGS(FLog::Graphics, "Error: failed to parse shader info: %s", getStringOrEmpty(name));
            continue;
        }

        if ((*it).HasMember("exclude") && (*it)["exclude"].IsString())
            if (isExcludedFromPack(getStringOrEmpty((*it)["exclude"]), packName))
                continue;

        std::string _name = std::string(name.GetString());

        if ((*it).HasMember("version") && (*it)["version"].IsNumber() && (*it).HasMember("key") && (*it)["key"].IsString())
        {
            std::string key = (*it)["key"].GetString();
            int version = (*it)["version"].GetInt();

            Aya::GameBasicSettings::VirtualVersion virtualVersion;
            if (version == 2016)
                virtualVersion = Aya::GameBasicSettings::VERSION_2016;
            else if (version == 2015)
                virtualVersion = Aya::GameBasicSettings::VERSION_2015;
            else if (version == 2014)
                virtualVersion = Aya::GameBasicSettings::VERSION_2014;
            else if (version == 2013)
                virtualVersion = Aya::GameBasicSettings::VERSION_2013;
            else if (version == 2012)
                virtualVersion = Aya::GameBasicSettings::VERSION_2012;

            std::string modifiedName = _name;
            size_t keyPosition = modifiedName.find(key);
            if (keyPosition != std::string::npos)
            {
                modifiedName.erase(keyPosition, key.length());
            }

            swaps[modifiedName].push_back(std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>(virtualVersion, key));
        }

        bool isVertex = _name.find("VS") != std::string::npos;

        try
        {
            std::vector<char> shaderBytecode;

            const PackEntryFile* entry = shaderPack.findEntry(_name.c_str());

            if (!entry)
                throw Aya::runtime_error("Error: failed to find shader %s in pack", _name.c_str());

            shaderBytecode.assign(shaderPack.data.begin() + entry->offset, shaderPack.data.begin() + entry->offset + entry->size);

            if (isVertex)
            {
                shared_ptr<VertexShader> shader = device->createVertexShader(shaderBytecode);

                shader->setDebugName(_name);

                vertexShaders[_name] = shader;
            }
            else
            {
                shared_ptr<FragmentShader> shader = device->createFragmentShader(shaderBytecode);

                shader->setDebugName(_name);

                fragmentShaders[_name] = shader;
            }
        }
        catch (const Aya::base_exception& e)
        {
            FASTLOGS(FLog::Graphics, "Error: failed to create shader %s", _name);
            ShaderProgram::dumpToFLog(e.what(), FLog::Graphics);

            {
                std::string text = e.what();

                std::vector<std::string> messages;
                boost::split(messages, text, boost::is_from_range('\n', '\n'));

                while (!messages.empty() && messages.back().empty())
                    messages.pop_back();

                StandardOut::singleton()->printf(MESSAGE_ERROR, "Error: failed to create shader %s : %s", _name.c_str(), e.what());

                for (size_t i = 0; i < messages.size(); ++i)
                    StandardOut::singleton()->print(MESSAGE_ERROR, messages[i]);
            }
        }
    }

    StandardOut::singleton()->printf(MESSAGE_INFO, "Loaded %d vertex shaders and %d fragment shaders in %d ms", vertexShaders.size(),
        fragmentShaders.size(), static_cast<int>(timer.delta().msec()));
}

std::string ShaderManager::getClosestVersionSwapKey(
    Aya::GameBasicSettings::VirtualVersion currentVersion, const std::vector<std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>>& swaps)
{
    auto closest = swaps[0];
    int smallestDifference = std::abs(static_cast<int>(currentVersion) - static_cast<int>(closest.first));

    for (const auto& swap : swaps)
    {
        int difference = std::abs(static_cast<int>(currentVersion) - static_cast<int>(swap.first));
        if (difference <= smallestDifference)
        {
            closest = swap;
            smallestDifference = difference;
        }
    }

    return closest.second;
}

shared_ptr<ShaderProgram> ShaderManager::getProgram(const std::string& vsName, const std::string& fsName)
{
    std::string key;
    std::string correctedVsName = vsName;
    std::string correctedFsName = fsName;
    Aya::GameBasicSettings::VirtualVersion version = Aya::GameBasicSettings::singleton().getVirtualVersion();

    std::vector<std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>> vsVersions;
    std::vector<std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>> fsVersions;

    for (const auto& [shader, _swap] : swaps)
    {
        for (auto swap : _swap)
        {
            if (shader == vsName && version >= swap.first)
            {
                vsVersions.push_back(std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>(swap.first, swap.second));
            }

            if (shader == fsName && version >= swap.first)
            {
                fsVersions.push_back(std::pair<Aya::GameBasicSettings::VirtualVersion, std::string>(swap.first, swap.second));
            }
        }
    }

    if (!vsVersions.empty())
    {
        std::string vsSwapKey = getClosestVersionSwapKey(version, vsVersions);
        correctedVsName = vsSwapKey + vsName;
    }

    if (!fsVersions.empty())
    {
        std::string fsSwapKey = getClosestVersionSwapKey(version, fsVersions);
        correctedFsName = fsSwapKey + fsName;
    }

    key.reserve(vsName.size() + 1 + fsName.size());
    key += correctedVsName;
    key += '*';
    key += correctedFsName;

    ShaderPrograms::iterator it = shaderPrograms.find(key);

    if (it != shaderPrograms.end())
        return it->second;

    // StandardOut::singleton()->printf(MESSAGE_INFO,"Returning new %s", key.c_str());
    return shaderPrograms[key] = createProgram(key, correctedVsName, correctedFsName);
}

shared_ptr<ShaderProgram> ShaderManager::getProgramOrFFP(const std::string& vsName, const std::string& fsName)
{
    shared_ptr<ShaderProgram> result = getProgram(vsName, fsName);

    return result ? result : getProgramFFP();
}

shared_ptr<ShaderProgram> ShaderManager::getProgramFFP()
{
    Device* device = visualEngine->getDevice();

    if (!shaderProgramFFP && device->getCaps().supportsFFP)
        shaderProgramFFP = device->createShaderProgramFFP();

    return shaderProgramFFP;
}

shared_ptr<ShaderProgram> ShaderManager::createProgram(const std::string& name, const std::string& vsName, const std::string& fsName)
{
    VertexShaders::iterator vsit = vertexShaders.find(vsName);

    if (vsit == vertexShaders.end())
        return shared_ptr<ShaderProgram>();

    FragmentShaders::iterator fsit = fragmentShaders.find(fsName);

    if (fsit == fragmentShaders.end())
        return shared_ptr<ShaderProgram>();

    try
    {
        shared_ptr<ShaderProgram> result = visualEngine->getDevice()->createShaderProgram(vsit->second, fsit->second);

        result->setDebugName(name);

        return result;
    }
    catch (const Aya::base_exception& e)
    {
        FASTLOGS(FLog::Graphics, "Error: failed to link shader program %s", vsName + "/" + fsName);
        ShaderProgram::dumpToFLog(e.what(), FLog::Graphics);

        return shared_ptr<ShaderProgram>();
    }
}

} // namespace Graphics
} // namespace Aya