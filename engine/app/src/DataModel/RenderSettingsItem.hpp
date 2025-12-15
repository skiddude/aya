#pragma once

// standard C/C++ Headers
#include <vector>

// Roblox Headers
#include "DataModel/GlobalSettings.hpp"
#include "Base/RenderSettings.hpp"
#include "Reflection/Reflection.hpp"


extern const char* const sRenderSettings;
class CRenderSettingsItem
    : public Aya::GlobalAdvancedSettingsItem<CRenderSettingsItem, sRenderSettings>
    , public Aya::CRenderSettings
{
    std::string profileName;
    G3D::Vector2int16 currentDisplaySize;
    std::vector<G3D::Vector2int16> fullScreenSizes;

public:
    void setShowAggregation(bool value);

    static Aya::Reflection::BoundProp<std::string> prop_profileName;

    CRenderSettingsItem();

    void runProfiler(bool overwriteExistingValues);

    void setAASamples(AASamples value);

    void setFullscreenSize(G3D::Vector2int16 value);
    void setWindowSize(G3D::Vector2int16 value);

    bool isSynchronizedWithPhysics;

    void setGraphicsMode(GraphicsMode value);

    void setFrameRateManagerMode(FrameRateManagerMode value);
    void setAntialiasingMode(AntialiasingMode value);
    void setQualityLevel(QualityLevel value);
    void setEditQualityLevel(QualityLevel value);
    void setAutoQualityLevel(int level);
    void setResolutionPreference(ResolutionPreset value);

    void setMinCullDistance(int value);
    void setDebugShowBoundingBoxes(bool value);
    void setEagerBulkExecution(bool value);

    void setEnableFRM(bool value);

    void setTextureCacheSize(unsigned int size);
    void setMeshCacheSize(unsigned int size);

    bool getDebugDisableInterpolation() const;
    void setDebugDisableInterpolation(bool value);

    bool getShowInterpolationPath() const;
    void setShowInterpolationPath(bool value);

    bool getDebugReloadAssets() const;
    void setDebugReloadAssets(bool value);

    bool getObjExportMergeByMaterial() const;
    void setObjExportMergeByMaterial(bool value);

    static Aya::Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::ResolutionPreset> prop_resolution;

    // special signal used only in native code
    Aya::signal<void(const Aya::Reflection::PropertyDescriptor*)> settingsChangedSignal;
};
