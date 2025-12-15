local avatarViewService = game:GetService('AvatarViewService')
local ayaService = game:GetService('AyaService')
local runService = game:GetService('RunService')
local userInputService = game:GetService('UserInputService')
userInputService.MouseIconEnabled = false
game:GetService("ScriptContext").ScriptsDisabled = true
game:GetService("ChangeHistoryService"):SetEnabled(false)

local wornAssets = {}

local Settings = UserSettings()
local GameSettings = Settings.GameSettings
local wornAssets = {}
game:SetPlaceID(1818, false)
local plr = game.Players:CreateLocalPlayer(0)
plr:LoadCharacter()

local character = plr.Character

avatarViewService.AssetWorn:Connect(function(assetId, assetCategory)
    if wornAssets[assetId] then
        for _, child in pairs(character:GetChildren()) do
            local assetIntValue = child:FindFirstChild("AssetId")
            if assetIntValue and assetIntValue:IsA("IntValue") and assetIntValue.Value == assetId then
                child:Destroy()
                break
            end
        end
        wornAssets[assetId] = nil
    else
        print('lua assetid: ' .. assetId)

        local model = game:GetObjects("ayaassetid://" .. assetId)[1]
        
        if assetCategory == "face" then
            for _, child in ipairs(character.Head:GetChildren()) do
                if child:IsA("Decal") or child:IsA("Texture") then
                    child:Destroy()
                end
            end
            model.Parent = character.Head
        else
            model.Parent = character
        end
        
        local assetIntValue = Instance.new("IntValue")
        assetIntValue.Name = "AssetId"
        assetIntValue.Value = assetId
        assetIntValue.Parent = model

        wornAssets[assetId] = model
    end
end)

local character = plr.Character
local rootPart = character:WaitForChild("HumanoidRootPart")
local mouse = game:GetService('Players').LocalPlayer:GetMouse()
local isRotating = false
local isPanning = false
local lastMousePos = Vector2.new()
local rotationSpeed = 0.8
local panSpeed = 0.02
local currentOrbitAngles = Vector2.new(0, 0) -- (vertical, horizontal)
local currentPanOffset = Vector3.new(0, 0, 0)
local minVerticalAngle = math.rad(-180)
local maxVerticalAngle = math.rad(180)

local initialPos = CFrame.new(0, 3, 0)
local savedCamera = workspace.CurrentCamera.CFrame
rootPart.CFrame = initialPos
local basePosition = initialPos.p

-- body color begin

local function getHatsToIgnore(character)
    local ignoreList = {}
    for _, item in pairs(character:GetChildren()) do
        if item:IsA("Accessory") then 
        table.insert(ignoreList, item)
            print(item.Name)
        end
    end
    if game.Workspace:FindFirstChild("FocusPart") then
       table.insert(ignoreList, game.Workspace.FocusPart)
    end
    return ignoreList
end

local playerGui = game.Players.LocalPlayer.PlayerGui
local character = game.Players.LocalPlayer.Character

local screenGui = Instance.new("ScreenGui")
screenGui.Name = "ColorPickerBucket"
screenGui.Parent = playerGui

local selectedButtonTable = {}
local button = Instance.new("ImageButton")
button.Parent = screenGui
button.Image = "ayaasset://textures/paint.png"
button.Size = UDim2.new(0, 21, 0, 21);
button.Position = UDim2.new(0.94, -14, 0.02, 0);

button.BackgroundColor3 = Color3.new(1, 1, 1);
button.BorderColor3 = Color3.new(0, 0, 0);

button.MouseEnter:Connect(function() 
    button.BackgroundColor3 = Color3.new(0.431373, 0.6, 0.788235);
end)

button.MouseLeave:Connect(function() 
    button.BackgroundColor3 = Color3.new(1, 1, 1);
end)

local selectionBox
local selectionLasso
local inGui = false
local inPalette = false

local selectedButtonTable = {}
local currentColor = BrickColor.new(1)
local oldButton = nil

local player = game.Players.LocalPlayer

