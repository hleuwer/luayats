-- Adopt LUA_PATH, so we find the modules.
TK_SIMTIME = 2
TK_NRUN = 100
-- Give higher level for more debugging output
print("package.path="..package.path)
require "yats.stdlib"
require "yats.agere"
require "examples/ageretm.netz"
require "yats.muxdmx"
require "yats.misc"
require "yats.src"
require "examples/ageretm.macro"
require "yats.user"
require "yats.graphics"
-----------------------------------------------------------------------------
-- Simulation Control
-----------------------------------------------------------------------------
yats.sim:SetRand(10)
yats.sim:ResetTime()
-- Output Setup
-- set: display graphs; nil: do not display graphs
display = 1
local defattrib = {
   drawoper = cd.REPLACE,
   drawcolor = cd.BLUE,
   drawstyle = cd.CONTINUOUS,
   drawwidth = 1,
   textoper = cd.REPLACE,
   texttypeface = "Helvetica",
   textface = cd.BOLD,
   textcolor = cd.BLUE,
   textsize = 8,
   bgopacity = cd.TRANSPARENT,
   bgcolor = cd.YELLOW
}
-- set: display dots and messages; nil: be quiet
messages = 1

-- Slot Setup
SlotLength = 1e-6 
yats.SlotBitRate = 53*8/SlotLength  
print("SlotLength = " .. SlotLength)
yats.sim:setSlotLength(SlotLength)
slots = TimeToSlot(tonumber(os.getenv("TK_SIMTIME")) or TK_SIMTIME or error("TK_SIMTIME not set."))
nrun = nrun or tonumber(os.getenv("TK_NRUN") or  TK_NRUN or error("TK_NRUN not set."))
		
-- Traffic Manager general setup
-- Maximum output rate (physical rate)
ShapingRate = 1e6
-- bgsource = 0
udpsrc = 3
nsrc = udpsrc
nTrafficQueue = nsrc
nPolicer = nsrc
packetlength = 1500
maxbuff_packet = 10
maxbuff_byte = maxbuff_packet * packetlength
tcpwnd = 65536
MinimumRates = {
  1e4, 1e6, 2e6, 3e4, 1e5, 1.5e6
}

-----------------------------------------------------------------------------
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Network Setup
--
--  cell-source => framegen => framemark => line                             => meas => sink
--  cell-source => framegen => framemark => line => aggr_meas => TM => demux => meas => sink
--  cell-source => framegen => framemark => line =>                          => meas => sink
-----------------------------------------------------------------------------
-- Source, Frame Generator and Frame Marker
src, d2f, fm = {},{},{}
for i = 1, udpsrc do
  local udpsourcerate = 1e6
  local delta = TimeToSlot(packetlength * 8 / udpsourcerate)
  local EX = 1
  local ES = EX * packetlength*7/(udpsourcerate*SlotToTime(1))-EX
  src[i] = yats.cbrsrc{
    "src"..i, 
    delta = delta, 
    vci = i, 
--    out = {"d2f"..i, "dat2fram"}
    out = {"d2f"..i}
  }
  d2f[i] = yats.dat2fram{
    "d2f"..i, 
    flen=packetlength, 
    connid = i, 
--    out = {"fm"..i, "framemarker"}
    out = {"fm"..i}
  }
  fm[i] = yats.framemarker{
    "fm"..i, 
    vlanId = i, 
    vlanPriority = 0, 
--    out = {"dcline"..i, "line"}
    out = {"dcline"..i}
  }
end

-- Line
dcl = {}
for i = 1, nsrc do
  dcl[i] = yats.line{
    "dcline"..i, 
    delay = TimeToSlot(i*10e-3), 
    out = {"tm","in"..i}
  }
end

-- Traffic Manager
tm = yats.AgereTm{
  "tm",
  ninp = nsrc,
  nTrafficQueue = nTrafficQueue,
  nPolicer = nPolicer,
  maxconn = nsrc,
  maxbuff_p = 10 * maxbuff_packet,
  maxbuff_b = 10 * maxbuff_byte,
  shapingrate = ShapingRate,
  out = {"ipmeas_aggr", "meas3"}
}

-- Basic SDWRR parameters
tm:SchedSdwrrSetServiceQuantum(5*1500);
tm:SchedSdwrrSetMaxPduSize(1500);

-- TQ maximum # of packet buffers
for i = 1, nTrafficQueue  do
  tm:TqSetMaxBuff_p(i, maxbuff_packet)
end

-- Minimum rates
for i = 1, nsrc do
  tm:TqSetMinimumRate(i,MinimumRates[i]);
end

print("Parameters of traffic queues:")
print("  serviceQuantum: " .. tm.scheduler.serviceQuantum)
print("  maxPduSize    : " .. tm.scheduler.maxPduSize)
print("  maxWeight     : " .. tm.scheduler.maxWeight)
for i = 1, nTrafficQueue do
  print("  tq["..i.."].limit1: " .. tm.TrafficQueue[i].limit1)
  print("  tq["..i.."].limit2: " .. tm.TrafficQueue[i].limit2)
  print("  tq["..i.."].limit3: " .. tm.TrafficQueue[i].limit3)
