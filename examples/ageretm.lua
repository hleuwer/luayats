-- Adopt LUA_PATH, so we find the modules.
TK_SIMTIME = 2
TK_NRUN = 100
require "yats.core"
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
display = display or true
-- set: display dots and messages; nil: be quiet
messages = message or true

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
  if false then
    src[i] = yats.cbrsrc{
      "src"..i, 
      delta = delta, 
      vci = i, 
      out = {"d2f"..i}
    }
  else
    src[i] = yats.bssrc{
      "src"..i,
      ex = EX,
      es = ES,
      delta = 1,
      vci = i, 
      out = {"d2f"..i}
    }
  end
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
if display == true then

  local left
  local ddelta = 1000
  local dupdate = ddelta/10
  local height = 100
  local dup = 130
  local width, brdx, leftblock, upblock = 150, 5, 10, 200
  local up, left = upblock, leftblock
  local doit = 1
  local doitc = 3


  local meterudp, metertm = {},{}
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
      title = "udp"..i..".Count"
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
      title = "tm.conn["..i.."].lost_p"
    }
    up = up + dup
  end

  leftblock = left + width + brdx
  up = upblock

  local metertq = {}
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
      title = "tq["..(i-1).."].buff_p"
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
    title = "tm.buff_p"
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
    title="Aggr Rate"
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
      title="upd["..i.."].ctd"
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
      title = "udp["..i.."].FrameRate"
    }
    left = left + width + brdx

    histo[i] = yats.histo{
      "histo"..i, 
      val={ipmeas[i],"CTD", i},
      win={left,up,width,height},
      delta=ddelta,
      title="CTD Dist"
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

-- Now we can run, we do it in pieces.
sslots = 0
for n = 1, nrun do
  if messages  then 
    print("\nPartial Run "..n.." with "..slots.." slots ("..yats.SimTimeReal.." sec).") 
  end
  if messages  then
    yats.sim:run(slots, slots/10)
  else
    yats.sim:run(slots, slots+1)
  end
  sslots = sslots + slots
--  for i = 1,nsrc do
--  end
end
result = {}
if display == true then
  local k=1
  for i = 1, nsrc do
    local h = histo[i]
    for j = 1, h.nvals do
      result[k] = h:getVal(j-1)
      k = k + 1
    end
  end
end
if messages  == true then
  print("finished: "..sslots/1e6.." Mio slots")
  print("real-time: "..yats.SimTimeReal.." sec.")
end
return result