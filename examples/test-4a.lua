require "yats.stdlib"
require "yats.src"
require "yats.muxdmx"
require "yats.misc"
require "yats.graphics"

-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 --> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 --> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 --> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 --> | |    | |--> line_4 --> sink_4
-- cbrsrc_n --> |/      \|--> line_n --> sink_n
--              mux    demux

-- Setting test non nil provides additional output.
local test = nil

-- Show a list of loaded packages.
--print("Loaded packages: \n"..ptostring(_LOADED))

-- Init the yats random generator.
--print("Init random generator.")
yats.sim:SetRand(10)

-- Reset simulation time.
--print("Reset simulation time.")
yats.sim:ResetTime()

-- Use any number you like here
local nsrc = 10

-- Create Sources
local src = {}
for i = 1, nsrc do
  src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
end

-- Create Multiplexer
mx = yats.mux{"mux", ninp=nsrc, buff=10, out={"demux", "demux"}}

-- Create Demultiplexer
-- We first create the list of outputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
local dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}

-- Create Lines and Sinks
local lin, snk, meas = {}, {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas"}}
  snk[i] = yats.sink{"sink"..i}
  meas[i] = yats.meas{"meas"..i, vci = i, maxtim = 1000, out = {"sink"..i, "sink"}}
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

local histo = yats.histo2{
  "hist", val={mx,"QLen"}, 
  title = "QLEN", win={100,100,200,200}, nvals = 100, update=10000
}

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
if false then
  print("Connection check:")
  for i = 1, nsrc do
    print("\tsuc of "..src[i].name..": "..src[i]:get_suc().luaobj.name, src[i]:get_shand())
    print("\tsuc "..i.." of "..dmx.name..": "..dmx:get_suc(i).luaobj.name, dmx:get_shand(i))
    print("\tsuc of "..lin[i].name..": "..lin[i]:get_suc().luaobj.name, lin[i]:get_shand())
  end
end

-- Display demux switch table
if false then
  print("demux switch table: "..tostring(dmx:getRouting()))
  local st, entries 
  st, entries = dmx:getRouting()
  print("Table has "..entries.." entries.")
  for i = 1, table.getn(st) do
    print("(from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
  end
end

-- Run simulation: 1. path
yats.sim:run(1000000, 10000)
--print("Mux Queue Len: "..mx:getQLen())

-- Run simulation: 2. path (optional)
--yats.sim:run(1000000, 100000)

-- Display some values from the Multiplexer.
if false then
  print("Mux Queue Len: "..mx:getQLen())
  print("Mux Queue MaxLen: "..mx:getQMaxLen())
  print("Mux Losses per VC:")
  for i = 1, nsrc do
    print("Mux "..mx.name.." Loss "..i..": "..mx:getLoss(i).."  "..mx:getLossVCI(i))
  end
  print("\nSource cell counters:")
-- Display source counters.
  for i = 1, nsrc do
    print("Source "..src[i].name.." couont: "..src[i]:getCounter())
  end
  -- Display meas counter.
  print("\nMeasurement cell counters:")
  for i = 1, nsrc do
    print("Meas "..meas[i].name.." count: "..meas[i]:getCounter())
  end
  -- Display sink counter.
  print("\nSink cell counters:")
  for i = 1, nsrc do
    print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
  end
  -- Display meas distances.
  print("\nCell Delays:")
  for i = 1, nsrc do
    io.stdout:write(string.format("%2d:",i))
    for j = 1, 15 do
      io.stdout:write(string.format("%7d",meas[i]:getDist(j)))
    end
    print()
  end
end