function onMouseLeave(hoverSelection)
    if oldButton ~= nil then
        local notSelected = true
        local selectionText = ""
        for key, value in pairs(selectedButtonTable) do
            if oldButton == value then
                notSelected = false
            else
                selectionText = value.BackgroundColor.Name
            end
        end
        if notSelected then
            hoverSelection.Text = selectionText
            oldButton.Parent.BackgroundColor = BrickColor.Black()
        end
    end
    oldButton = nil
end

function onMouseEnter(hoverSelection, guiButton)
    onMouseLeave(hoverSelection)
    hoverSelection.Text = guiButton.BackgroundColor.Name
    if guiButton ~= selectedButton then
        guiButton.Parent.BackgroundColor = BrickColor.White()
        oldButton = guiButton
    end
end

function onMouseUp(colorHolder, paletteFrame, guiButton)
    if selectedButtonTable[colorHolder] ~= nil then
        selectedButtonTable[colorHolder].Parent.BackgroundColor = BrickColor.Black()
    end
    guiButton.Parent.BackgroundColor = BrickColor.Yellow()
    colorHolder.BackgroundColor = guiButton.BackgroundColor
    selectionBox.Color = guiButton.BackgroundColor
    selectionLasso.Color = guiButton.BackgroundColor
    currentColor = guiButton.BackgroundColor
    selectedButtonTable[colorHolder] = guiButton

    onMouseLeavePalette(paletteFrame)

    print("Selected Color: " .. guiButton.BackgroundColor.Name)
end

function onShowColorDialog(paletteFrame)
    paletteFrame.Visible = true
end

function setSelectionBox(part) 
    unsetSelectionBox()
    selectionBox.Adornee = part
    selectionLasso.Part = part
end

function unsetSelectionBox() 
    selectionBox.Adornee = nil
    selectionLasso.Part = nil
end

function onMouseEnterGui(mouse)
    unsetSelectionBox()
    inGui = true
end

function onMouseLeaveGui(mouse)
    inGui = false
end

function onMouseEnterPalette(mouse)
    unsetSelectionBox()
    inPalette = true
end

function onMouseLeavePalette(paletteFrame, mouse)
    paletteFrame.Visible = false
    inPalette = false
end

