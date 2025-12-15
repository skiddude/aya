function Start(Port, Level)
	local NetworkServer = game:GetService("NetworkServer")
	local Players = game:GetService("Players")
	local RunService = game:GetService("RunService")

	local PlaceId = 1818
	-- local Port = 53640
	-- local Level = "ayaasset://test.rbxl"

	-- Hook Players events
	Players.PlayerAdded:Connect(function(player)
		print("Player " .. player.userId .. " added")
	end)

	Players.PlayerRemoving:Connect(function(player)
		print("Player " .. player.userId .. " leaving")
	end)

	-- Network settings
	pcall(function() settings().Network.UseInstancePacketCache = true end)
	pcall(function() settings().Network.UsePhysicsPacketCache = true end)
	pcall(function() settings()["Task Scheduler"].PriorityMethod = Enum.PriorityMethod.AccumulatedError end)

	settings().Network.PhysicsSend = Enum.PhysicsSendMethod.TopNErrors
	settings().Network.ExperimentalPhysicsEnabled = true
	settings().Network.WaitingForCharacterLogRate = 100

	settings().Diagnostics.LuaRamLimit = 0

	-- Load and start the place
	game:SetPlaceID(PlaceId, false)
	game:GetService("ChangeHistoryService"):SetEnabled(false)

	game:Load(Level)

	NetworkServer:Start(Port) 

	-- Run the server
	RunService:Run()

	print("== Aya.Server : Started place ID " .. PlaceId .. " on port " .. Port .. " ==")
end