end

-- Assign queues and policing instances
for i = 1, nsrc do
  tm:ConnSetDestinationQueue(i, i - 1);
  tm:ConnSetDestinationPolicer(i, i - 1);
end
for i = 1, nPolicer do
  tm:PolicerSetRate(i, 2e5, 8 * packetlength, 4e5 , 3 * packetlength);
  tm:PolicerSetAction(i, 0)	-- 0-nothing,1-marking,2-CIRdrop,3-PIRdrop
end

-- Create aggregate rate measurement
ipmeas_aggr = yats.meas3{
  "ipmeas_aggr", 
  ctd = {0,100}, 
  connid = {1,nsrc},
  ctddiv = TimeToSlot(1/100),
--  out = {"tcpdemux", "demux"}
  out = {"tcpdemux"}
}

-- Create EVPL demux
local dout = {}
-- output pins
for i = 1, nsrc do 
  dout[i] = {"ipmeas"..i} 
end
tcpdemux = yats.demux{
  "tcpdemux", 
  frametype = true, 
  maxvci = nsrc, 
  nout = nsrc,
  out = dout
}

-- Establish cross connections
for i = 1,nsrc do
  tcpdemux:signal{i,i,i}
end

-- Create single measurements and the sinks
ipmeas, sink = {}, {}
for i = 1, nsrc do
  ipmeas[i] = yats.meas3{
    "ipmeas"..i, 
    ctd = {0,500}, 
    ctddiv = TimeToSlot(1/100),
    connid = {i,i},
    out = {"datasink"..i,"sink"} 
  }
  sink[i] = yats.sink{"datasink"..i}
end

-----------------------------------------------------------------------------
-- Display Setup
-----------------------------------------------------------------------------
local meterudp, metertm, metertq = {},{},{}
if display then

  local left
  local ddelta = 1000
  local dupdate = ddelta/10
  local height = 100
  local dup = 130
  local width, brdx, leftblock, upblock = 150, 5, 10, 200
  local up, left = upblock, leftblock
  local doit = 1
  local doitc = 3


  up = upblock
  for i = 1, nsrc do
    -- source's running packet count
    left = leftblock

    meterudp[i] = yats.meter{
      "meter_udp"..i, 
      val={src[i], "Count"}, 
      win={left, up, width, height}, 
      mode=yats.DiffMode, 
      drawmode = yats.drawmode.yall, 
      nvals = 100,
      maxval = 20, 
      delta=ddelta, 
      update=dupdate, 
      title = "udp"..i..".Count",
      attrib=defattrib,
      display=false
    }
    -- traffic manager's running packet losses
    left = left + width + brdx

    metertm[i] = yats.meter{
      "meter_tm"..i, 
      val={tm,"conn["..i.."].lost_p"}, 
      win={left, up, width, height},
      mode=yats.DiffMode, 
      drawmode = yats.drawmode.yall, 
      nvals = 1000, 
      linemode=1, 
      linecolor=1,
      maxval = 10, 
      delta= ddelta, 
      update=dupdate, 
      title = "tm.conn["..i.."].lost_p",
      attrib=defattrib,
      display=false
    }
    up = up + dup
  end

  leftblock = left + width + brdx
  up = upblock

  for i = 1, nTrafficQueue do
    -- traffic manager's buffers per traffic queue
    left = leftblock

    metertq[i] = yats.meter{
      "meter_tq"..i, 
      val={tm, "tq["..(i-1).."].buff_p"}, 
      win = {left, up, width, height},
      mode=yats.AbsMode, 
      drawmode=yats.drawmode.yall, 
      nvals = 1000, 
      maxval=2*maxbuff_packet, 
      delta = ddelta,
      update = dupdate, 
      title = "tq["..(i-1).."].buff_p",
      attrib=defattrib,
      display=false
    }
    up = up + dup
  end

  leftblock = left + width + brdx
  up, left = upblock, leftblock

  -- traffic manager's total buffer space

  meterbuff = yats.meter{
    "meterbuff", val={tm, "buff_p"}, 
    win={left,up,width,height},
    mode=yats.AbsMode,
    nvals=1000,
    maxval=4*maxbuff_packet, 
    delta=ddelta, 
    update=dupdate,
    title = "tm.buff_p",
    attrib = defattrib,
    display=false
  }
  left = left+width+brdx
  
  -- Aggregate rate

  meteraggr = yats.meter{
    "meteraggr", 
    val={ipmeas_aggr,"FrameRate"}, 
    win={left,up,width,height},
    nvals = 1000, 
    maxval = 2 * ShapingRate, 
    delta = ddelta, 
    update = dupdate, 
    mode = yats.AbsMode,
    drawmode = yats.drawmode.yall, 
    title="Aggr Rate",
    attrib = defattrib,
    display=false
  }
  left = left + width + brdx
  leftblock = left
  up = upblock

  metermean, meterctd, histo = {}, {}, {}

  for i = 1,nsrc do
    left = leftblock
    -- mean CTD
    meterctd[i] = yats.meter{
      "meterctd"..i, 
      val={ipmeas[i], "MeanCTD", i}, 
      win={left,up,width,height},
      mode = yats.AbsMode, 
      drawmode=yats.drawmode.yall, 
      nvals = 1000, 
      maxval = TimeToSlot(100), 
      delta = ddelta,
      update=dupdate, 
      title="upd["..i.."].ctd",
      attrib=defattrib,
      display=false
    }
    left = left + width + brdx
    -- connection's frame rate

    metermean[i] = yats.meter{
      "metermean"..i, 
      val={ipmeas[i], "FrameRate"}, 
      win={left,up,width,height},
      mode = yats.AbsMode, 
      drawmode=yats.drawmode.yall, 
      nvals = 1000, 
      maxval = 1.5e6, 
      delta = ddelta,
      update = dupdate, 
      title = "udp["..i.."].FrameRate",
      attrib=defattrib,
      display=false
    }
    left = left + width + brdx


    histo[i] = yats.histo{
      "histo"..i, 
      val={ipmeas[i],"CTD", i},
      win={left,up,width,height},
      delta=ddelta,
      title="CTD Dist",
      attrib=defattrib,
      display=false
    }
    up = up + dup
  end


