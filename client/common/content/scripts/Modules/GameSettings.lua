--[[
        Filename: GameSettings.lua
        Written by: jeditkacheff
        Version 1.0
        Description: Takes care of the Game Settings Tab in Settings Menu
--]]

-------------- SERVICES --------------
local CoreGui = game:GetService("CoreGui")
local RobloxGui = CoreGui:WaitForChild("RobloxGui")
local GuiService = game:GetService("GuiService")
local UserInputService = game:GetService("UserInputService")
local PlatformService = nil 
pcall(function() PlatformService = game:GetService("PlatformService") end)
local ContextActionService = game:GetService("ContextActionService")
local Settings = UserSettings()
local GameSettings = Settings.GameSettings
local StarterPlayer = game:GetService("StarterPlayer")
-- from loadingscript.lua
local PLACEID = game.PlaceId

function GetGameName()
    if GameAssetInfo ~= nil then
        if IsConvertMyPlaceNameInXboxAppEnabled() then
            return GetFilteredGameName(GameAssetInfo.Name, self:GetCreatorName())
        else
            return GameAssetInfo.Name
        end
    else
        return ''
    end
end

function GetCreatorName()
    if GameAssetInfo ~= nil then
        return GameAssetInfo.Creator.Name
    else
        return ''
    end
end

-------------- CONSTANTS --------------
local GRAPHICS_QUALITY_LEVELS = 10
local GRAPHICS_QUALITY_TO_INT = {
    ["Enum.SavedQualitySetting.Automatic"] = 0,
    ["Enum.SavedQualitySetting.QualityLevel1"] = 1,
    ["Enum.SavedQualitySetting.QualityLevel2"] = 2,
    ["Enum.SavedQualitySetting.QualityLevel3"] = 3,
    ["Enum.SavedQualitySetting.QualityLevel4"] = 4,
    ["Enum.SavedQualitySetting.QualityLevel5"] = 5,
    ["Enum.SavedQualitySetting.QualityLevel6"] = 6,
    ["Enum.SavedQualitySetting.QualityLevel7"] = 7,
    ["Enum.SavedQualitySetting.QualityLevel8"] = 8,
    ["Enum.SavedQualitySetting.QualityLevel9"] = 9,
    ["Enum.SavedQualitySetting.QualityLevel10"] = 10,
}
local PC_CHANGED_PROPS = {
    DevComputerMovementMode = true,
    DevComputerCameraMode = true,
    DevEnableMouseLock = true,
}
local TOUCH_CHANGED_PROPS = {
    DevTouchMovementMode = true,
    DevTouchCameraMode = true,
}
local CAMERA_MODE_DEFAULT_STRING = UserInputService.TouchEnabled and "Default (Follow)" or "Default (Classic)"

local MOVEMENT_MODE_DEFAULT_STRING = UserInputService.TouchEnabled and "Default (Thumbstick)" or "Default (Keyboard)"
local MOVEMENT_MODE_KEYBOARDMOUSE_STRING = "Keyboard + Mouse"
local MOVEMENT_MODE_CLICKTOMOVE_STRING = UserInputService.TouchEnabled and "Tap to Move" or "Click to Move"

----------- UTILITIES --------------
local utility = require(RobloxGui.Modules.Utility)

------------ Variables -------------------
RobloxGui:WaitForChild("Modules"):WaitForChild("TenFootInterface")
RobloxGui:WaitForChild("Modules"):WaitForChild("SettingsHub")
local isTenFootInterface = require(RobloxGui.Modules.TenFootInterface):IsEnabled()
local PageInstance = nil
local LocalPlayer = game.Players.LocalPlayer
local platform = UserInputService:GetPlatform()
local overscanScreen = nil

