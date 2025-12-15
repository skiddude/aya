#pragma once

#include "Script/script.hpp"
#include "Script/ModuleScript.hpp"
#include "Script/LuaSourceContainer.hpp"
#include "Utility/ContentId.hpp"

#include "Utility/HttpAsync.hpp"

#include "Utility/ProtectedString.hpp"



class LuaSourceBuffer
{

public:
    static LuaSourceBuffer fromInstance(boost::shared_ptr<Aya::Instance> instance);
    static LuaSourceBuffer fromContentId(const Aya::ContentId& contentId);
    static Aya::HttpFuture getScriptAssetSource(const Aya::ContentId& contentId, int universeId);

    LuaSourceBuffer();

    boost::shared_ptr<Aya::LuaSourceContainer> toInstance() const;

    bool isNamedAsset() const;
    bool isModuleScript() const;
    void reloadLiveScript();

    bool isCodeEmbedded() const;
    bool sourceAvailable() const;
    std::string getScriptText() const;
    void setScriptText(const std::string& newText);
    std::string getScriptName() const;
    std::string getFullName() const;

    bool empty() const;

    bool operator==(const LuaSourceBuffer& other) const;
    bool operator!=(const LuaSourceBuffer& other) const;

    inline size_t hash_value() const
    {
        return boost::hash_value(script) ^ boost::hash_value(moduleScript) ^ boost::hash_value(contentId.toString());
    }

private:
    boost::shared_ptr<Aya::Script> script;
    boost::shared_ptr<Aya::ModuleScript> moduleScript;
    Aya::ContentId contentId;

    explicit LuaSourceBuffer(boost::shared_ptr<Aya::ModuleScript> moduleScript);
    explicit LuaSourceBuffer(boost::shared_ptr<Aya::Script> script);
    explicit LuaSourceBuffer(const Aya::ContentId& contentId);
};

inline size_t hash_value(const LuaSourceBuffer& lsb)
{
    return lsb.hash_value();
}