local primaryColor = nil
function buildGui(root, mouse)
    local mainFrame = Instance.new("Frame")
    mainFrame.Name = "MainFrame"
    mainFrame.Position = UDim2.new(0.0, 0, 1.0, -248)
    mainFrame.Size = UDim2.new(0.0, 200, 0.0, 250)
    mainFrame.Transparency = 1.0
    mainFrame.BackgroundColor = BrickColor.White()
    mainFrame.Parent = root

    local paletteFrame = Instance.new("Frame")
    paletteFrame.Position = UDim2.new(0.0, 0, 0.0, -50)
    paletteFrame.Size = UDim2.new(1.0, 0, 4.0/5, 0)
    paletteFrame.BackgroundColor = BrickColor.White()
    paletteFrame.Visible = false
    paletteFrame.Parent = mainFrame
    paletteFrame.MouseEnter:Connect(function() print("EnterPalette") onMouseEnterPalette(mouse) end)
    paletteFrame.MouseLeave:Connect(function() print("LeavePalette")  onMouseLeavePalette(paletteFrame, mouse) end)

    local sideBar = Instance.new("Frame")
    sideBar.Position = UDim2.new(0.0, 0, 4.0/5, 0)
    sideBar.Size = UDim2.new(1.0, 0, 1.0/5, 0)
    sideBar.BackgroundColor = BrickColor.White()
    sideBar.Parent = mainFrame
    sideBar.MouseEnter:Connect(function() onMouseEnterGui(mouse) end)
    sideBar.MouseLeave:Connect(function() onMouseLeaveGui(mouse) end)

    primaryColor = Instance.new("TextButton")
    primaryColor.Position = UDim2.new(0.75, 1, 0.0, 0)
    primaryColor.Size = UDim2.new(0.25, -1, 1.0, 0)
    primaryColor.Text  = ""    
    primaryColor.BackgroundColor = BrickColor.White()
    primaryColor.BorderColor = BrickColor.Black()
    primaryColor.BorderSizePixel = 1
    primaryColor.Parent = sideBar
    primaryColor.MouseButton1Down:Connect(function() print("Showdialog") onShowColorDialog(paletteFrame) end)

    local hoverSelection = Instance.new("TextLabel")
    hoverSelection.Position = UDim2.new(0.0, 0, 0.0, 0)
    hoverSelection.Size = UDim2.new(0.75, 0, 1.0, 0)
    hoverSelection.BackgroundColor = BrickColor.White()
    hoverSelection.TextColor = BrickColor.Black()
    hoverSelection.TextSize = 16
    hoverSelection.Text = primaryColor.BackgroundColor.Name
    hoverSelection.Parent = sideBar

    for xOffset = 0, 9 do
        for yOffset = 0,9 do
            local guiFrame = Instance.new("Frame")
            guiFrame.Position = UDim2.new(1.0/8 * xOffset, 0, 1.0/8*yOffset, 0)
            guiFrame.Size = UDim2.new(1.0/8, 0, 1.0/8, 0)
            guiFrame.BackgroundColor = BrickColor.Black()
            guiFrame.BorderSizePixel = 0
            guiFrame.Parent = paletteFrame

            local guiButton = Instance.new("TextButton")
            guiButton.Position = UDim2.new(0.0, 2, 0.0, 2)
            guiButton.Size = UDim2.new(1.0, -4, 1.0, -4)
            guiButton.Text = ""
            guiButton.BorderSizePixel = 0
            guiButton.AutoButtonColor = false
            guiButton.BackgroundColor = BrickColor.palette(xOffset + yOffset*8)
            guiButton.MouseEnter:Connect(function() onMouseEnter(hoverSelection, guiButton) end)
            guiButton.MouseButton1Up:Connect(function() onMouseUp(primaryColor, paletteFrame, guiButton) end)
            guiButton.Parent = guiFrame

            if guiButton.BackgroundColor == primaryColor.BackgroundColor then
                guiFrame.BackgroundColor = BrickColor.White()
                selectedButtonTable[primaryColor] = guiButton
            end
        end
    end
    mainFrame.MouseLeave:Connect(function() onMouseLeave(hoverSelection) end)
end

function onPlayerSpawn()

end

player.CharacterAdded:Connect(onPlayerSpawn)

if player.Character then
    onPlayerSpawn()
end

local HttpService = game:GetService("HttpService") 

local function getBodyPartsAsJSON(character)
    local bodyParts = {}

    for _, part in pairs(character:GetChildren()) do
        if part:IsA("BasePart") and part.Name ~= "HumanoidRootPart" then
            local bodyPartInfo = {
                name = part.Name,
                brickColor = part.BrickColor.Number
            }
            table.insert(bodyParts, bodyPartInfo)
        end
    end

    local jsonBodyParts = HttpService:JSONEncode(bodyParts)
    return jsonBodyParts
end

local isShowing = false
local camera = workspace.CurrentCamera

