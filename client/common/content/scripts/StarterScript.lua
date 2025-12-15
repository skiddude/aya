print("hello from starterscript!")

local GameSettings = UserSettings().GameSettings
local CurrentVirtualVersion = GameSettings.VirtualVersion
local ScriptContext = game:GetService("ScriptContext")
local RobloxGui = game:GetService("CoreGui"):WaitForChild("RobloxGui")
local ContextActionService = game:GetService("ContextActionService")
local GuiService = game:GetService("GuiService")
local function clearDialogs()
    for _, obj in pairs(game:GetService("CoreGui"):GetChildren()) do
        if obj:IsA("BillboardGui") then
            print("Clearing old dialogs")
            obj:Destroy()
        end
    end
end

function fireNotification()
    Spawn(function()
        local function easeInOutQuad(t, b, c, d)
            t = t / (d / 2)
            if t < 1 then
                return c / 2 * t * t + b
            end
            t = t - 1
            return -c / 2 * (t * (t - 2) - 1) + b
        end
    
        local function fadeOut(blurEffect, duration)
            local startTime = tick()
            local connection
            connection = game:GetService("RunService").RenderStepped:Connect(function()
                local elapsedTime = tick() - startTime
                if elapsedTime < duration then
                    blurEffect.Size = easeInOutQuad(elapsedTime, 9, -9, duration)
                else
                    blurEffect.Size = 0
                    connection:disconnect()
                    blurEffect:Destroy()
                end
            end)
        end
    
        local blurEffect = workspace.CurrentCamera:FindFirstChild("VirtualVersionBlurEffect")
        if blurEffect then
            fadeOut(blurEffect, 0.3)
        end
    end)
end

local function createCameraControls(parent)
    -- @mdolli 
    -- old 2012 camera control buttons
    
    local cameraTiltDown = Instance.new("ImageButton")
    cameraTiltDown.Name = "CameraTiltDown"
    cameraTiltDown.Size = UDim2.new(0, 25, 0, 25)
    cameraTiltDown.Position = UDim2.new(1, -56, 1, -29)
    cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown.png"
    cameraTiltDown.Parent = parent
    cameraTiltDown.BackgroundTransparency = 1
    cameraTiltDown:SetVerb("CameraTiltDown")
    cameraTiltDown.RobloxLocked = true

    local cameraTiltUp = Instance.new("ImageButton")
    cameraTiltUp.Name = "CameraTiltUp"
    cameraTiltUp.Size = UDim2.new(0, 25, 0, 25)
    cameraTiltUp.Position = UDim2.new(1, -56, 1, -55)
    cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp.png"
    cameraTiltUp.Parent = parent
    cameraTiltUp.BackgroundTransparency = 1
    cameraTiltUp:SetVerb("CameraTiltUp")
    cameraTiltUp.RobloxLocked = true
    
    local cameraZoomIn = Instance.new("ImageButton")
    cameraZoomIn.Name = "CameraZoomIn"
    cameraZoomIn.Size = UDim2.new(0, 25, 0, 25)
    cameraZoomIn.Position = UDim2.new(1, -29, 1, -55)
    cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn.png"
    cameraZoomIn.Parent = parent
    cameraZoomIn.BackgroundTransparency = 1
    cameraZoomIn:SetVerb("CameraZoomIn")
    cameraZoomIn.RobloxLocked = true
    
    local cameraZoomOut = Instance.new("ImageButton")
    cameraZoomOut.Name = "CameraZoomOut"
    cameraZoomOut.Size = UDim2.new(0, 25, 0, 25)
    cameraZoomOut.Position = UDim2.new(1, -29, 1, -29)
    cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut.png"
    cameraZoomOut.Parent = parent
    cameraZoomOut.BackgroundTransparency = 1
    cameraZoomOut:SetVerb("CameraZoomOut")
    cameraZoomOut.RobloxLocked = true
    
    cameraTiltDown.MouseEnter:connect(function()
        cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown_ovr.png"
    end)
    
    cameraTiltDown.MouseLeave:connect(function()
        cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown.png"
    end)
    
    cameraTiltUp.MouseEnter:connect(function()
        cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp_ovr.png"
    end)
    
    cameraTiltUp.MouseLeave:connect(function()
        cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp.png"
    end)
    
    cameraZoomIn.MouseEnter:connect(function()
        cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn_ovr.png"
    end)
    
    cameraZoomIn.MouseLeave:connect(function()
        cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn.png"
    end)
    
    cameraZoomOut.MouseEnter:connect(function()
        cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut_ovr.png"
    end)
    
    cameraZoomOut.MouseLeave:connect(function()
        cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut.png"
    end)
    
