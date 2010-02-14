require "stdlib"
require "src"
require "muxdmx"
require "misc"

-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 --> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 --> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 --> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 --> | |    | |--> line_4 --> sink_4
-- cbrsrc_n --> |/      \|--> line_n --> sink_n
--              mux    demux

-- Setting test non nil provides additional output.
test = nil

-- Show a list of loaded packages.
--print("Loaded packages: \n"..ptostring(_LOADED))

-- Init the yats random generator.
print("Init random generator.")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time.")
yats.sim:ResetTime()

-- Use any number you like here
nsrc = 6

-- Create Sources
src = {}
for i = 1, nsrc do
  src[i], t, name = yats.cbrsrc:new{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
--  print("object '"..name.."' of class '"..(t.luaobj.clname or "unknown").."' created.", t)
end

-- Create Multiplexer
mx, t, name = yats.mux:new{"mux", ninp=nsrc, buff=10, out={"demux", "demux"}}
--print("object '"..name.."' of class '"..(t.luaobj.clname or "unknown").."' created.", t)

-- Create Demultiplexer
-- We first create the list of otputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
dmx, t, name = yats.demux:new{"demux", maxvci = nsrc, nout = nsrc, out = dout}
--print("object '"..name.."' of class '"..(t.luaobj.clname or "unknown").."' created.", t)

-- Create Lines and Sinks
lin, snk = {}, {}
for i = 1, nsrc do
  lin[i], t, name = yats.line:new{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
--  print("object '"..name.."' of class '"..(t.luaobj.clname or "unknown").."' created.", t)
  snk[i], t, name = yats.sink:new{"sink"..i}
--  print("object '"..name.."' of class '"..(t.luaobj.clname or "unknown").."' created.", t)
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

-- Some test output
if test then
--  print(yats.sim:list("\nList of objects","\t"))
--  print("List of objects:"..tostring(yats.sim:list()))
--  print("\nA more detailed look on objects:")
--  print(ptostring(yats.sim.objectlist))
end

-- Connection management
--yats.sim:run(0,0)
yats.sim:connect()

-- Check connection
--print("Connection check:")
for i = 1, nsrc do
--  print("\tsuc of "..src[i].name..": "..src[i]:get_suc().luaobj.name, src[i]:get_shand())
--  print("\tsuc "..i.." of "..dmx.name..": "..dmx:get_suc(i).luaobj.name, dmx:get_shand(i))
--  print("\tsuc of "..lin[i].name..": "..lin[i]:get_suc().luaobj.name, lin[i]:get_shand())
end

-- Display demux switch table
--print("demux switch table: "..tostring(dmx:getRouting()))
local st, entries 
st, entries = dmx:getRouting()
--print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
--  print("(from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Run simulation: 1. path
for i = 1, 200 do
  yats.log:debug("Running "..i)
  yats.sim:run(10000, 10000)
end
--yats.sim:run(1000000, 100000)

--print("Mux Queue Len: "..mx:getQLen())

-- Run simulation: 2. path (optional)
yats.sim:run(1000000, 100000)

-- Display some values from the Multiplexer.
--print("Mux Queue Len: "..mx:getQLen())
--print("Mux Queue MaxLen: "..mx:getQMaxLen())
for i = 1, nsrc do
  --print("Mux "..mx.name.." Loss "..i..": "..mx:getLoss(i).."  "..mx:getLossVCI(i))
  -- Note: tolua maps lua table index 1 to C index 0. 
  -- Hence, we to add 1 to the vci index in mx.yatsobj.lostVCI[i]
  --print("Mux "..mx.name.." Loss "..i..": "..mx.yatsobj.lost[i].."  "..mx.yatsobj.lostVCI[i+1])
end
-- Display source counters.
for i = 1, nsrc do
  --print("Source "..src[i].name.." couont: "..src[i]:getCounter())
end
-- Display sink counter.
for i = 1, nsrc do
  --print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
end
