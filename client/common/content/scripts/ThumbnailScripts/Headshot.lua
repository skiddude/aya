local characterAppearanceUrl, fileExtension, x, y, quadratic, baseHatZoom, maxHatZoom, cameraOffsetX, cameraOffsetY = ...

local ThumbnailGenerator = game:GetService("ThumbnailGenerator")
game:GetService('ScriptContext').ScriptsDisabled = true

local player = game:GetService("Players"):CreateLocalPlayer(0)
player.CharacterAppearance = characterAppearanceUrl
player:LoadCharacter()

local maxDimension = 0

if player.Character then
    -- Remove gear
    for _, child in pairs(player.Character:GetChildren()) do
        if child:IsA("Tool") then
            child:Destroy()
        elseif child:IsA("Accoutrement") or child:IsA("Hat") then
            local handle = child:FindFirstChild("Handle")
            if handle then
                local attachment = handle:FindFirstChildWhichIsA("Attachment")
                -- legacy hat does not have attachment in it and should be considered when zoom out camera
                if not attachment or headAttachments[attachment.Name] then
                    local size = handle.Size / 2 + handle.Position - player.Character.Head.Position
                    local xy = Vector2.new(size.x, size.y)
                    if xy.magnitude > maxDimension then
                        maxDimension = xy.magnitude
                    end
                end
            end
        end
    end

    -- Setup Camera
    local maxHatOffset = 0.5 -- Maximum amount to move camera upward to accomodate large hats
    maxDimension = math.min(1, maxDimension / 3) -- Confine maxdimension to specific bounds

    if quadratic then
        maxDimension = maxDimension * maxDimension -- Zoom out on quadratic interpolation
    end

    local viewOffset = player.Character.Head.CFrame * CFrame.new(cameraOffsetX, cameraOffsetY + maxHatOffset * maxDimension, 0.1) -- View vector offset from head

    local yAngle = -math.pi / 16
    if FFlagNewHeadshotLighting then
        yAngle = 0 -- Camera is looking straight at avatar's face.
    end
    local positionOffset = player.Character.Head.CFrame + (CFrame.Angles(0, yAngle, 0).lookVector.unit * 3) -- Position vector offset from head

    local camera = Instance.new("Camera", player.Character)
    camera.Name = "ThumbnailCamera"
    camera.CameraType = Enum.CameraType.Scriptable
    camera.CoordinateFrame = CFrame.new(positionOffset.p, viewOffset.p)
    camera.FieldOfView = baseHatZoom + (maxHatZoom - baseHatZoom) * maxDimension
end

local result = ThumbnailGenerator:Click(fileExtension, x, y, --[[hideSky = ]] true)

return result