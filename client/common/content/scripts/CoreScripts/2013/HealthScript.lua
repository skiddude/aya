--[[ 
	This script controls the gui the player sees in regards to his or her health.
	Can be turned with Game.StarterGui:SetCoreGuiEnabled(Enum.CoreGuiType.Health,false)
--]]

---------------------------------------------------------------------
-- Initialize/Variables
while not game do
	wait(1/60)
end
while not game:GetService("Players") do
	wait(1/60)
end

local GameSettings = UserSettings().GameSettings

local currentHumanoid = nil

local LegacyHealthGui = nil
local lastHealth = 100
local lastHealth2 = 100
local maxWidth = 0.96

local HealthGui = nil
local lastHealth = 100
local HealthPercentageForOverlay = 5
local maxBarTweenTime = 0.3
local greenColor = Color3.new(0.2, 1, 0.2)
local redColor = Color3.new(1, 0.2, 0.2)
local yellowColor = Color3.new(1, 1, 0.2)

local guiEnabled = false
local healthChangedConnection = nil
local humanoidDiedConnection = nil
local characterAddedConnection = nil

local greenBarImage = "ayaasset://textures/ui/Health-BKG-Center.png"
local greenBarImageLeft = "ayaasset://textures/ui/Health-BKG-Left-Cap.png"
local greenBarImageRight = "ayaasset://textures/ui/Health-BKG-Right-Cap.png"
local hurtOverlayImage = "ayaasset://textures/2015/HealthScript/hurtOverlayImage.png"

game:GetService("ContentProvider"):Preload(greenBarImage)
game:GetService("ContentProvider"):Preload(hurtOverlayImage)

while not game:GetService("Players").LocalPlayer do
	wait(1/60)
end

---------------------------------------------------------------------
-- Functions

local capHeight = 15
local capWidth = 7