end

local function clearCorescripts()
    for _, obj in pairs(RobloxGui:GetChildren()) do
        if obj.Name ~= "Modules" and obj.Name ~= "CoreScripts/2016/DeveloperConsole" then
            obj:Destroy()
        end
    end
end

local function runStarterScript(newVersion)
    if not RobloxGui:FindFirstChild("CoreScripts/2016/DeveloperConsole") then
        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/DeveloperConsole", RobloxGui)
    end

    if newVersion == Enum.VirtualVersion["2016"] then
        GuiService:SetGlobalGuiInset(0, 36, 0, 0)

        if not RobloxGui:FindFirstChild("ControlFrame") then
            clearCorescripts()
            local controlFrame = Instance.new("Frame")
            controlFrame.Name = "ControlFrame"
            controlFrame.Size = UDim2.new(1, 0, 1, 0)
            controlFrame.BackgroundTransparency = 1
            controlFrame.RobloxLocked = true
            controlFrame.Parent = RobloxGui

            local bottomLeftControl = Instance.new("Frame")
            bottomLeftControl.Name = "BottomLeftControl"
            bottomLeftControl.Size = UDim2.new(0, 130, 0, 46)
            bottomLeftControl.Position = UDim2.new(0, 0, 1, -46)
            bottomLeftControl.BackgroundTransparency = 1
            bottomLeftControl.RobloxLocked = true
            bottomLeftControl.Parent = controlFrame

            local bottomRightControl = Instance.new("Frame")
            bottomRightControl.Name = "BottomRightControl"
            bottomRightControl.Size = UDim2.new(0, 180, 0, 41)
            bottomRightControl.Position = UDim2.new(1, -180, 1, -41)
            bottomRightControl.BackgroundTransparency = 1
            bottomRightControl.RobloxLocked = true
            bottomRightControl.Parent = controlFrame

            local topLeftControl = Instance.new("Frame")
            topLeftControl.Name = "TopLeftControl"
            topLeftControl.Size = UDim2.new(0.05, 0, 0.05, 0)
            topLeftControl.BackgroundTransparency = 1
            topLeftControl.RobloxLocked = true
            topLeftControl.Parent = controlFrame
        end

        local soundFolder = Instance.new("Folder")
        soundFolder.Name = "Sounds"
        soundFolder.Parent = RobloxGui
        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/Topbar", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/MainBotChatScript", RobloxGui)

        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/NotificationScript", RobloxGui)

        require(RobloxGui.Modules.Chat)
        require(RobloxGui.Modules.PlayerlistModule)

        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/BubbleChat", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/PurchasePromptScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/HealthScript", RobloxGui)

        require(RobloxGui.Modules.BackpackScript)

        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/VehicleHud", RobloxGui)

        ScriptContext:AddCoreScriptLocal("CoreScripts/2016/GamepadMenu", RobloxGui)
    elseif GameSettings.VirtualVersion == Enum.VirtualVersion["2015"] then
        local controlFrame = Instance.new("Frame")
        controlFrame.Name = "ControlFrame"
        controlFrame.Size = UDim2.new(1, 0, 1, 0)
        controlFrame.BackgroundTransparency = 1
        controlFrame.Parent = RobloxGui

        local topLeftControl = Instance.new("Frame")
        topLeftControl.Name = "TopLeftControl"
        topLeftControl.Size = UDim2.new(0, 100, 0, 50)
        topLeftControl.Position = UDim2.new(0, 0, 0, 0)
        topLeftControl.BackgroundColor3 = Color3.new(1, 0, 0)
        topLeftControl.BackgroundTransparency = 1 
        topLeftControl.Parent = controlFrame
        
        local bottomLeftControl = Instance.new("Frame")
        bottomLeftControl.Name = "BottomLeftControl"
        bottomLeftControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomLeftControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomLeftControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomLeftControl.BackgroundTransparency = 1 
        bottomLeftControl.Parent = controlFrame
        
        local bottomRightControl = Instance.new("Frame")
        bottomRightControl.Name = "BottomRightControl"
        bottomRightControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomRightControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomRightControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomRightControl.BackgroundTransparency = 1 
        bottomRightControl.Parent = controlFrame
        
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/Settings", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/BubbleChat", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/MainBotChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/ChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/PurchasePromptScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/HealthScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/PlayerListScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/BackpackScript", RobloxGui)
    elseif GameSettings.VirtualVersion == Enum.VirtualVersion["2014"] then
        local controlFrame = Instance.new("Frame")
        controlFrame.Name = "ControlFrame"
        controlFrame.Size = UDim2.new(1, 0, 1, 0)
        controlFrame.BackgroundTransparency = 1
        controlFrame.Parent = RobloxGui

        local topLeftControl = Instance.new("Frame")
        topLeftControl.Name = "TopLeftControl"
        topLeftControl.Size = UDim2.new(0, 100, 0, 50)
        topLeftControl.Position = UDim2.new(0, 0, 0, 0)
        topLeftControl.BackgroundColor3 = Color3.new(1, 0, 0)
        topLeftControl.BackgroundTransparency = 1 
        topLeftControl.Parent = controlFrame
        
        local bottomLeftControl = Instance.new("Frame")
        bottomLeftControl.Name = "BottomLeftControl"
        bottomLeftControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomLeftControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomLeftControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomLeftControl.BackgroundTransparency = 1 
        bottomLeftControl.Parent = controlFrame
    
        local bottomRightControl = Instance.new("Frame")
        bottomRightControl.Name = "BottomRightControl"
        bottomRightControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomRightControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomRightControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomRightControl.BackgroundTransparency = 1 
        bottomRightControl.Parent = controlFrame

        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/BackpackBuilder", RobloxGui)

        RobloxGui:WaitForChild("CurrentLoadout")
        RobloxGui:WaitForChild("Backpack")
        local Backpack = RobloxGui.Backpack

        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/BackpackManager", Backpack)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/BackpackGear", Backpack)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/LoadoutScript", RobloxGui.CurrentLoadout)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/BubbleChat", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2015/Settings", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/ToolTip", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/MainBotChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/PopupScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/NotificationScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/ChatScript", RobloxGui)    
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/PurchasePromptScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/HealthScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/PlayerlistScript", RobloxGui)
    elseif GameSettings.VirtualVersion == Enum.VirtualVersion["2013"] then
        local controlFrame = Instance.new("Frame")
        controlFrame.Name = "ControlFrame"
        controlFrame.Size = UDim2.new(1, 0, 1, 0)
        controlFrame.BackgroundTransparency = 1
        controlFrame.Parent = RobloxGui

        local topLeftControl = Instance.new("Frame")
        topLeftControl.Name = "TopLeftControl"
        topLeftControl.Size = UDim2.new(0, 100, 0, 50)
        topLeftControl.Position = UDim2.new(0, 0, 0, 0)
        topLeftControl.BackgroundColor3 = Color3.new(1, 0, 0)
        topLeftControl.BackgroundTransparency = 1 
        topLeftControl.Parent = controlFrame
        
        local bottomLeftControl = Instance.new("Frame")
        bottomLeftControl.Name = "BottomLeftControl"
        bottomLeftControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomLeftControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomLeftControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomLeftControl.BackgroundTransparency = 1 
        bottomLeftControl.Parent = controlFrame
    
        local bottomRightControl = Instance.new("Frame")
        bottomRightControl.Name = "BottomRightControl"
        bottomRightControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomRightControl.Position = UDim2.new(1, -100, 1, -50) 
        bottomRightControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomRightControl.BackgroundTransparency = 1 
        bottomRightControl.Parent = controlFrame

        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/HealthScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/ToolTip", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/MainBotChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/Settings", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/PopupScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/NotificationScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/ChatScript", RobloxGui)    
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/PurchasePromptScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/PlayerListScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/BubbleChat", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/BackpackBuilder", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/BackpackManager", RobloxGui:WaitForChild("Backpack"))
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/LoadoutScript", RobloxGui:WaitForChild("CurrentLoadout"))
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/BackpackResizer", RobloxGui:WaitForChild("Backpack"))
    elseif GameSettings.VirtualVersion == Enum.VirtualVersion["2012"] then
        local controlFrame = Instance.new("Frame")
        controlFrame.Name = "ControlFrame"
        controlFrame.Size = UDim2.new(1, 0, 1, 0)
        controlFrame.BackgroundTransparency = 1
        controlFrame.Parent = RobloxGui

        local topLeftControl = Instance.new("Frame")
        topLeftControl.Name = "TopLeftControl"
        topLeftControl.Size = UDim2.new(0, 100, 0, 50)
        topLeftControl.Position = UDim2.new(0, 0, 0, 0)
        topLeftControl.BackgroundColor3 = Color3.new(1, 0, 0)
        topLeftControl.BackgroundTransparency = 1 
        topLeftControl.Parent = controlFrame
        
        local bottomLeftControl = Instance.new("Frame")
        bottomLeftControl.Name = "BottomLeftControl"
        bottomLeftControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomLeftControl.Position = UDim2.new(0, 0, 1, -50) 
        bottomLeftControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomLeftControl.BackgroundTransparency = 1 
        bottomLeftControl.Parent = controlFrame
    
        local bottomRightControl = Instance.new("Frame")
        bottomRightControl.Name = "BottomRightControl"
        bottomRightControl.Size = UDim2.new(0, 100, 0, 50) 
        bottomRightControl.Position = UDim2.new(1, -100, 1, -50) 
        bottomRightControl.BackgroundColor3 = Color3.new(0, 1, 0)
        bottomRightControl.BackgroundTransparency = 1 
        bottomRightControl.Parent = controlFrame

        local cameraTiltDown = Instance.new("ImageButton")
        cameraTiltDown.Name = "CameraTiltDown"
        cameraTiltDown.Size = UDim2.new(0, 25, 0, 25)
        cameraTiltDown.Position = UDim2.new(1, -56, 1, -29)
        cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown.png"
        cameraTiltDown.Parent = bottomRightControl
        cameraTiltDown.BackgroundTransparency = 1
        cameraTiltDown:SetVerb("CameraTiltDown")
        cameraTiltDown.RobloxLocked = true
    
        local cameraTiltUp = Instance.new("ImageButton")
        cameraTiltUp.Name = "CameraTiltUp"
        cameraTiltUp.Size = UDim2.new(0, 25, 0, 25)
        cameraTiltUp.Position = UDim2.new(1, -56, 1, -55)
        cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp.png"
        cameraTiltUp.Parent = bottomRightControl
        cameraTiltUp.BackgroundTransparency = 1
        cameraTiltUp:SetVerb("CameraTiltUp")
        cameraTiltUp.RobloxLocked = true
        
        local cameraZoomIn = Instance.new("ImageButton")
        cameraZoomIn.Name = "CameraZoomIn"
        cameraZoomIn.Size = UDim2.new(0, 25, 0, 25)
        cameraZoomIn.Position = UDim2.new(1, -29, 1, -55)
        cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn.png"
        cameraZoomIn.Parent = bottomRightControl
        cameraZoomIn.BackgroundTransparency = 1
        cameraZoomIn:SetVerb("CameraZoomIn")
        cameraZoomIn.RobloxLocked = true
        
        local cameraZoomOut = Instance.new("ImageButton")
        cameraZoomOut.Name = "CameraZoomOut"
        cameraZoomOut.Size = UDim2.new(0, 25, 0, 25)
        cameraZoomOut.Position = UDim2.new(1, -29, 1, -29)
        cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut.png"
        cameraZoomOut.Parent = bottomRightControl
        cameraZoomOut.BackgroundTransparency = 1
        cameraZoomOut:SetVerb("CameraZoomOut")
        cameraZoomOut.RobloxLocked = true
        
        cameraTiltDown.MouseEnter:connect(function()
            cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown_ovr.png"
        end)
        
        cameraTiltDown.MouseLeave:connect(function()
            cameraTiltDown.Image = "ayaasset://textures/CameraTiltDown.png"
        end)
        
        cameraTiltUp.MouseEnter:connect(function()
            cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp_ovr.png"
        end)
        
        cameraTiltUp.MouseLeave:connect(function()
            cameraTiltUp.Image = "ayaasset://textures/CameraTiltUp.png"
        end)
        
        cameraZoomIn.MouseEnter:connect(function()
            cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn_ovr.png"
        end)
        
        cameraZoomIn.MouseLeave:connect(function()
            cameraZoomIn.Image = "ayaasset://textures/CameraZoomIn.png"
        end)
        
        cameraZoomOut.MouseEnter:connect(function()
            cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut_ovr.png"
        end)
        
        cameraZoomOut.MouseLeave:connect(function()
            cameraZoomOut.Image = "ayaasset://textures/CameraZoomOut.png"
        end)

        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/HealthScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/ToolTip", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2014/MainBotChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2013/Settings", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/PopupScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/NotificationScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/ChatScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/PurchasePromptScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/PlayerListScript", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/BubbleChat", RobloxGui)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/BackpackBuilder", RobloxGui)

        local Backpack = RobloxGui:WaitForChild("Backpack")
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/BackpackManager", Backpack)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/BackpackGear", Backpack)
        ScriptContext:AddCoreScriptLocal("CoreScripts/2012/LoadoutScript", RobloxGui:WaitForChild("CurrentLoadout"))

        GuiService:SetGlobalGuiInset(0, 0, 0, 0)
    end

    CurrentVirtualVersion = newVersion
