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
test = nil

-- Show a list of loaded packages.
print("Loaded packages: \n"..pretty(_LOADED))

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
dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}

-- Create Lines and Sinks
lin, snk, meas = {}, {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas"}}
  snk[i] = yats.sink{"sink"..i}
  meas[i] = yats.meas{"meas"..i, vci = i, MAXTIM = 1000, out = {"sink"..i, "sink"}}
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

--histo = yats.histo2:new{"hist", val={mx,"QLen"}, title = "QLEN", win={100,100,400,200}, nvals = 100, update=10000}
meter = yats.meter{
  "meter", val={mx,"QLen"}, title = "QLEN-meter", 
  win={500, 100, 400, 200}, nvals = 100, delta = 1000, 
  mode=yats.AbsMode, maxval = 20, linemode=1
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

-- Display demux switch table
print("demux switch table: "..tostring(dmx:getRouting()))
local st, entries 
st, entries = dmx:getRouting()
print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
  print("(from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Run simulation: 1. path
yats.sim:run(1000000, 100000)
print("Mux Queue Len: "..mx:getQLen())

-- Run simulation: 2. path (optional)
--yats.sim:run(1000000, 100000)
result = {}
-- Display some values from the Multiplexer.
print("Mux Queue Len: "..mx:getQLen())
print("Mux Queue MaxLen: "..mx:getQMaxLen())
print("Mux Losses per VC:")
table.insert(result, mx:getQLen())
table.insert(result, mx:getQMaxLen())
for i = 1, nsrc do
  print("Mux "..mx.name.." Loss "..i..": "..mx:getLoss(i).."  "..mx:getLossVCI(i))
  table.insert(result, mx:getLoss(i))
end
print("\nSource cell counters:")
-- Display source counters.
for i = 1, nsrc do
  print("Source "..src[i].name.." couont: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end
-- Display meas counter.
print("\nMeasurement cell counters:")
for i = 1, nsrc do
  print("Meas "..meas[i].name.." count: "..meas[i]:getCounter())
  table.insert(result, meas[i]:getCounter())
end
-- Display sink counter.
print("\nSink cell counters:")
for i = 1, nsrc do
  print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end
-- Display meas distances.
print("\nCell Delays:")
for i = 1, nsrc do
  io.stdout:write(string.format("%2d:",i))
  for j = 1, 15 do
    io.stdout:write(string.format("%7d",meas[i]:getDist(j)))
    table.insert(result, meas[i]:getDist(j))
  end
  print()
end

return result