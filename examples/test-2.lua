require "yats.stdlib"
require "yats.src"
require "yats.muxdmx"
require "yats.misc"
require "yats.dummy"

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
print("Loaded packages: \n"..ptostring(_LOADED))

-- Init the yats random generator.
print("Init random generator.")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time.")
yats.sim:ResetTime()

-- Use any number you like here
nsrc = 2

-- Create Sources
src = {}
for i = 1, nsrc do
  src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
end

-- Create Multiplexer
mx = yats.mux{"mux", ninp=nsrc, buff=10, out={"demux", "demux"}}

-- Create Demultiplexer
-- We first create the list of outputs.
local dout = {}
for i = 1, nsrc do
--  dout[i] = {"line"..i, "line"..i}
  dout[i] = {"dummy"..i, "luadummy"}
end
dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}

-- Create Lines and Sinks
lin, snk = {}, {}
dummy = {}
for i = 1, nsrc do
--  lin[i], t, name = yats.line:new{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
  dummy[i] = yats.luadummy{"dummy"..i, out = {"line"..i, "line"}}
  lin[i] = yats.line{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
  snk[i] = yats.sink{"sink"..i}
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

-- Some test output
if test then
  print(yats.sim:list("\nList of objects","\t"))
  print("List of objects:"..tostring(yats.sim:list()))
  print("\nA more detailed look on objects:")
  print(ptostring(yats.sim.objectlist))
end

-- Connection management
--yats.sim:run(0,0)
yats.sim:connect()

-- Check connection
print("Connection check:")
for i = 1, nsrc do
  print("\tsuc of "..dummy[i].name..": "..src[i]:get_suc().name, src[i]:get_shand())
  print("\tsuc of "..src[i].name..": "..src[i]:get_suc().name, src[i]:get_shand())
  print("\tsuc "..i.." of "..dmx.name..": "..dmx:get_suc(i).name, dmx:get_shand(i))
  print("\tsuc of "..lin[i].name..": "..lin[i]:get_suc().name, lin[i]:get_shand())
end

-- Display demux switch table
print("demux switch table:\n"..pretty(dmx:getRouting()))
local st, entries 
st, entries = dmx:getRouting()
print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
  print("(from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Run simulation: 1. path
yats.sim:run(50000, 10000)
print("Mux Queue Len: "..mx:getQLen())

-- Run simulation: 2. path (optional)
--yats.sim:run(50000, 10000)

-- Display some values from the Multiplexer.
print("Mux Queue Len: "..mx:getQLen())
print("Mux Queue MaxLen: "..mx:getQMaxLen())
for i = 1, nsrc do
  print("Mux "..mx.name.." Loss "..i..": "..mx:getLoss(i).."  "..mx:getLossVCI(i))
end
-- Display source counters.
local result = {}
for i = 1, nsrc do
  print("Source "..src[i].name.." couont: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end
-- Display sink counter.
for i = 1, nsrc do
  print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end
-- Display dummy object values
for i = 1, nsrc do
  print("lua1out "..dummy[i].name.." data counter = "..dummy[i]:get_counter())
  print("lua1out "..dummy[i].name.." event counter = "..tostring(dummy[i]:get_evcounter()))
end
return result