----------- CLASS DECLARATION --------------
local function Initialize()
    local settingsPageFactory = require(RobloxGui.Modules.SettingsPageFactory)
    local this = settingsPageFactory:CreateNewPage()

    ----------- FUNCTIONS ---------------
    local function createGraphicsOptions()

        ------------------ Fullscreen Selection GUI Setup ------------------
        local fullScreenInit = 1
        if not GameSettings:InFullScreen() then
            fullScreenInit = 2
        end

        this.FullscreenFrame, 
        this.FullscreenLabel,
        this.FullscreenEnabler = utility:AddNewRow(this, "Fullscreen", "Selector", {"On", "Off"}, fullScreenInit)

        local fullScreenSelectionFrame = this.FullscreenEnabler.SliderFrame and this.FullscreenEnabler.SliderFrame or this.FullscreenEnabler.SelectorFrame

        this.FullscreenEnabler.IndexChanged:connect(function(newIndex)
            GuiService:ToggleFullscreen()
        end)

        
        ------------------ Gfx Enabler Selection GUI Setup ------------------
        this.GraphicsEnablerFrame, 
        this.GraphicsEnablerLabel,
        this.GraphicsQualityEnabler = utility:AddNewRow(this, "Graphics Mode", "Selector", {"Automatic", "Manual"}, 1)

        ------------------ Gfx Slider GUI Setup  ------------------
        this.GraphicsQualityFrame, 
        this.GraphicsQualityLabel,
        this.GraphicsQualitySlider = utility:AddNewRow(this, "Graphics Quality", "Slider", GRAPHICS_QUALITY_LEVELS, 1)
        this.GraphicsQualitySlider:SetMinStep(1)

        ------------------ Discord Rich Presence Selection GUI Setup ------------------
        this.DiscordRichPresenceFrame, 
        this.DiscordRichPresenceLabel,
        this.DiscordRichPresenceEnabler = utility:AddNewRow(this, "Discord Rich Presence", "Selector", {"On", "Off"}, GameSettings.DiscordRichPresenceEnabled and 1 or 2)


        ------------------ Micro Profiler Selection GUI Setup ------------------
        this.MicroProfilerFrame, 
        this.MicroProfilerLabel,
        this.MicroProfilerEnabler = utility:AddNewRow(this, "Micro Profiler", "Selector", {"On", "Off"}, GameSettings.MicroProfilerEnabled and 1 or 2)

        this.MicroProfilerEnabler.IndexChanged:connect(function(newIndex)
            GameSettings.MicroProfilerEnabled = newIndex == 1
        end)
        ------------------ Max Framerate Selection GUI Setup ------------------
        local maxFramerateEnumItems = Enum.MaxFramerate:GetEnumItems()
        local startingFrameRateEnumItem = 2

        for i = 1, #maxFramerateEnumItems do
            if GameSettings.MaxFramerate == maxFramerateEnumItems[i] then
                startingFrameRateEnumItem = i
            end
        end

        this.MaxFramerateFrame,
        this.MaxFramerateLabel,
        this.MaxFramerateEnabler = utility:AddNewRow(this, "Max Framerate", "Selector", {"30 FPS", "60 FPS", "75 FPS", "120 FPS", "144 FPS", "200 FPS", "240 FPS", "360 FPS", "Uncapped"}, startingFrameRateEnumItem)
        ------------------ Virtual Version Selection GUI Setup ------------------
        local virtualVersionEnumItems = Enum.VirtualVersion:GetEnumItems()
        local startingVirtualVersionEnumItem = 1
        
        local minVersion = StarterPlayer.MinVirtualVersion
        local maxVersion = StarterPlayer.MaxVirtualVersion
        
        local options = {}
        local filteredEnumItems = {}
        
        for i = 1, #virtualVersionEnumItems do
            local enumItem = virtualVersionEnumItems[i]
            
            if enumItem.Value >= minVersion.Value and enumItem.Value <= maxVersion.Value then
                table.insert(options, enumItem.Name)
                table.insert(filteredEnumItems, enumItem)
                
                if GameSettings.VirtualVersion == enumItem then
                    startingVirtualVersionEnumItem = #options
                end
            end
        end

        virtualVersionEnumItems = filteredEnumItems

        this.VirtualVersionFrame,
        this.VirtualVersionLabel,
        this.VirtualVersionEnabler = utility:AddNewRow(this, "Virtual Version", "DropDown", options, startingVirtualVersionEnumItem)
        
        ------------------ Freaky Mode Selection GUI Setup ------------------
        if settings():GetFFlag("FreakyModeEnabled") and GameSettings.VirtualVersion == Enum.VirtualVersion['2016'] then
            this.FreakyModeFrame, 
            this.FreakyModeLabel,
            this.FreakyModeEnabler = utility:AddNewRow(this, "Freaky Mode", "Selector", {"On", "Off"}, GameSettings.FreakyModeEnabled and 1 or 2)

            this.FreakyModeEnabler.IndexChanged:connect(function(newIndex)
                GameSettings.FreakyModeEnabled = newIndex == 1
            end)

            spawn(function()
                -- when switching virtualversion, this will erorr
                pcall(function () 
                    this.Page["Freaky ModeFrame"]["Freaky ModeLabel"].Font = Enum.Font.LuckiestGuy
                    this.Page["Freaky ModeFrame"]["Freaky ModeLabel"].TextStrokeTransparency = 0.6

                    local hue = 0
                    while wait() do
                        hue = (hue + (0.005))
                        if hue > 1 then
                            hue = hue - 1
                        end

                        pcall(function () 
                            this.Page["Freaky ModeFrame"]["Freaky ModeLabel"].TextColor3 = Color3.fromHSV(hue, 1, 1)
                            this.Page["Freaky ModeFrame"]["Freaky ModeLabel"].TextStrokeColor3 = Color3.fromHSV(hue, 1, 0.6)
                        end)
                    end
                end)
            end)
        end

        ------------------------- Connection Setup ----------------------------
        settings().Rendering.EnableFRM = true

        function SetGraphicsQuality(newValue, automaticSettingAllowed)
            local percentage = newValue/GRAPHICS_QUALITY_LEVELS
            local newQualityLevel = math.floor((settings().Rendering:GetMaxQualityLevel() - 1) * percentage)
            if newQualityLevel == 20 then
                newQualityLevel = 21
            elseif newValue == 1 then
                newQualityLevel = 1
            elseif newValue < 1 and not automaticSettingAllowed then
                newValue = 1
                newQualityLevel = 1
            elseif newQualityLevel > settings().Rendering:GetMaxQualityLevel() then
                newQualityLevel = settings().Rendering:GetMaxQualityLevel() - 1
            end

            GameSettings.SavedQualityLevel = newValue
            settings().Rendering.QualityLevel = newQualityLevel
        end

        local function setGraphicsToAuto()
            this.GraphicsQualitySlider:SetZIndex(1)
            this.GraphicsQualityLabel.ZIndex = 1
            this.GraphicsQualitySlider:SetInteractable(false)

            SetGraphicsQuality(Enum.QualityLevel.Automatic.Value, true)
        end
        local function setGraphicsToManual(level)
            this.GraphicsQualitySlider:SetZIndex(2)
            this.GraphicsQualityLabel.ZIndex = 2
            this.GraphicsQualitySlider:SetInteractable(true)

            -- need to force the quality change if slider is already at this position
            if this.GraphicsQualitySlider:GetValue() == level then
                SetGraphicsQuality(level)
            else
                this.GraphicsQualitySlider:SetValue(level)
            end
        end

        game.GraphicsQualityChangeRequest:connect(function(isIncrease)
            if settings().Rendering.QualityLevel == Enum.QualityLevel.Automatic then return end
            --
            local currentGraphicsSliderValue = this.GraphicsQualitySlider:GetValue()
            if isIncrease then
                currentGraphicsSliderValue = currentGraphicsSliderValue + 1
            else
                currentGraphicsSliderValue = currentGraphicsSliderValue - 1
            end

            this.GraphicsQualitySlider:SetValue(currentGraphicsSliderValue)
        end)
        
        this.GraphicsQualitySlider.ValueChanged:connect(function(newValue)
            SetGraphicsQuality(newValue)
        end)

        this.GraphicsQualityEnabler.IndexChanged:connect(function(newIndex)
            if newIndex == 1 then
                setGraphicsToAuto()
            elseif newIndex == 2 then
                setGraphicsToManual( this.GraphicsQualitySlider:GetValue() )
            end
        end)

        this.DiscordRichPresenceEnabler.IndexChanged:connect(function(newIndex)
            GameSettings.DiscordRichPresenceEnabled = newIndex == 1
        end)

        this.VirtualVersionEnabler.IndexChanged:connect(function(newIndex)
            Spawn(function()
                local function easeInOutQuad(t, b, c, d)
                    t = t / (d / 2)
                    if t < 1 then
                        return c / 2 * t * t + b
                    end
                    t = t - 1
                    return -c / 2 * (t * (t - 2) - 1) + b
                end
            
                local function fadeIn(blurEffect, duration)
                    local startTime = tick()
                    local connection
                    connection = game:GetService("RunService").RenderStepped:Connect(function()
                        local elapsedTime = tick() - startTime
                        if elapsedTime < duration then
                            blurEffect.Size = easeInOutQuad(elapsedTime, 0, 9, duration)
                        else
                            blurEffect.Size = 9
                            connection:disconnect()
                        end
                    end)
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
            
                local blurEffect = Instance.new("BlurEffect")
                blurEffect.Size = 0
                blurEffect.Parent = workspace.CurrentCamera
                blurEffect.Name = "VirtualVersionBlurEffect"
                blurEffect.RobloxLocked = true
            
                fadeIn(blurEffect, 0.2) 
                GameSettings.VirtualVersion = virtualVersionEnumItems[newIndex]
            end)

        end)

        this.MaxFramerateEnabler.IndexChanged:connect(function(newIndex)
            GameSettings.MaxFramerate = maxFramerateEnumItems[newIndex]
        end)

        if GameSettings.SavedQualityLevel == Enum.SavedQualitySetting.Automatic then
            this.GraphicsQualitySlider:SetValue(5)
            this.GraphicsQualityEnabler:SetSelectionIndex(1)
        else
            local graphicsLevel = tostring(GameSettings.SavedQualityLevel)
            if GRAPHICS_QUALITY_TO_INT[graphicsLevel] then
                graphicsLevel = GRAPHICS_QUALITY_TO_INT[graphicsLevel]
            else
                graphicsLevel = GRAPHICS_QUALITY_LEVELS
            end

            spawn(function()
                this.GraphicsQualitySlider:SetValue(graphicsLevel)
                this.GraphicsQualityEnabler:SetSelectionIndex(2)
            end)
        end
    end

    local function createCameraModeOptions(movementModeEnabled)
        ------------------------------------------------------
        ------------------
        ------------------ Shift Lock Switch -----------------
        if UserInputService.MouseEnabled then
            this.ShiftLockFrame, 
            this.ShiftLockLabel,
            this.ShiftLockMode,
            this.ShiftLockOverrideText = nil

            if UserInputService.MouseEnabled and UserInputService.KeyboardEnabled then
                local startIndex = 2
                if GameSettings.ControlMode == Enum.ControlMode.MouseLockSwitch then
                    startIndex = 1
                end

                this.ShiftLockFrame, 
                this.ShiftLockLabel,
                this.ShiftLockMode = utility:AddNewRow(this, "Shift Lock Switch", "Selector", {"On", "Off"}, startIndex)

                this.ShiftLockOverrideText = utility:Create'TextLabel'
                {
                    Name = "ShiftLockOverrideLabel",
                    Text = "Set by Developer",
                    TextColor3 = Color3.new(1,1,1),
                    Font = Enum.Font.SourceSans,
                    FontSize = Enum.FontSize.Size24,
                    BackgroundTransparency = 1,
                    Size = UDim2.new(0,200,1,0),
                    Position = UDim2.new(1,-350,0,0),
                    Visible = false,
                    ZIndex = 2,
                    Parent = this.ShiftLockFrame
                };

                this.ShiftLockMode.IndexChanged:connect(function(newIndex)
                    if newIndex == 1 then
                        GameSettings.ControlMode = Enum.ControlMode.MouseLockSwitch
                    else
                        GameSettings.ControlMode = Enum.ControlMode.Classic
                    end
                end)
            end
        end


        ------------------------------------------------------
        ------------------
        ------------------ Camera Mode -----------------------
        do
            local enumItems = nil
            local startingCameraEnumItem = 1
            if UserInputService.TouchEnabled then
                enumItems = Enum.TouchCameraMovementMode:GetEnumItems()
            else
                enumItems = Enum.ComputerCameraMovementMode:GetEnumItems()
            end

            local cameraEnumNames = {}
            local cameraEnumNameToItem = {}
            for i = 1, #enumItems do
                local displayName = enumItems[i].Name
                if displayName == 'Default' then
                    displayName = CAMERA_MODE_DEFAULT_STRING
                end

                if UserInputService.TouchEnabled then
                    if GameSettings.TouchCameraMovementMode == enumItems[i] then
                        startingCameraEnumItem = i
                    end
                else
                    if GameSettings.ComputerCameraMovementMode == enumItems[i] then
                        startingCameraEnumItem = i
                    end
                end

                cameraEnumNames[i] = displayName
                cameraEnumNameToItem[displayName] = enumItems[i].Value
            end

            this.CameraModeFrame, 
            this.CameraModeLabel,
            this.CameraMode = utility:AddNewRow(this, "Camera Mode", "Selector", cameraEnumNames, startingCameraEnumItem)

            this.CameraModeOverrideText = utility:Create'TextLabel'
            {
                Name = "CameraDevOverrideLabel",
                Text = "Set by Developer",
                TextColor3 = Color3.new(1,1,1),
                Font = Enum.Font.SourceSans,
                FontSize = Enum.FontSize.Size24,
                BackgroundTransparency = 1,
                Size = UDim2.new(0,200,1,0),
                Position = UDim2.new(1,-350,0,0),
                Visible = false,
                ZIndex = 2,
                Parent = this.CameraModeFrame
            };

            this.CameraMode.IndexChanged:connect(function(newIndex)
                local newEnumSetting = cameraEnumNameToItem[cameraEnumNames[newIndex]]

                if UserInputService.TouchEnabled then
                    GameSettings.TouchCameraMovementMode = newEnumSetting
                else
                    GameSettings.ComputerCameraMovementMode = newEnumSetting
                end
            end)
        end

        ------------------------------------------------------
        ------------------
        ------------------ Movement Mode ---------------------
        if movementModeEnabled then
            local movementEnumItems = nil
            local startingMovementEnumItem = 1
            if UserInputService.TouchEnabled then
                movementEnumItems = Enum.TouchMovementMode:GetEnumItems()
            else
                movementEnumItems = Enum.ComputerMovementMode:GetEnumItems()
            end

            local movementEnumNames = {}
            local movementEnumNameToItem = {}
            for i = 1, #movementEnumItems do
                local displayName = movementEnumItems[i].Name
                if displayName == "Default" then
                    displayName = MOVEMENT_MODE_DEFAULT_STRING
                elseif displayName == "KeyboardMouse" then
                    displayName = MOVEMENT_MODE_KEYBOARDMOUSE_STRING
                elseif displayName == "ClickToMove" then
                    displayName = MOVEMENT_MODE_CLICKTOMOVE_STRING
                end

                if UserInputService.TouchEnabled then
                    if GameSettings.TouchMovementMode == movementEnumItems[i] then
                        startingMovementEnumItem = i
                    end
                else
                    if GameSettings.ComputerMovementMode == movementEnumItems[i] then
                        startingMovementEnumItem = i
                    end
                end

                movementEnumNames[i] = displayName
                movementEnumNameToItem[displayName] = movementEnumItems[i]
            end

            this.MovementModeFrame, 
            this.MovementModeLabel,
            this.MovementMode = utility:AddNewRow(this, "Movement Mode", "Selector", movementEnumNames, startingMovementEnumItem)

            this.MovementModeOverrideText = utility:Create'TextLabel'
            {
                Name = "MovementDevOverrideLabel",
                Text = "Set by Developer",
                TextColor3 = Color3.new(1,1,1),
                Font = Enum.Font.SourceSans,
                FontSize = Enum.FontSize.Size24,
                BackgroundTransparency = 1,
                Size = UDim2.new(0,200,1,0),
                Position = UDim2.new(1,-350,0,0),
                Visible = false,
                ZIndex = 2,
                Parent = this.MovementModeFrame
            };

            this.MovementMode.IndexChanged:connect(function(newIndex)
                local newEnumSetting = movementEnumNameToItem[movementEnumNames[newIndex]]
                
                if UserInputService.TouchEnabled then
                    GameSettings.TouchMovementMode = newEnumSetting
                else
                    GameSettings.ComputerMovementMode = newEnumSetting
                end
            end)
        end


        ------------------------------------------------------
        ------------------
        ------------------------- Connection Setup -----------
        function setCameraModeVisible(visible)
            if this.CameraMode then
                this.CameraMode.SelectorFrame.Visible = visible
                this.CameraMode:SetInteractable(visible)
            end
        end

        function setMovementModeVisible(visible)
            if this.MovementMode then
                this.MovementMode.SelectorFrame.Visible = visible
                this.MovementMode:SetInteractable(visible)
            end
        end

        function setShiftLockVisible(visible)
            if this.ShiftLockMode then
                this.ShiftLockMode.SelectorFrame.Visible = visible
                this.ShiftLockMode:SetInteractable(visible)
            end
        end

        do -- initial set of dev vs user choice for guis
            local isUserChoiceCamera = false
            if UserInputService.TouchEnabled then
                isUserChoiceCamera = LocalPlayer.DevTouchCameraMode == Enum.DevTouchCameraMovementMode.UserChoice
            else
                isUserChoiceCamera = LocalPlayer.DevComputerCameraMode == Enum.DevComputerCameraMovementMode.UserChoice
            end

            if not isUserChoiceCamera then
                this.CameraModeOverrideText.Visible = true
                setCameraModeVisible(false)
            else
                this.CameraModeOverrideText.Visible = false
                setCameraModeVisible(true)
            end


            local isUserChoiceMovement = false
            if UserInputService.TouchEnabled then
                isUserChoiceMovement = LocalPlayer.DevTouchMovementMode == Enum.DevTouchMovementMode.UserChoice
            else
                isUserChoiceMovement = LocalPlayer.DevComputerMovementMode == Enum.DevComputerMovementMode.UserChoice
            end

            if this.MovementModeOverrideText then
                if not isUserChoiceMovement then
                    this.MovementModeOverrideText.Visible = true
                    setMovementModeVisible(false)
                else
                    this.MovementModeOverrideText.Visible = false
                    setMovementModeVisible(true)
                end
            end

            if this.ShiftLockOverrideText then
                this.ShiftLockOverrideText.Visible = not LocalPlayer.DevEnableMouseLock
                setShiftLockVisible(LocalPlayer.DevEnableMouseLock)
            end
        end

        local function updateUserSettingsMenu(property)
            if this.ShiftLockOverrideText and property == "DevEnableMouseLock" then
                this.ShiftLockOverrideText.Visible = not LocalPlayer.DevEnableMouseLock
                setShiftLockVisible(LocalPlayer.DevEnableMouseLock)
            elseif property == "DevComputerCameraMode" then
                local isUserChoice = LocalPlayer.DevComputerCameraMode == Enum.DevComputerCameraMovementMode.UserChoice
                setCameraModeVisible(isUserChoice)
                this.CameraModeOverrideText.Visible = not isUserChoice
            elseif property == "DevComputerMovementMode" then
                local isUserChoice = LocalPlayer.DevComputerMovementMode == Enum.DevComputerMovementMode.UserChoice
                setMovementModeVisible(isUserChoice)
                if this.MovementModeOverrideText then
                    this.MovementModeOverrideText.Visible = not isUserChoice
                end
            -- TOUCH
            elseif property == "DevTouchMovementMode" then
                local isUserChoice = LocalPlayer.DevTouchMovementMode == Enum.DevTouchMovementMode.UserChoice
                setMovementModeVisible(isUserChoice)
                if this.MovementModeOverrideText then
                    this.MovementModeOverrideText.Visible = not isUserChoice
                end
            elseif property == "DevTouchCameraMode" then
                local isUserChoice = LocalPlayer.DevTouchCameraMode == Enum.DevTouchCameraMovementMode.UserChoice
                setCameraModeVisible(isUserChoice)
                this.CameraModeOverrideText.Visible = not isUserChoice
            end
        end

        LocalPlayer.Changed:connect(function(property)
            if IsTouchClient then
                if TOUCH_CHANGED_PROPS[property] then
                    updateUserSettingsMenu(property)
                end
            else
                if PC_CHANGED_PROPS[property] then
                    updateUserSettingsMenu(property)
                end
            end
        end)
    end

    local function createVolumeOptions()
        local startVolumeLevel = math.floor(GameSettings.MasterVolume * 10)
        this.VolumeFrame, 
        this.VolumeLabel,
        this.VolumeSlider = utility:AddNewRow(this, "Volume", "Slider", 10, startVolumeLevel)

        local volumeSound = Instance.new("Sound", game.CoreGui.RobloxGui:WaitForChild("Sounds"))
        volumeSound.Name = "VolumeChangeSound"
        volumeSound.SoundId = "ayaasset://sounds/uuhhh.mp3"

        this.VolumeSlider.ValueChanged:connect(function(newValue)
            local soundPercent = newValue/10
            volumeSound.Volume = soundPercent
            volumeSound:Play()
            GameSettings.MasterVolume = soundPercent
        end)
    end

    local function createMouseOptions()
        local MouseSteps = 10
        local MinMouseSensitivity = 0.2

        -- equations below map a function to include points (0, 0.2) (5, 1) (10, 4)
        -- where x is the slider position, y is the mouse sensitivity
        local function translateEngineMouseSensitivityToGui(engineSensitivity)
            return math.floor((2.0/3.0) * (math.sqrt(75.0 * engineSensitivity - 11.0) - 2))
        end

        local function translateGuiMouseSensitivityToEngine(guiSensitivity)
            return 0.03 * math.pow(guiSensitivity,2) + (0.08 * guiSensitivity) + MinMouseSensitivity
        end

        local startMouseLevel = translateEngineMouseSensitivityToGui(GameSettings.MouseSensitivity)
        
        this.MouseSensitivityFrame, 
        this.MouseSensitivityLabel,
        this.MouseSensitivitySlider = utility:AddNewRow(this, "Mouse Sensitivity", "Slider", MouseSteps, startMouseLevel)
        this.MouseSensitivitySlider:SetMinStep(1)

        this.MouseSensitivitySlider.ValueChanged:connect(function(newValue)
            GameSettings.MouseSensitivity = translateGuiMouseSensitivityToEngine(newValue)
        end)
    end

    local function createOverscanOption()
        local showOverscanScreen = function()

            if not overscanScreen then
                local createOverscanFunc = require(RobloxGui.Modules.OverscanScreen)
                overscanScreen = createOverscanFunc(RobloxGui)
                overscanScreen:SetStyleForInGame()
            end

            local MenuModule = require(RobloxGui.Modules.SettingsHub)
               MenuModule:SetVisibility(false, true)

               local closedCon = nil
            closedCon = overscanScreen.Closed:connect(function()
                closedCon:disconnect()
                pcall(function() PlatformService.BlurIntensity = 0 end)
                ContextActionService:UnbindCoreAction("RbxStopOverscanMovement")
                MenuModule:SetVisibility(true, true)
            end)

            pcall(function() PlatformService.BlurIntensity = 10 end)

            local noOpFunc = function() end
            ContextActionService:BindCoreAction("RbxStopOverscanMovement", noOpFunc, false,
                                                Enum.UserInputType.Gamepad1, Enum.UserInputType.Gamepad2,
                                                Enum.UserInputType.Gamepad3, Enum.UserInputType.Gamepad4)

            local ScreenManager = require(RobloxGui.Modules.ScreenManager)
            ScreenManager:OpenScreen(overscanScreen)

        end

        local adjustButton, adjustText, setButtonRowRef = utility:MakeStyledButton("AdjustButton", "Adjust", UDim2.new(0,300,1,-20), showOverscanScreen, this)
        adjustText.Font = Enum.Font.SourceSans
        adjustButton.Position = UDim2.new(1,-400,0,12)

        local row = utility:AddNewRowObject(this, "Safe Zone", adjustButton)
        setButtonRowRef(row)
    end

    createCameraModeOptions(not isTenFootInterface and 
                                (UserInputService.TouchEnabled or UserInputService.MouseEnabled or UserInputService.KeyboardEnabled))

    if UserInputService.MouseEnabled then
        createMouseOptions()
    end

    createVolumeOptions()
    createGraphicsOptions()

    if isTenFootInterface then
        createOverscanOption()
    end

    ------ TAB CUSTOMIZATION -------
    this.TabHeader.Name = "GameSettingsTab"

    this.TabHeader.Icon.Image = "ayaasset://textures/ui/Settings/MenuBarIcons/GameSettingsTab.png"
    if utility:IsSmallTouchScreen() then
        this.TabHeader.Icon.Size = UDim2.new(0,34,0,34)
        this.TabHeader.Icon.Position = UDim2.new(this.TabHeader.Icon.Position.X.Scale,this.TabHeader.Icon.Position.X.Offset,0.5,-17)
        this.TabHeader.Size = UDim2.new(0,125,1,0)
    elseif isTenFootInterface then
        this.TabHeader.Icon.Image = "ayaasset://textures/ui/Settings/MenuBarIcons/GameSettingsTab@2x.png"
        this.TabHeader.Icon.Size = UDim2.new(0,90,0,90)
        this.TabHeader.Icon.Position = UDim2.new(0,0,0.5,-43)
        this.TabHeader.Size = UDim2.new(0,280,1,0)
    else
        this.TabHeader.Icon.Size = UDim2.new(0,45,0,45)
        this.TabHeader.Icon.Position = UDim2.new(0,15,0.5,-22)
    end


    this.TabHeader.Icon.Title.Text = "Settings"

    ------ PAGE CUSTOMIZATION -------
    this.Page.ZIndex = 5
    
    return this
end


----------- Page Instantiation --------------

PageInstance = Initialize()
return PageInstance