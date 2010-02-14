-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 -> dat2frm -> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 -> dat2frm -> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 -> dat2frm -> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 -> dat2frm -> | |    | |--> line_4 --> sink_4
-- cbrsrc_n -> dat2frm -> |/      \|--> line_n --> sink_n
--                        mux    demux

yats.kernel_log:setLevel("DEBUG")
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
local nsrc = 4
local npcp = 8 
local ninprio = npcp
local nprio = 4
local bitrate = 100e6
local srcrate = ((8 * flen) / bitrate / yats.SlotLength) * nsrc * 0.9
local agetime = 30/yats.SlotLength
--------------------------------
-- Create Sources
--------------------------------
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
    src[i] = yats.bssrc{"src"..i, ex = 400, es = 200, delta = srcrate, vci = i, out = {"d2f"..i, "dat2fram"}}
--    src[i] = yats.cbrsrc{"src"..i, delta = srcrate, vci = i, out = {"d2f"..i, "dat2fram"}}
  end
end
for i = 1, nsrc do
   d2f[i] = yats.dat2fram{"d2f"..i, connid=i, smac = 100+i, dmac = 200+i, flen = flen, out = {"bridge", "p"..i}}
end
--------------------------------
-- Create eth bridge
--   servicerate in bits per slot
--------------------------------
local servicerate = bitrate * yats.SlotLength
print(string.format("servicerate=%d srcrate=%d slotlength=%f", servicerate, srcrate, yats.SlotLength))

local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end

print("CREATE BRIDGE")
bridge = yats.ethbridge{
   "bridge", nports = nsrc, nprio=nprio, ninprio = ninprio, servicerate=servicerate,
   ndb = 1, agetime = agetime, out = dout
}
for i = 1, nsrc do
   print("  ", bridge:getMux(i), bridge:getMux(i).setQueueMax)
end
--------------------------------
-- Setup the bridge
--------------------------------
print("SETUP BRIDGE")
--   set queue length
for i = 1, nsrc do
   print("  PORT"..i)
   bridge:setQueueMax(i, 0, 30)
   bridge:setQueueMax(i, 1, 30)
   bridge:setQueueMax(i, 2, 10)
   bridge:setQueueMax(i, 3, 10)
   if false then
   --   define priority mapping: ETH standard Table 8 in 802.1Q for 4 piorities
   bridge:setPriority(i, 0, 0);
   bridge:setPriority(i, 1, 0);
   bridge:setPriority(i, 2, 1);
   bridge:setPriority(i, 3, 1);
   bridge:setPriority(i, 4, 2);
   bridge:setPriority(i, 5, 2);
   bridge:setPriority(i, 6, 3);
   bridge:setPriority(i, 7, 3);
   end
end

--------------------------------
--   display some mappings
--------------------------------
print("MAPPINGS")
for j = 1, nsrc do
   for i = 0, nprio-1 do
      print(string.format("  port=%d prio=%d len=%d", j, i, bridge:getQueueMax(j, i)))
   end
   if false then
   for i = 0, ninprio-1 do 
      print(string.format("  port=%d inprio=%d prio=%d", j, i, bridge:getPriority(j, i)))
   end
end
   for i = 1, nsrc do
      print(string.format("  src=%d smac=%x pcp=inprio=%d", i, d2f[j].smac, d2f[j].pcp))
   end
end

local x,y,dx,dy = 0, 0, 200, 100

-- Create Lines and Sinks
lin, snk, meas, hist = {}, {}, {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas3"}}
  snk[i] = yats.sink{"sink"..i}
  meas[i] = yats.meas3{"meas"..i, connid = {1,nsrc}, ctd = {0,1000}, ctddiv = 10, out = {"sink"..i, "sink"}}
  hist[i] = yats.histo{"hist"..i, title = "PORT="..i..": FTD", 
    val = {meas[i], "CTD", i}, win = {x,y,dx,dy}, delta=1000, display=false
  }
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
print("CONNECTING")
yats.sim:connect()

-- FINAL DISPLAY
print("Input Counters")
for i = 1, nsrc do
   print(string.format("  port %d: unicast  =%d", i, bridge:getInUCount(i)))
   print(string.format("  port %d: multicast=%d", i, bridge:getInMCount(i)))
   print(string.format("  port %d: broadcast=%d", i, bridge:getInBCount(i)))
   print(string.format("  port %d: flood    =%d", i, bridge:getInFCount(i)))
end

-- Run simulation: 1. path
yats.sim:run(50000, 100000)

-- Configure static entries => simulate learning
bridge:addmac(0, 201, "1000", agetime)
bridge:addmac(0, 202, "0001", agetime)
bridge:addmac(0, 203, "0010", agetime)
bridge:addmac(0, 204, "0010", agetime)

-- Run simulation: 2. path
yats.sim:run(500000, 10000)

result = {}

-- FINAL DISPLAY
print("Input Counters")
for i = 1, nsrc do
   print(string.format("  port %d: unicast  =%d", i, bridge:getInUCount(i)))
   print(string.format("  port %d: multicast=%d", i, bridge:getInMCount(i)))
   print(string.format("  port %d: broadcast=%d", i, bridge:getInBCount(i)))
   print(string.format("  port %d: flood    =%d", i, bridge:getInFCount(i)))
end

print("Frame Transfer Delay per Port:")
for i = 1, nsrc do
  print(string.format("  CTD %d: mean=%d", i, meas[i]:getMeanCTD(0)))
end
print("  Losses per output and input port:")
for i = 1, nsrc do
   for j = 1, nsrc do
      print("    Loss "..i..","..j..": "..bridge.omux[i]:getLoss(j))
      table.insert(result, bridge.omux[i]:getLoss(j))
   end
end
print("  Losses per port and priority queue")
for i = 1, nsrc do
   for j = 0, nprio-1 do
      print("    LossPRIO "..i..","..j..": "..bridge.omux[i]:getLossPRIO(j))
      table.insert(result, bridge.omux[i]:getLossPRIO(j))
   end
end

-- Display source counters.
print("Source cell counters:")
for i = 1, nsrc do
  print("  "..src[i].name.." count: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end

-- Display meas counter.
print("Measurement frame counters:")
for i = 1, nsrc do
  print("  "..meas[i].name.." count: "..meas[i]:getCounter())
  table.insert(result, meas[i]:getCounter())
end

-- Display sink counter.
print("Sink frame counters:")
for i = 1, nsrc do
  print("  "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end
return result