function CreateGui()
	if HealthGui and #HealthGui:GetChildren() > 0 then 
		HealthGui.Parent = game:GetService("CoreGui").RobloxGui
		return 
	end

	local hurtOverlay = Instance.new("ImageLabel")
	hurtOverlay.Name = "HurtOverlay"
	hurtOverlay.BackgroundTransparency = 1
	hurtOverlay.Image = hurtOverlayImage
	hurtOverlay.Position = UDim2.new(-10,0,-10,0)
	hurtOverlay.Size = UDim2.new(20,0,20,0)
	hurtOverlay.Visible = false
	hurtOverlay.Parent = HealthGui
	
	local healthFrame = Instance.new("Frame")
	healthFrame.Name = "HealthFrame"
	healthFrame.BackgroundTransparency = 1
	healthFrame.BackgroundColor3 = Color3.new(1,1,1)
	healthFrame.BorderColor3 = Color3.new(0,0,0)
	healthFrame.BorderSizePixel = 0
	healthFrame.Position = UDim2.new(0.5,-85,1,-20)
	healthFrame.Size = UDim2.new(0,170,0,capHeight)
	healthFrame.Parent = HealthGui

	local healthBarBackCenter = Instance.new("ImageLabel")
	healthBarBackCenter.Name = "healthBarBackCenter"
	healthBarBackCenter.BackgroundTransparency = 1
	healthBarBackCenter.Image = greenBarImage
	healthBarBackCenter.Size = UDim2.new(1,-capWidth*2,1,0)
	healthBarBackCenter.Position = UDim2.new(0,capWidth,0,0)
	healthBarBackCenter.Parent = healthFrame
	healthBarBackCenter.ImageColor3 = Color3.new(1,1,1)

	local healthBarBackLeft = Instance.new("ImageLabel")
	healthBarBackLeft.Name = "healthBarBackLeft"
	healthBarBackLeft.BackgroundTransparency = 1
	healthBarBackLeft.Image = greenBarImageLeft
	healthBarBackLeft.Size = UDim2.new(0,capWidth,1,0)
	healthBarBackLeft.Position = UDim2.new(0,0,0,0)
	healthBarBackLeft.Parent = healthFrame
	healthBarBackLeft.ImageColor3 = Color3.new(1,1,1)

	local healthBarBackRight = Instance.new("ImageLabel")
	healthBarBackRight.Name = "healthBarBackRight"
	healthBarBackRight.BackgroundTransparency = 1
	healthBarBackRight.Image = greenBarImageRight
	healthBarBackRight.Size = UDim2.new(0,capWidth,1,0)
	healthBarBackRight.Position = UDim2.new(1,-capWidth,0,0)
	healthBarBackRight.Parent = healthFrame
	healthBarBackRight.ImageColor3 = Color3.new(1,1,1)


	local healthBar = Instance.new("Frame")
	healthBar.Name = "HealthBar"
	healthBar.BackgroundTransparency = 1
	healthBar.BackgroundColor3 = Color3.new(1,1,1)
	healthBar.BorderColor3 = Color3.new(0,0,0)
	healthBar.BorderSizePixel = 0
	healthBar.ClipsDescendants = true
	healthBar.Position = UDim2.new(0, 0, 0, 0)
	healthBar.Size = UDim2.new(1,0,1,0)
	healthBar.Parent = healthFrame


	local healthBarCenter = Instance.new("ImageLabel")
	healthBarCenter.Name = "healthBarCenter"
	healthBarCenter.BackgroundTransparency = 1
	healthBarCenter.Image = greenBarImage
	healthBarCenter.Size = UDim2.new(1,-capWidth*2,1,0)
	healthBarCenter.Position = UDim2.new(0,capWidth,0,0)
	healthBarCenter.Parent = healthBar
	healthBarCenter.ImageColor3 = greenColor

	local healthBarLeft = Instance.new("ImageLabel")
	healthBarLeft.Name = "healthBarLeft"
	healthBarLeft.BackgroundTransparency = 1
	healthBarLeft.Image = greenBarImageLeft
	healthBarLeft.Size = UDim2.new(0,capWidth,1,0)
	healthBarLeft.Position = UDim2.new(0,0,0,0)
	healthBarLeft.Parent = healthBar
	healthBarLeft.ImageColor3 = greenColor

	local healthBarRight = Instance.new("ImageLabel")
	healthBarRight.Name = "healthBarRight"
	healthBarRight.BackgroundTransparency = 1
	healthBarRight.Image = greenBarImageRight
	healthBarRight.Size = UDim2.new(0,capWidth,1,0)
	healthBarRight.Position = UDim2.new(1,-capWidth,0,0)
	healthBarRight.Parent = healthBar
	healthBarRight.ImageColor3 = greenColor

	HealthGui.Parent = game:GetService("CoreGui").RobloxGui
end

function UpdateGui(health)
	if not HealthGui then return end
	
	local healthFrame = HealthGui:FindFirstChild("HealthFrame")
	if not healthFrame then return end
	
	local healthBar = healthFrame:FindFirstChild("HealthBar")
	if not healthBar then return end
	
	-- If more than 1/4 health, bar = green.  Else, bar = red.
	local percentHealth = (health/currentHumanoid.MaxHealth)
	if percentHealth ~= percentHealth then
		percentHealth = 1
		healthBar.healthBarCenter.ImageColor3 = yellowColor
		healthBar.healthBarRight.ImageColor3 = yellowColor
		healthBar.healthBarLeft.ImageColor3 = yellowColor
	elseif percentHealth > 0.25  then		
		healthBar.healthBarCenter.ImageColor3 = greenColor
		healthBar.healthBarRight.ImageColor3 = greenColor
		healthBar.healthBarLeft.ImageColor3 = greenColor
	else
		healthBar.healthBarCenter.ImageColor3 = redColor
		healthBar.healthBarRight.ImageColor3 = redColor
		healthBar.healthBarLeft.ImageColor3 = redColor
	end
		
	local width = (health / currentHumanoid.MaxHealth)
 	width = math.max(math.min(width,1),0) -- make sure width is between 0 and 1
 	if width ~= width then width = 1 end

	local healthDelta = lastHealth - health
	lastHealth = health
	
	local percentOfTotalHealth = math.abs(healthDelta/currentHumanoid.MaxHealth)
	percentOfTotalHealth = math.max(math.min(percentOfTotalHealth,1),0) -- make sure percentOfTotalHealth is between 0 and 1
	if percentOfTotalHealth ~= percentOfTotalHealth then percentOfTotalHealth = 1 end

	local newHealthSize = UDim2.new(width,0,1,0)
	
	healthBar.Size = newHealthSize

	local sizeX = healthBar.AbsoluteSize.X
	if sizeX < capWidth then
		healthBar.healthBarCenter.Visible = false
		healthBar.healthBarRight.Visible = false
	elseif sizeX < (2*capWidth + 1) then
		healthBar.healthBarCenter.Visible = true
		healthBar.healthBarCenter.Size = UDim2.new(0,sizeX - capWidth,1,0)
		healthBar.healthBarRight.Visible = false
	else
		healthBar.healthBarCenter.Visible = true
		healthBar.healthBarCenter.Size = UDim2.new(1,-capWidth*2,1,0)
		healthBar.healthBarRight.Visible = true
	end

	local thresholdForHurtOverlay = currentHumanoid.MaxHealth * (HealthPercentageForOverlay/100)
	
	if healthDelta >= thresholdForHurtOverlay and guiEnabled then
		AnimateHurtOverlay()
	end