end

runStarterScript(CurrentVirtualVersion)

GameSettings.Changed:connect(function(property)
    if property ~= "VirtualVersion" then return end
    if GameSettings.VirtualVersion == CurrentVirtualVersion then return end

    local currentVersion = tostring(CurrentVirtualVersion):gsub("Enum.VirtualVersion.", "")
    local newVersion = tostring(GameSettings.VirtualVersion):gsub("Enum.VirtualVersion.", "")

    print("-- Switching VirtualVersion from " .. currentVersion .. " to " .. newVersion .. " --")

    if CurrentVirtualVersion == Enum.VirtualVersion["2016"] and GameSettings.VirtualVersion ~= Enum.VirtualVersion["2016"] then
        if RobloxGui:FindFirstChild("Modules") then
            for _, module in pairs(RobloxGui.Modules:GetChildren()) do
                module:Destroy()
            end
        end
    end

    clearCorescripts()
    clearDialogs()

    if CurrentVirtualVersion == Enum.VirtualVersion["2016"] then
        ContextActionService:UnbindCoreAction("RbxSettingsHubSwitchTab")
        ContextActionService:UnbindCoreAction("RbxSettingsHubStopCharacter")
        ContextActionService:UnbindCoreAction("RbxSettingsScrollHotkey")
        ContextActionService:UnbindCoreAction("RBXEscapeMainMenu")
    end

    GuiService:SetMenuIsOpen(false)
    GuiService.SelectedCoreObject = nil

    runStarterScript(GameSettings.VirtualVersion)
    fireNotification(GameSettings.VirtualVersion)
end)