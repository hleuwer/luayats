require "yats"
require "yats.stdlib"
require "yats.src"
require "yats.muxevt"
require "yats.misc"
require "yats.graphics"

-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 -> dat2frm -> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 -> dat2frm -> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 -> dat2frm -> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 -> dat2frm -> | |    | |--> line_4 --> sink_4
-- cbrsrc_n -> dat2frm -> |/      \|--> line_n --> sink_n
--                        mux    demux

-- Setting test non nil provides additional output.
test = nil

-- Init the yats random generator.
print("Init random generator...")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time...")
yats.sim:ResetTime()

-- Use any number you like here
local flen = 1024
local nsrc = 6
local npcp = 8 
local ninprio = npcp
local nprio = 4
local bitrate = 100e6
local srcrate = ((8 * flen) / bitrate / yats.SlotLength) * nsrc / 2

-- Create Sources
src, d2f = {},{}
if false then
  i = 1
  src[i] = yats.bssrc{"src"..i, ex = 100, es  = 200, delta = 2, vci = i, out = {"mux", "in"..i}}
  
  i = 2
  src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
  
  i = 3
  src[i] = yats.listsrc{"src"..i, delta = {1,5,6,2,1,1}, vci = i, rep=true, out = {"mux", "in"..i}}
  
  for i = 4, nsrc do
    src[i] = yats.geosrc{"src"..i, ed = 10, vci = i, out = {"mux", "in"..i}}
  end
else
  for i = 1, nsrc do
    src[i] = yats.bssrc{"src"..i, ex = 400, es  = 200, delta = srcrate, vci = i, out = {"d2f"..i, "dat2fram"}}
--    src[i] = yats.cbrsrc{"src"..i, delta = srcrate, vci = i, out = {"d2f"..i, "dat2fram"}}
    d2f[i] = yats.dat2fram{"d2f"..i, connid=i, pcp=math.mod(i-1, npcp), flen = flen, out = {"fmux", "in"..i}}
  end
end

-- Create Multiplexer
--   servicerate in bits per slot
local servicerate = bitrate * yats.SlotLength
print(string.format("servicerate=%d srcrate=%d slotlength=%f", servicerate, srcrate, yats.SlotLength))
mx = yats.muxFrmPrio{
   "fmux", ninp=nsrc, nprio=nprio, buff=10, servicerate=servicerate, 
   mode="async", out={"demux", "demux"}
}
--   set queue length
mx:setQueueMax(0, 20)
mx:setQueueMax(1, 20)
mx:setQueueMax(2, 10)
mx:setQueueMax(3, 10)

--   define priority mapping: ETH standard Table 8 in 802.1Q for 4 piorities
mx:setPriority(0, 0);
mx:setPriority(1, 0);
mx:setPriority(2, 1);
mx:setPriority(3, 1);
mx:setPriority(4, 2);
mx:setPriority(5, 2);
mx:setPriority(6, 3);
mx:setPriority(7, 3);

--   display some mappings
for i = 0, ninprio-1 do 
   print(string.format("  inprio=%d prio=%d", i, mx:getPriority(i)))
end
for i = 0, nprio-1 do
   print(string.format("  prio=%d len=%d", i, mx:getQueueMax(i)))
end
for i = 1, nsrc do
   print(string.format("  src=%d pcp=inprio=%d", i, d2f[i].pcp))
end

-- Create Demultiplexer
--    we first create the list of outputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, frametype=true, out = dout}

local x,y,dx,dy = 0, 0, 200, 100

-- Create Lines and Sinks
lin, snk, meas, hist = {}, {}, {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas3"}}
  snk[i] = yats.sink{"sink"..i}
  meas[i] = yats.meas3{"meas"..i, connid = {i,i}, ctd = {0,1000}, ctddiv = 10, out = {"sink"..i, "sink"}}
  hist[i] = yats.histo{"hist"..i, title = "CONNID="..i..": FTD", 
    val = {meas[i], "CTD", i}, win = {x,y,dx,dy}, delta=1000, display=false
  }
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

disp = yats.bigdisplay {
  "disp", title="IAT", nrow = 6, ncol = 2, width = 200, height = 100, 
  instruments = {
    {nil, hist[1]},
    {nil, hist[2]},
    {nil, hist[3]},
    {nil, hist[4]},
    {nil, hist[5]},
    {nil, hist[6]},
  }
}
disp:show()
disp:setAttribute("polystep", 1)

-- Connect the network
yats.sim:connect()

-- Display demux switch table
local st, entries = dmx:getRouting()
print("Demux switch table")
print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
  print("  (from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Run simulation: 1. path
yats.sim:run(500000*2, 10000)

local result = {}

-- FINAL DISPLAY
print("Cell Transfer Delay pr VC:")
for i = 1, nsrc do
  print(string.format("  CTD %d: mean=%d", i, meas[i]:getMeanCTD(0)))
end
print("  Losses per input:")
for i = 1, nsrc do
   print("    Loss "..i..": "..mx:getLoss(i))
   table.insert(result, mx:getLoss(i))
end
print("  Losses per priority code point")
for i = 0, ninprio-1 do
  print("    LossINPRIO "..i..": "..mx:getLossINPRIO(i))
  table.insert(result, mx:getLossINPRIO(i))
end
print("  Losses per priority queue")
for i = 0, nprio-1 do
  print("    LossPRIO "..i..": "..mx:getLossPRIO(i))
  table.insert(result, mx:getLossPRIO(i))
end

-- Display source counters.
print("Source cell counters:")
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