end

function AnimateHurtOverlay()
	if not HealthGui then return end
	
	local overlay = HealthGui:FindFirstChild("HurtOverlay")
	if not overlay then return end
	
	local newSize = UDim2.new(20, 0, 20, 0)
	local newPos = UDim2.new(-10, 0, -10, 0)

	if overlay:IsDescendantOf(game) then
		-- stop any tweens on overlay
		overlay:TweenSizeAndPosition(newSize,newPos,Enum.EasingDirection.Out,Enum.EasingStyle.Linear,0,true,function()
			
			-- show the gui
			overlay.Size = UDim2.new(1,0,1,0)
			overlay.Position = UDim2.new(0,0,0,0)
			overlay.Visible = true
			
			-- now tween the hide
			if overlay:IsDescendantOf(game) then
				overlay:TweenSizeAndPosition(newSize,newPos,Enum.EasingDirection.Out,Enum.EasingStyle.Quad,10,false,function()
					overlay.Visible = false
				end)
			else
				overlay.Size = newSize
				overlay.Position = newPos
			end
		end)
	else
		overlay.Size = newSize
		overlay.Position = newPos
	end

end

function humanoidDied()
	 UpdateGui(0)
end

function disconnectPlayerConnections()
	if characterAddedConnection then characterAddedConnection:disconnect() end
	if humanoidDiedConnection then humanoidDiedConnection:disconnect() end
	if healthChangedConnection then healthChangedConnection:disconnect() end
end

function newPlayerCharacter()
	disconnectPlayerConnections()
	
	local version = GameSettings.VirtualVersion
	if version == Enum.VirtualVersion['2016'] or version == Enum.VirtualVersion['2015'] or version == Enum.VirtualVersion['2014'] then
		startGui()
	else
		startGuiLegacy()
	end
end

function startGui()
	characterAddedConnection = game:GetService("Players").LocalPlayer.CharacterAdded:connect(newPlayerCharacter)

	local character = game:GetService("Players").LocalPlayer.Character
	if not character then
		return
	end

	currentHumanoid = character:WaitForChild("Humanoid")
	if not currentHumanoid then
		return
	end

	if not game:GetService("StarterGui"):GetCoreGuiEnabled(Enum.CoreGuiType.Health) then
		return
	end

	healthChangedConnection = currentHumanoid.HealthChanged:connect(UpdateGui)
	humanoidDiedConnection = currentHumanoid.Died:connect(humanoidDied)

	UpdateGui(currentHumanoid.Health)
		
	CreateGui()
end

