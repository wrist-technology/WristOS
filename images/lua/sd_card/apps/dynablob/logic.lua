#!/usr/bin/env lua
local dynawa = {}
local app = {}

function app:init_blob()
	return {age = 0, stage = 1, intestine = 0.5, food = 3, food_max = 10, bladder = 0.5, liquid = 3, liquid_max = 10, shit = 0, piss = 0, disease = 0}
end

function app:init()
	self:init_player()
	self.blob = self:init_blob()
	local val = 0
	local t = os.time()
	while true do
		local input = io.read()
		app:tick(self.blob)
		app:print_stats(self.blob)
	end
end

function app:init_player()
	self.player={cash = 30}
end

function app:tick(blob)
	local player = self.player
	player.cash = player.cash + 1
	blob.age = blob.age + 1
	local rnd = math.random()
	blob.food = blob.food - rnd
	blob.bladder = blob.bladder 
end

local round = function (num)
	return string.format("%1.2f",num)
end

function app:print_stats(blob)
	print("Age","stage","intest","food","bladder","liquid","shit","piss","disease")
	print(blob.age,blob.stage, blob.intestine, (round(blob.food).."/"..blob.food_max), blob.bladder, (round(blob.liquid).."/"..blob.liquid_max), blob.shit, blob.piss, blob.disease)
end

app:init()

