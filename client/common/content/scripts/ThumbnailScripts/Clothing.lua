
local assetUrl, fileExtension, x, y, baseUrl, mannequinId = ...

local ThumbnailGenerator = game:GetService("ThumbnailGenerator")
game:GetService("ScriptContext").ScriptsDisabled = true

local mannequin = game:GetObjects("ayaassetid://" .. tostring(mannequinId))[1]
mannequin.Humanoid.DisplayDistanceType = Enum.HumanoidDisplayDistanceType.None
mannequin.Parent = workspace

local clothing = game:GetObjects(assetUrl)[1]
clothing.Parent = mannequin

return ThumbnailGenerator:Click(fileExtension, x, y, --[[hideSky = ]] true)