function CreateGuiLegacy()
	if LegacyHealthGui then LegacyHealthGui:Destroy() end

	local tray = Instance.new("Frame")
	tray.Name = "HealthGui"
	tray.BackgroundColor3 = Color3.fromRGB(107, 50, 124)
	tray.BackgroundTransparency = 1
	tray.BorderColor3 = Color3.fromRGB(27, 42, 53)
	tray.BorderSizePixel = 1
	tray.Position = UDim2.new(0.5, -44, 1, -26)
	tray.Size = UDim2.new(0, 170, 0, 18)
	tray.SizeConstraint = Enum.SizeConstraint.RelativeYY
	tray.Visible = true
	tray.archivable = true
	tray.ZIndex = 1

	local bkg = Instance.new("ImageLabel")
	bkg.Name = "bkg"
	bkg.Active = false
	bkg.BackgroundTransparency = 1
	bkg.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	bkg.BorderColor3 = Color3.fromRGB(27, 42, 53)
	bkg.BorderSizePixel = 1
	bkg.Position = UDim2.new(0, 0, 0, 0)
	bkg.Image = "ayaasset://textures/ui/healthBarBkg.png"
	bkg.Size = UDim2.new(1, 0, 1, 0)
	bkg.SizeConstraint = Enum.SizeConstraint.RelativeXY
	bkg.Visible = true
	bkg.archivable = true
	bkg.ZIndex = 1
	bkg.Parent = tray

	local bar2 = Instance.new("Frame")
	bar2.Name = "bar2"
	bar2.Active = false
	bar2.BackgroundTransparency = 1
	bar2.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	bar2.BorderColor3 = Color3.fromRGB(27, 42, 53)
	bar2.BorderSizePixel = 0
	bar2.Position = UDim2.new(0.0189999994, 0, 0.100000001, 0)
	bar2.Size = UDim2.new(0.192000002, 0, 0.829999983, 0)
	bar2.SizeConstraint = Enum.SizeConstraint.RelativeXY
	bar2.Visible = true
	bar2.archivable = true
	bar2.ZIndex = 1
	bar2.Parent = tray

	local bar = Instance.new("ImageLabel")
	bar.Name = "bar"
	bar.Active = false
	bar.BackgroundTransparency = 1
	bar.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	bar.BorderColor3 = Color3.fromRGB(27, 42, 53)
	bar.BorderSizePixel = 1
	bar.Position = UDim2.new(0.0189999994, 0, 0.100000001, 0)
	bar.Image = "ayaasset://textures/ui/healthBarGreen.png"
	bar.Size = UDim2.new(0.959999979, 0, 0.829999983, 0)
	bar.SizeConstraint = Enum.SizeConstraint.RelativeXY
	bar.Visible = true
	bar.archivable = true
	bar.ZIndex = 1
	bar.Parent = tray

	local label = Instance.new("ImageLabel")
	label.Name = "label"
	label.Active = false
	label.BackgroundTransparency = 1
	label.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	label.BorderColor3 = Color3.fromRGB(27, 42, 53)
	label.BorderSizePixel = 0
	label.Position = UDim2.new(0.680000007, 0, 0.300000012, 0)
	label.Size = UDim2.new(0.25, 0, 0.349999994, 0)
	label.SizeConstraint = Enum.SizeConstraint.RelativeXY
	label.Visible = true
	label.archivable = true
	label.ZIndex = 1
	label.Parent = tray
	label.Image = "ayaasset://textures/ui/healthLabel.png"

	local barRed = Instance.new("ImageLabel")
	barRed.Name = "barRed"
	barRed.Active = false
	barRed.BackgroundTransparency = 1
	barRed.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	barRed.BorderColor3 = Color3.fromRGB(27, 42, 53)
	barRed.BorderSizePixel = 1
	barRed.Position = UDim2.new(0.0189999994, 0, 0.100000001, 0)
	barRed.Image = "ayaasset://textures/ui/healthBarRed.png"
	barRed.Size = UDim2.new(0, 0, 0, 0)
	barRed.SizeConstraint = Enum.SizeConstraint.RelativeXY
	barRed.Visible = true
	barRed.archivable = true
	barRed.ZIndex = 1
	barRed.Parent = tray

	local hurtOverlay = Instance.new("ImageLabel")
	hurtOverlay.Name = "HurtOverlay"
	hurtOverlay.BackgroundTransparency = 1
	hurtOverlay.Image = "ayaasset://textures/ui/legacyHurtOverlay.png"
	hurtOverlay.Position = UDim2.new(10, 0, 0, 0)
	hurtOverlay.Size = UDim2.new(1, 0, 1.15, 30)
	hurtOverlay.Visible = false
	hurtOverlay.Parent = tray

	LegacyHealthGui = tray
	LegacyHealthGui.Parent = game:GetService("CoreGui").RobloxGui
