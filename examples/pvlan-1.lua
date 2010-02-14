require "yats"
require "yats.stdlib"
require "yats.rstp"
require "yats.misc"

local function sdel(t,u,v)
  if regression == true then 
    return u 
  end
  if t == "r" then
    return math.random(u,v)
  elseif t == "c" then
    return u
  end
end


del = {
  sdel("c", 10, 2000),
  sdel("c", 10, 2000),
  sdel("c", 10, 2000),
  sdel("c", 10, 2000),
  sdel("c", 10, 2000),
  sdel("c", 10, 2000),
  nil
}
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
--- Couple of nc sinks
--local snk = {}
--for i=1,10 do snk[i] = yats.sink{"sink-"..i} end

--- North Bridges.
ne = {
  pe = {
    west = yats.bridge{
      "PE-WEST", 
      nport=4, 
      basemac="010000",
      start_delay=del[1],
      out = {
	{"ELS-WEST","in"..3},
	nc(),
	{"PE-EAST","in"..3},
	nc()
      }
    },
    east = yats.bridge{
      "PE-EAST", 
      nport=4, 
      basemac="020000",
      start_delay=del[2],
      out = {
	nc(),
	{"ELS-EAST","in"..4},
	{"PE-WEST","in"..3},
	nc(),
      }
    }
  },
   
  --- Marconi Bridges.
  els = {
    west = yats.bridge{
      "ELS-WEST", 
      nport=4, 
      basemac="030000",
      start_delay=del[3],
      out = {
	{"CE-WEST","in"..1},
	{"CE-EAST","in"..1},
	{"PE-WEST","in"..1},
	nc()
      }
    },
    east = yats.bridge{
      "ELS-EAST", 
      nport=4, 
      basemac="040000",
      start_delay=del[4],
      out = {
	{"CE-WEST","in"..2},
	{"CE-EAST","in"..2},
	nc(),
	{"PE-EAST","in"..2}
      }
    }
  },
  --- South Bridges.
  ce = {
    west = yats.bridge{
      "CE-WEST", 
      nport=4, 
      basemac="050000",
      start_delay=del[5],
      out = {
	{"ELS-WEST","in"..1},
	{"ELS-EAST","in"..1},
	{"CE-EAST","in"..3},
	nc(),
      }
    },
    east = yats.bridge{
      "CE-EAST", 
      nport=4, 
      basemac="060000",
      start_delay=del[6],
      out = {
	{"ELS-WEST","in"..2},
	{"ELS-EAST","in"..2},
	{"CE-WEST","in"..3},
	nc(),
      }
    }
  }
}

-- Connection management
yats.log:debug("connecting")
yats.sim:connect()

for key, pe2ce in pairs(ne) do
  for key, bridge in pairs(pe2ce) do
    for i, t in bridge:successors() do
      printf("%s pin %d => %s handle %d\n", bridge.name, i, t.suc.name, t.shand)
    end
  end
end

-- Run simulation: 1. path
yats.sim:run(60000, 1000)


for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_stp(0)
   end
end
for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_port(0)
   end
end

ne.els.west:linkDown(1)

yats.sim:run(60000, 1000)


for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_stp(0)
   end
end
for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_port(0)
   end
end

ne.els.west:linkUp(1)

yats.sim:run(60000, 1000)


for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_stp(0)
   end
end
for key, pe2ce in pairs(ne) do
   for key, bridge in pairs(pe2ce) do
      bridge:show_port(0)
   end
end
result = {}
for _, loc in pairs(ne) do
  for _, b in pairs(loc) do
    local t = b:get_stpm(0)
    for k,v in pairs(t) do
      if type(v) ~= "table" then
	table.insert(result, {k,v})
      end
    end
    for i = 1,4 do
      t = b:get_port(0,i)
      for k, v in pairs(t) do
	if type(v) ~= "table" then
	  table.insert(result, {k,v})
	end
      end
    end
  end
end

--print("Netlist")
--yats.makeDot("pvlan-1")
--print(pretty(result))
return result