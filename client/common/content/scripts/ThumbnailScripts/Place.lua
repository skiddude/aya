local assetUrl, fileExtension, x, y, universeId = ...

local ThumbnailGenerator = game:GetService("ThumbnailGenerator")

if universeId ~= nil then
    pcall(function() game:SetUniverseId(universeId) end)
end

game:GetService("ScriptContext").ScriptsDisabled = true
game:GetService("StarterGui").ShowDevelopmentGui = false

game:Load(assetUrl)

ThumbnailGenerator:AddProfilingCheckpoint("GameLoaded")

-- Do this after again loading the place file to ensure that these values aren't changed when the place file is loaded.
game:GetService("ScriptContext").ScriptsDisabled = true
game:GetService("StarterGui").ShowDevelopmentGui = false

return ThumbnailGenerator:Click(fileExtension, x, y, --[[hideSky = ]] false)