end

function UpdateGUILegacy(health)
	local tray = LegacyHealthGui
	local width = (health / currentHumanoid.MaxHealth) * maxWidth
	local height = 0.83
	local lastX = tray.bar.Position.X.Scale
	local x = 0.019 + (maxWidth - width)
	local y = 0.1
	
	tray.bar.Position = UDim2.new(x,0,y, 0) 
	tray.bar.Size = UDim2.new(width, 0, height, 0)
	-- If more than 1/4 health, bar = green.  Else, bar = red.
	if( (health / currentHumanoid.MaxHealth) > 0.25 ) then
		tray.barRed.Size = UDim2.new(0, 0, 0, 0)
	else
		tray.barRed.Position = tray.bar.Position
		tray.barRed.Size = tray.bar.Size
		tray.bar.Size = UDim2.new(0, 0, 0, 0)
	end
	
	if ( (lastHealth - health) > (currentHumanoid.MaxHealth / 10) ) then
		lastHealth = health

		if currentHumanoid.Health ~= currentHumanoid.MaxHealth then
			delay(0,function()
				AnimateHurtOverlay()
			end)
			delay(0,function()
				AnimateBarsLegacy(x, y, lastX, height)
			end)
		end
	else
		lastHealth = health
	end
end

function AnimateHurtOverlay()
	-- Start:
	-- overlay.Position = UDim2.new(0, 0, 0, -22)
	-- overlay.Size = UDim2.new(1, 0, 1.15, 30)
	
	-- Finish:
	-- overlay.Position = UDim2.new(-2, 0, -2, -22)
	-- overlay.Size = UDim2.new(4.5, 0, 4.65, 30)
	
	overlay = LegacyHealthGui.HurtOverlay
	overlay.Position = UDim2.new(-2, 0, -2, -22)
	overlay.Size = UDim2.new(4.5, 0, 4.65, 30)
	-- Animate In, fast
	local i_total = 2
	local wiggle_total = 0
	local wiggle_i = 0.02
	for i=1,i_total do
		overlay.Position = UDim2.new( (-2 + (2 * (i/i_total)) + wiggle_total/2), 0, (-2 + (2 * (i/i_total)) + wiggle_total/2), -22 )
		overlay.Size = UDim2.new( (4.5 - (3.5 * (i/i_total)) + wiggle_total), 0, (4.65 - (3.5 * (i/i_total)) + wiggle_total), 30 )
		wait(0.01)
	end
	
	i_total = 30
	
	wait(0.03)
	
	-- Animate Out, slow
	for i=1,i_total do
		if( math.abs(wiggle_total) > (wiggle_i * 3) ) then
			wiggle_i = -wiggle_i
		end
		wiggle_total = wiggle_total + wiggle_i
		overlay.Position = UDim2.new( (0 - (2 * (i/i_total)) + wiggle_total/2), 0, (0 - (2 * (i/i_total)) + wiggle_total/2), -22 )
		overlay.Size = UDim2.new( (1 + (3.5 * (i/i_total)) + wiggle_total), 0, (1.15 + (3.5 * (i/i_total)) + wiggle_total), 30 )
		wait(0.01)
	end
	
	-- Hide after we're done
	overlay.Position = UDim2.new(10, 0, 0, 0)
end

