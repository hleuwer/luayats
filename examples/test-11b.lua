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
yats.sim:run(2500, 1000)

-- Produce test result
result = {}
for _, b in ipairs{bridge_1, bridge_2} do
  local t = b:get_stpm(0)
  for _,v in pairs(t) do
    if type(v) ~= "table" then
      table.insert(result, v)
    end
  end
  for i = 1,4 do
    t = b:get_port(0,i)
    for _, v in pairs(t) do
      if type(v) ~= "table" then
	table.insert(result, v)
      end
    end
  end
end
yats.log:info("Setting forward delay.")
assert(bridge_1:config_stp(0, "forward_delay", 12))
assert(bridge_2:config_stp(0, "forward_delay", 12))

-- Run simulator: 2. path
yats.sim:run(40000, 1000)

-- Produce test result
for _, b in ipairs{bridge_1, bridge_2} do
  local t = b:get_stpm(0)
  for _,v in pairs(t) do
    if type(v) ~= "table" then
      table.insert(result, v)
    end
  end
  for i = 1,4 do
    t = b:get_port(0,i)
    for _, v in pairs(t) do
      if type(v) ~= "table" then
	table.insert(result, v)
      end
    end
  end
end

-- Show bridge state after 1st convergence
bridge_1:show_port(0)
bridge_2:show_port(0)

-- Unplug bridge port 
yats.log:info("Unplugging port 1 of bridge 2")
bridge_2:plug_out(1)

-- Run simulator: 3. path
yats.sim:run(150000, 1000)

-- Produce test result
for _, b in ipairs{bridge_1, bridge_2} do
  local t = b:get_stpm(0)
  for _,v in pairs(t) do
    if type(v) ~= "table" then
      table.insert(result, v)
    end
  end
  for i = 1,4 do
    t = b:get_port(0,i)
    for _, v in pairs(t) do
      if type(v) ~= "table" then
	table.insert(result, v)
      end
    end
  end
end

-- Show bridge state after 2nd convergence
bridge_1:show_port(0)
bridge_2:show_port(0)


yats.log:info(string.format("\nbridge_1 state = %s\n", pretty(bridge_1:get_stpstate(0))))
yats.log:info(string.format("\nbridge_2 state = %s\n", pretty(bridge_2:get_stpstate(0))))
yats.log:info(string.format("\nbridge_1 port 1 = %s\n", pretty(bridge_1:get_portstate(0, 1))))

return result