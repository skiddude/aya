local characterAppearanceUrl, fileExtension, x, y = ...

local ThumbnailGenerator = game:GetService("ThumbnailGenerator")
game:GetService("ScriptContext").ScriptsDisabled = true

local player = game:GetService("Players"):CreateLocalPlayer(0)
player.CharacterAppearance = characterAppearanceUrl
player:LoadCharacter()

-- Raise up the character's arm if they have gear.
if player.Character then
    player.Character.Humanoid.DisplayDistanceType = Enum.HumanoidDisplayDistanceType.None
    for _, child in pairs(player.Character:GetChildren()) do
        if child:IsA("Tool") then
            player.Character.Torso["Right Shoulder"].CurrentAngle = math.rad(90)
            break
        end
    end
end

return ThumbnailGenerator:Click(fileExtension, x, y, --[[hideSky = ]] true)