mouse.Button1Down:Connect(function()
    if isShowing then
        local mousePosition = mouse.Hit.p 

        local origin = camera.CFrame.p 

        local direction = (mousePosition - origin).unit * 500 
        local ray = Ray.new(origin, direction)

        local ignoreList = getHatsToIgnore(player.Character)

        local part, position = workspace:FindPartOnRayWithIgnoreList(ray, ignoreList)
        
        if part and part:IsDescendantOf(player.Character) then
            print("Hit part: " .. part.Name)
            part.BrickColor = currentColor
            print(getBodyPartsAsJSON(character))
            avatarViewService:SetColor(getBodyPartsAsJSON(character)) -- : (
        else
            print("No valid body part hit")
        end
    end
end)

local function applyBodyColorsToCharacter(character, jsonString)
    local bodyPartsColors = HttpService:JSONDecode(jsonString)

    for _, bodyPartData in pairs(bodyPartsColors) do
        local bodyPartName = bodyPartData.name
        local brickColorValue = bodyPartData.brickColor

        local bodyPart = character:FindFirstChild(bodyPartName)
        if bodyPart and bodyPart:IsA("BasePart") then
            bodyPart.BrickColor = BrickColor.new(brickColorValue)
            print("Applied color to " .. bodyPartName .. ": " .. tostring(brickColorValue))
        else
            print("Body part not found: " .. bodyPartName)
        end
    end
end

button.MouseButton1Down:Connect(function() 
    if isShowing == false then
        local mouse = player:GetMouse()

        local guiMain = Instance.new("ScreenGui")
        guiMain.Parent = player:WaitForChild("PlayerGui") 
    
        inGui = false
        inPalette = false
    
        buildGui(guiMain, mouse)
    
        selectionBox = Instance.new("SelectionBox")
        selectionBox.Adornee = nil  
        selectionBox.Parent = player.PlayerGui
    
        selectionLasso = Instance.new("SelectionPartLasso")
        selectionLasso.Name = "Model Selection Lasso"
        selectionLasso.Humanoid = player.Character:WaitForChild("Humanoid")
        selectionLasso.Part = nil 
        selectionLasso.Visible = true
        selectionLasso.Archivable = false
        selectionLasso.Parent = game.Workspace 
    
        selectionBox.Color = primaryColor.BackgroundColor  
        selectionLasso.Color = primaryColor.BackgroundColor 
        isShowing = true

    else
        -- MainFrame
        for _, item in ipairs(player.PlayerGui:GetChildren()) do
            print(item.Name)
        end
        player.PlayerGui:WaitForChild("ScreenGui"):Destroy()
        print("asdkasojkdjioasdjoisdjoai")
        isShowing = false
    end
end)


-- body color end

local function clamp(value, min, max)
    return math.min(math.max(value, min), max)
end

mouse.Button1Down:connect(function()
    isRotating = true
    lastMousePos = Vector2.new(mouse.X, mouse.Y)
end)

mouse.Button1Up:connect(function()
    isRotating = false
end)

mouse.Button2Down:connect(function()
    isPanning = true
    lastMousePos = Vector2.new(mouse.X, mouse.Y)
end)

mouse.Button2Up:connect(function()
    isPanning = false
end)

runService:BindToRenderStep("CharacterViewer", Enum.RenderPriority.Camera.Value + 1, function()
    local mouseDelta = Vector2.new(mouse.X, mouse.Y) - lastMousePos
    
    if isRotating then
        currentOrbitAngles = Vector2.new(
            clamp(
                currentOrbitAngles.X - mouseDelta.Y * rotationSpeed * math.pi/180,
                minVerticalAngle,
                maxVerticalAngle
            ),
            currentOrbitAngles.Y - mouseDelta.X * rotationSpeed * math.pi/180
        )
    elseif isPanning then
        local rightVector = Vector3.new(
            math.cos(currentOrbitAngles.Y - math.pi/2),
            0,
            math.sin(currentOrbitAngles.Y - math.pi/2)
        )
        local upVector = Vector3.new(0, 1, 0)
        
        currentPanOffset = currentPanOffset + 
            rightVector * (-mouseDelta.X * panSpeed) +
            upVector * (-mouseDelta.Y * panSpeed)
    end
    
    if isRotating or isPanning then
        local targetPos = basePosition + currentPanOffset
        
        local finalTransform = CFrame.new(targetPos) *
                              CFrame.Angles(0, currentOrbitAngles.Y, 0) *
                              CFrame.Angles(currentOrbitAngles.X, 0, 0)
        
        rootPart.CFrame = finalTransform
        
        lastMousePos = Vector2.new(mouse.X, mouse.Y)
    end
end)

local function resetView()
    currentOrbitAngles = Vector2.new(0, 0)
    currentPanOffset = Vector3.new(0, 0, 0)
    rootPart.CFrame = initialPos
    workspace.CurrentCamera.CFrame = savedCamera
end

userInputService.InputBegan:Connect(function(input)
    if input.KeyCode == Enum.KeyCode.R then
        resetView()
    end
end)

-- Clean up function
local function cleanup()
    runService:UnbindFromRenderStep("CharacterViewer")
end

game.Close:connect(cleanup)

Spawn(function()
    wait(0.01)
    savedCamera = workspace.CurrentCamera.CFrame
end)