function AnimateBarsLegacy(x, y, lastX, height)
	local tray = LegacyHealthGui
	local width = math.abs(x - lastX)
	if( x > lastX ) then
		x = lastX
	end
	tray.bar2.Position = UDim2.new(x,0, y, 0)
	tray.bar2.Size = UDim2.new(width, 0, height, 0)
	tray.bar2.BackgroundTransparency = 0
	local GBchannels = 1
	local j = 0.2

	local i_total = 30
	for i=1,i_total do
		-- Increment Values
		if (GBchannels < 0.2) then
			j = -j
		end
		GBchannels = GBchannels + j
		if (i > (i_total - 10)) then
			tray.bar2.BackgroundTransparency = tray.bar2.BackgroundTransparency + 0.1
		end
		tray.bar2.BackgroundColor3 = Color3.new(1, GBchannels, GBchannels)
		
		wait(0.02)
	end
end

function HealthChangedLegacy(health)
	UpdateGUILegacy(health)
	if ( (lastHealth2 - health) > (currentHumanoid.MaxHealth / 10) ) then
		lastHealth2 = health
	else
		lastHealth2 = health
	end
end

function startGuiLegacy()
	characterAddedConnection = game:GetService("Players").LocalPlayer.CharacterAdded:connect(newPlayerCharacter)

	local character = game:GetService("Players").LocalPlayer.Character
	if not character then
		return
	end

	currentHumanoid = character:WaitForChild("Humanoid")
	if not currentHumanoid then
		return
	end

	if not game:GetService("StarterGui"):GetCoreGuiEnabled(Enum.CoreGuiType.Health) then
		return
	end

	healthChangedConnection = currentHumanoid.HealthChanged:connect(HealthChangedLegacy)
	humanoidDiedConnection = currentHumanoid.Died:connect(function() HealthChangedLegacy(0) end)

	CreateGuiLegacy()
end



---------------------------------------------------------------------
-- Start Script

HealthGui = Instance.new("Frame")
HealthGui.Name = "HealthGui"
HealthGui.BackgroundTransparency = 1
HealthGui.Size = UDim2.new(1,0,1,0)

game:GetService("StarterGui").CoreGuiChangedSignal:connect(function(coreGuiType,enabled)
	if coreGuiType == Enum.CoreGuiType.Health or coreGuiType == Enum.CoreGuiType.All then
		if guiEnabled and not enabled then
			if HealthGui then
				HealthGui.Parent = nil
			end

			if LegacyHealthGui then
				LegacyHealthGui:Destroy()
			end

			disconnectPlayerConnections()
		elseif not guiEnabled and enabled then
			startGui()
		end
		
		guiEnabled = enabled
	end
end)

if game:GetService("StarterGui"):GetCoreGuiEnabled(Enum.CoreGuiType.Health) and GameSettings.VirtualVersion ~= Enum.VirtualVersion['2016'] then
	guiEnabled = true
	startGuiLegacy()
end

GameSettings.Changed:connect(function(prop)
	if prop ~= 'VirtualVersion' then return end

	local enabled = GameSettings.VirtualVersion ~= Enum.VirtualVersion['2016']

	if guiEnabled and not enabled then
		if HealthGui then
			HealthGui.Parent = nil
		end

		if LegacyHealthGui then
			LegacyHealthGui:Destroy()
		end
		
		disconnectPlayerConnections()
	elseif not guiEnabled and enabled then
		startGuiLegacy()
	end
	
	guiEnabled = enabled

	local version = GameSettings.VirtualVersion

	if guiEnabled and HealthGui and version ~= Enum.VirtualVersion['2016'] and version ~= Enum.VirtualVersion['2015'] and version ~= Enum.VirtualVersion['2014'] then
		if HealthGui then
			HealthGui.Parent = nil
		end
		disconnectPlayerConnections()
		startGuiLegacy()
	elseif guiEnabled and LegacyHealthGui and (version == Enum.VirtualVersion['2016'] or version == Enum.VirtualVersion['2015'] or version == Enum.VirtualVersion['2014']) then
		if LegacyHealthGui then
			LegacyHealthGui:Destroy()
		end
		disconnectPlayerConnections()
		startGuiLegacy()
	end
end)