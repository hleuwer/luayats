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
yats.sim:run(60000, 1000)

-- Produce test result.
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

bridge_1:show_stp(0)
bridge_2:show_stp(0)
bridge_1:show_port(0)
bridge_2:show_port(0)
bridge_1:show_port(0,1)
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


for _, b in ipairs{bridge_1, bridge_2} do
  printf("INSPECT `%s'\n", b.name)
  for k,v in b:inspect() do
    if type(v.val) == "function" then
--      printf("inspect %d: key=`%s', INSPECT ITERATOR %s\n", 
--	     k, v.key, tostring2(v.val))
      printf("INSPECT `%s' `%s'\n", b.name, v.val(b, "*n", unpack(v.arg)))
      for l, w in v.val(b, "*d", unpack(v.arg)) do
	printf("inspect %d.%d: key=`%s', val=%s, modifier=%s\n",
	       k, l, w.key, w.val or "?", tostring2(w.modifier))
	table.insert(result, w.key)
	table.insert(result, w.val)
      end
      print()
    else
      printf("inspect %d: key=`%s', val=%s, modifier=%s\n",
	     k, v.key, v.val, tostring2(v.modifier))
      table.insert(result, v.key)
      table.insert(result, v.val)
    end
  end
  print()
end
--print(pretty(result))
return result