end

-- Display cross connect table
print("tcpdemux switch table:")
local st, entries
st, entries = tcpdemux:getRouting()
print("  table has "..entries.." entries.")
for i = 1, table.getn(st) do
  print("  (from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

-- Show the bigdisplay
--title, nrow, ncol, width, height, attrib, cinstrs
if true then
bigdisp = yats.bigdisplay{
  "bigdisp",
  title="AGERETM", nrow=3, ncol=8, width=100, height=100, attrib=defattrib,
  instruments = {
    {meterudp[1], metertm[1], metertq[1], nil,      nil,       meterctd[1], metermean[1], histo[1]},
    {meterudp[2], metertm[2], metertq[2], nil,      meteraggr, meterctd[2], metermean[2], histo[2]},
    {meterudp[3], metertm[3], metertq[3], nil,      nil,       meterctd[3], metermean[3], histo[3]}
  }
}
--bigdisp:show()
else
  bigdisp = {
    yats.bigdisplay{
      "bigdisp-1",
      title="INGRESS", nrow=3, ncol=6, width=100, height=100, attrib=defattrib,
      instruments= {
	{meterudp[1], metertm[1], metertq[1]},
	{meterudp[2], metertm[2], metertq[2]},
	{meterudp[3],metertm[3], metertq[3]}
      }
    },
    yats.bigdisplay{
      "bigdisp-2",
      title="BUFFER", nrow=4, ncol=6, width=100, height=100, attrib=defattrib,
      instruments = {
	{meterbuff, meteraggr}
      }
    },
    yats.bigdisplay{
      "bigdisp-3",
      title="EGRESS", nrow=4, ncol=6, width=100, height=100, attrib=defattrib,
      instruments = {
	{meterctd[1], metermean[1], histo[1]},
	{meterctd[2], metermean[2], histo[2]},
	{meterctd[3], metermean[3], histo[3]}
      }
    }
  }
  
  for i,v in ipairs(bigdisp) do v:show() end
  
end

-- Now we can run, we do it in pieces.
sslots = 0
local m1, m2
for n = 1, nrun do
   if messages  then 
      print("\nPartial Run "..n.." with "..slots.." slots ("..yats.SimTimeReal.." sec).") 
   end
   if n == 3 then
      -- meterbuff => (2,2), get meter_tm
      m1 = bigdisp:attach(meterbuff, 2, 2)
      -- meter_tm ==> (2,4), get nothing
      bigdisp:attach(m1, 2, 4)
   elseif n == 5 then
      -- get meterbuff
      m1 = bigdisp:detach(2,2)
      -- put it to 2,4 and get meter_tm
      m1 = bigdisp:attach(m1, 2, 4)
      -- put meter_tm to it's original place
      bigdisp:attach(m1, 2, 2)
   end
   if messages  then
      yats.sim:run(slots, slots/10)
   else
      yats.sim:run(slots, slots+1)
   end
   sslots = sslots + slots
end

if messages  then
  print("finished: "..sslots/1e6.." Mio slots")
  print("real-time: "..yats.SimTimeReal.." sec.")
end
result = {}
local k = 1
for i = 1, nsrc do
  local h = histo[i]
  for j = 1, h.nvals do
    result[k] = h:getVal(j-1)
    k = k + 1
  end
end
return result