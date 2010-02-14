require "stdlib"
require "src"
require "muxevt"
require "misc"
require "graphics"

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

-- Init the yats random generator.
print("Init random generator...")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time...")
yats.sim:ResetTime()

-- Use any number you like here
nsrc = 6

-- Create Sources
src = {}
i = 1
src[i] = yats.bssrc{"src"..i, ex = 100, es  = 200, delta = 2, vci = i, out = {"mux", "in"..i}}

i = 2
src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}

i = 3
src[i] = yats.listsrc{"src"..i, delta = {1,5,6,2,1,1}, vci = i, rep=true, out = {"mux", "in"..i}}

for i = 4, nsrc do
  src[i] = yats.geosrc{"src"..i, ed = 10, vci = i, out = {"mux", "in"..i}}
end

-- Create Multiplexer
mx = yats.muxBase{"mux", ninp=nsrc, buff=10, service=2, out={"demux", "demux"}}

-- Create Demultiplexer
-- We first create the list of outputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}

local x,y,dx,dy = 0, 0, 200, 100

-- Create Lines and Sinks
lin, snk, meas, hist = {}, {}, {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas3"}}
  snk[i] = yats.sink{"sink"..i}
  meas[i] = yats.meas3{"meas"..i, vci = {i,i}, iat = {0,100}, iatdiv = 1, out = {"sink"..i, "sink"}}
  hist[i] = yats.histo{"hist"..i, title = "VCI="..i..": IAT", 
    val = {meas[i], "IAT", i}, win = {x,y,dx,dy}, delta=1000, display=false
  }
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end


histo = yats.histo2{
  "hist", val={mx,"QLen"}, title = "QLEN", 
  win={100,100,400,200}, nvals = 20, update=10000, display=false
}

disp = yats.bigdisplay {
  "disp", title="IAT", nrow = 6, ncol = 2, width = 200, height = 100, 
  instruments = {
    {histo, hist[1]},
    {nil, hist[2]},
    {nil, hist[3]},
    {nil, hist[4]},
    {nil, hist[5]},
    {nil, hist[6]},
  }
}

disp:show()

-- Connection management
yats.sim:connect()

-- Display demux switch table
local st, entries = dmx:getRouting()
print("Demux switch table")
print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
  print("  (from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Run simulation: 1. path
yats.sim:run(5000000, 100000)

result = {}
-- Display some values from the Multiplexer.
print("Multiplexer snapshot:")
print("  Queue Len: "..mx:getQLen())
print("  Queue MaxLen: "..mx:getQMaxLen())
table.insert(result, mx:getQLen())
table.insert(result, mx:getQMaxLen())
print("  Losses per VC:")
for i = 1, nsrc do
  print("    Loss "..i..": "..mx:getLoss(i).."  "..mx:getLossVCI(i).."  "..
	string.format("%.1f %%", mx:getLoss(i)*100/src[i].counter))
  table.insert(result, mx:getLoss(i))
  table.insert(result, mx:getLossVCI(i))
end
print("Source cell counters:")
-- Display source counters.
for i = 1, nsrc do
  print("  "..src[i].name.." count: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end
-- Display meas counter.
print("Measurement cell counters:")
for i = 1, nsrc do
  print("  "..meas[i].name.." count: "..meas[i]:getCounter())
  table.insert(result, meas[i]:getCounter())
end
-- Display sink counter.
print("Sink cell counters:")
for i = 1, nsrc do
  print("  "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end
return result