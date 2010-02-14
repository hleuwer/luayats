require "yats"
require "yats.stdlib"
require "yats.rstp"

-- Example test-11.lua: 2 RSTP instances

-- Setting test non nil provides additional output.
test = true

-- Init the yats random generator.
yats.log:info("Init random generator.")
yats.sim:SetRand(10)
yats.sim:setSlotLength(1e-3)

-- Reset simulation time.
yats.log:info("Reset simulation time.")
yats.sim:ResetTime()

if false then
luactrl = yats.luacontrol{
  "luactrl",
  actions = {
    early = {
      {100000, yats.cli}
    },
    late = {},
  }
}
end
bridge_1 = yats.bridge{
  "bridge_1", 
  nport=4, 
  basemac="110000",
  vlans = {[0] = 32768, [5] = 32768},
  memberset = {[0] = {1,1,1,1}, [5] = {1,1,1,1}},
  start_delay=start_delay_1 or yats.random(1,2000),
  out = {
    {"bridge_2","in"..1},
    {"bridge_2","in"..2}
  }
}
 
yats.log:debug(string.format("Object '%s' created", bridge_1.name))
bridge_2 = yats.bridge{
  "bridge_2", 
  nport=4, 
  basemac="220000",
  vlans = {[0] = 32765, [5] = 32768},
  memberset = {[0] = {1,1,1,1}, [5] = {1,0,1,1}},
  start_delay=start_delay_2 or yats.random(1,2000),
  out = {
    {"bridge_1","in"..1},
    {"bridge_1","in"..2}
  }
}
yats.log:debug(string.format("Object '%s' created", bridge_1.name))
yats.log:debug(string.format("Object '%s' created", bridge_2.name))

-- Connection management
yats.log:debug("connecting")
yats.sim:connect()
yats.log:debug("connected.")
-- Check connection
yats.log:debug("Connection check:")
yats.log:debug("\tsuc of "..bridge_1.name..": "..bridge_1:get_suc(1).name, bridge_1:get_shand(1))
yats.log:debug("\tsuc of "..bridge_1.name..": "..bridge_1:get_suc(2).name, bridge_1:get_shand(2))
yats.log:debug("\tsuc of "..bridge_2.name..": "..bridge_2:get_suc(1).name, bridge_2:get_shand(1))
yats.log:debug("\tsuc of "..bridge_2.name..": "..bridge_2:get_suc(2).name, bridge_2:get_shand(2))

-- Run simulation: 1. path
yats.sim:run(60000, 1000)
result = {}
printf("==============================\n")
printf("Spanning Tree Instance VLAN 0: show_stp(0)\n")
printf("==============================\n")
bridge_1:show_stp(0)
bridge_2:show_stp(0)
bridge_1:show_port(0,1)
printf("==============================\n")
printf("Spanning Tree Instance VLAN 5: show_stp(5)\n")
printf("==============================\n")
bridge_1:show_stp(5)
bridge_2:show_stp(5)
printf("==============================\n")
printf("Spanning Tree Instance VLAN 0: show_port(0)\n")
printf("==============================\n")
bridge_1:show_port(0)
bridge_2:show_port(0)
printf("==============================\n")
printf("Spanning Tree Instance VLAN 5: show_port(5)\n")
printf("==============================\n")
bridge_1:show_port(5)
bridge_2:show_port(5)

for _, vid in pairs{0, 5} do
  for _, b in pairs{bridge_1, bridge_2} do
    local t = b:get_stpm(vid)
    for k,v in pairs(t or {}) do
      if type(v) ~= "table" then
	table.insert(result, {k,v})
      end
    end
    for i = 1,4 do
      t = b:get_port(vid,i)
      for k, v in pairs(t or {}) do
	if type(v) ~= "table" then
	  table.insert(result, {k,v})
	end
      end
    end
  end
end
--table.insert(result, bridge_1:get_stpm(0))
--table.insert(result, bridge_2:get_stpm(0))
--print(pretty(result))
--print(pretty(bridge_1:get_stpstate(0)))
---print(pretty(bridge_2:get_stpstate(0)))
if false then
  local rstp1 = bridge_1.rstp
  local stp1 = yats.ieeebridge.stpmstate:new()
  local stp2 = rstp1:get_stpmstate(0, stp1.val)
  local rstp1 = bridge_2.rstp
  local stp1 = yats.ieeebridge.stpmstate:new()
  local stp2 = rstp1:get_stpmstate(0, stp1.val)
end
return result