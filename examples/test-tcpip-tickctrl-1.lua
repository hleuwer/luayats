require "yats.stdlib"
require "yats.tcpip"
require "yats.polshap"
require "yats.graphics"
require "yats.user"

--                           Meter         Meter                Meter
--                           Window        Framerate   Delay    QLen, Loss
--                                                              
--  CBRFRAME ---data-------> TCPIPSEND --> MEASURE --> LINE --> SHAPER --> TCPIPREC -----> TICKCTRL --> SINK
--           <--start/stop--     /|\                                          |     <--start/stop--
--                                |                                           |
--                                :------------------Ack----------------------'
--

-- Utils
local slots = yats.time2slot
local kHz, ms = 1e3, 1e-3
local Mbps, MHz, us = 1e6, 1e6, 1e-6
local GHz, ns = 1e9, 1e-9

-- Init the yats random generator.
printf("Init random generator.\n")
yats.sim:SetRand(1)
printf("slot length is %f\n", yats.SlotLength)
printf("Setting slot length.\n")
yats.sim:setSlotLength(1e-6)
printf("slot length is %f\n", yats.SlotLength)
--yats.kernel_log:setLevel("DEBUG")
-- Parameters to play with
-- SOURCE
local sendrate = 4*8*Mbps
-- TCP SEND
local mtu = 1500         -- MTU size of sender
local mss = mtu-40       -- TCP max segment size
local buf = 65535        -- sender buffer
local nagle = true       -- fill MTU yes/no
local fretr = true       -- fast retransmit
local simtime = 20       -- 10s
local bitrate = 1*1e6    -- obsolete
local blocksize = 4096    -- Blocksize with which TCP sender is feeded
local txproctime = 300*us -- transmitter stack speed
local tcptick = 500*ms   -- Sender tick
local logretr = true    -- Log retries

-- TRANSMISSION LINE
local tdelay = 1*us    -- delay of transmission line

-- TCP REC:
local ackdel = 200*ms         -- acknowledge delay (ack sent no later than that!)
local rxproctime = 100*us -- receiver stack speed
local wnd = 32000        -- receiver window

-- SHAPER:
local shapedrate = 50*1e6 -- 
local qlen = 5          -- Queue Length of shaper

-- SOFTWARE - tickctrl
local appstop = 500
local appstart = 400
local contend = 30       -- Number of contending processes
--local appstart = 1     -- Time when application starts

-- graphics
local win = {100,100,600,300}

-- derived from above
local delay = slots(tdelay)     -- 1 ms
local ncycles = slots(simtime)  -- in slots    
local senddelay = slots(blocksize*8/sendrate)

-- A frame source generating n packets back to back - backpressured.
-- Simply feeds the TCP sender. Packet size is of no meaning.
local src = yats.cbrframe{
  "src", delta = senddelay, len = blocksize, start_time = 1, 
  out = {"tcps", "data"}
}

-- TCP sender with various parameters
local tcps = yats.tcpipsend{"tcps", 
  buf = buf, bstart = buf - blocksize, mtu = mss+40, ts = true, nagle = nagle,
  fretr = fretr, bitrate = bitrate, proctim = txproctime, tick = tcptick, rtomin = 1.5,
  oqwm = 2, rec = "tcpr", logretr = logretr,
  out = {
    {"ms", "meas2"},
    {"src", "ctrl"}
  }
}
-- Measurement device measuring the frame rate at the output of the TCP sender
local ms = yats.meas2{"ms", maxctd = 10, maxiat = 10, 
  out = {"line", "line"}
}

-- A delay line
local line = yats.line{"line", delay = delay, 
  out = {"shaper", "shap2"}
}

-- A shaper slowing down the connection and loosing data
local delta = (8 * mtu /  shapedrate) / yats.SlotLength
local shaper = yats.shap2{"shaper", delta = delta, buff = qlen, 
  out = {"tcpr", "data"}
}

-- TCP receiver
local tcpr = yats.tcpiprec{"tcpr",
   wnd = wnd, proctim = rxproctime, ackdel = ackdel, 
   out = {{"app", "in"},{ "lineack", "line"}}
}
-- Line for acknowledge frames
local lineack = yats.line{"lineack", delay = delay, 
   out = {"msack", "meas2"}
}

-- Distribution for application
local dgeo = yats.distrib{"dgeo", dist=yats.distrib.geometric, distargs={e=8000}}
local dbin = yats.distrib{"dbin", dist=yats.distrib.binomial, distargs={n=20, p=0.5}}
local dtab = yats.distrib{"dtab", 
  dist={
     {5000, 0.9}, {15000, 0.1}
--     {10000, 0.1}
  }
}
-- Application
local app = yats.tickctrl{"app",
   dist = dtab, xoff = appstop, xon = appstart, contproc = contend, phase = 1,
   out = {nc(), {"tcpr", "start"}}
}

-- Measurement of ack packet rate
local msack = yats.meas2{"msack", maxctd = 100, maxiat = 100, 
  out = {"tcps", "ack"}
}

-- Frame rate display
local mload = yats.meter{"mload", title = "framerate", 
  win = win, mode=yats.DiffMode, nvals = 400, maxval = 10, delta = 1000, update=10,
  val = {ms, "Count"}, display = false
}

-- TCP window display
local mcwnd = yats.meter{"mcwnd", title = "window", 
  win = win, mode=yats.AbsMode, nvals = 400, maxval = 80000, delta = 1000, update=10,
  val = {tcps, "CWND"}, display = false
}

-- Shaper's queue length display
local mqlen = yats.meter{"mqlen", title = "queue length", 
  win = win, mode=yats.AbsMode, nvals = 400, maxval = 25, delta = 1000, update=10,
  val = {shaper, "QLen"}, display = false
}

-- Shaper's loss display
local mloss = yats.meter{"mloss", title = "loss", 
  win = win, mode=yats.DiffMode, nvals = 400, maxval = 10, delta = 1000, update=10,
  val = {shaper, "Count"}, display = false
}

-- Applications queue length
local mapp = yats.meter{"mappq", title = "queue length", 
  win = win, mode=yats.AbsMode, nvals = 400, maxval = appstop*1.5, delta = 1000, update=10,
  val = {app, "QLen"}, display = false
}

-- Bigdisplay for all the single displays above
local disp = yats.bigdisplay{
  "disp", title = "TCP Monitor", nrow = 5, ncol = 1, width = 600, height = 100,
  instruments = {
    {mload},
    {mcwnd},
    {mqlen},
    {mloss},
    {mapp}
  }
}
disp:show()
disp:setattrib("usepoly", 1)
disp:setattrib("polytype", yats.CD_OPEN_LINES)
disp:setattrib("polystep", 1)
disp:setattrib("drawwidth", 1)

-- Connect and go
yats.sim:connect()
local ncyc,n = 5e4,0
local dostep = true
local cont = false
while n < ncycles do  

   if cont == false then
      local sstep = string.format("ticks=%d dT=%.3f us time=%.3f", ncyc, ncyc*yats.SlotLength/us,  yats.SimTimeReal)
      local b = iup.Alarm("ATTENTION", sstep, "Step", "Continue", "Stop")
      if b == 1 then
	 dostep = true
      elseif b == 2 then
	 cont = true
	 dostep = true
      else
	 break
      end
   end
   if dostep then
      yats.sim:run(ncyc)
      dostep = false
   end
   yats.sim:run(ncyc)

   --  print("ms.count = ", ms.counter)
   n = n + ncyc
end

printf("GENERIC:\n")
printf("Slotlength: %.3f us\n", yats.SlotLength/us)
printf("Simtime: %d %.3f s\n", yats.SimTime, yats.SimTimeReal)
printf("line delay = %.3f us\n", tdelay/us)
print()
print("SOURCE:")
printf("Sendrate: %.3f Mbps %d slots\n", sendrate/Mbps, senddelay)
print()
printf("TRANSMITTER:\n")
printf("tx_bytes: %d bytes\n", tcps.xmitted_bytes)
printf("tx_user : %d bytes\n", tcps.xmitted_user_bytes)
printf("rx_bytes: %d bytes\n", tcps.received_bytes)
printf("retransmitted bytes: %d bytes\n", tcps.rexmitted_bytes)
printf("ack packets: %d\n", tcps.received_acks)
printf("retransmit timeout : %d\n", tcps.rexmto)
printf("userrate: %.3f Mbit/s\n", tcps.xmitted_user_bytes * 8 / yats.SimTimeReal / MHz)
print()
printf("RECEIVER:\n")
printf("rx_bytes: %d bytes\n", tcpr.arrived_bytes)
printf("rx_user : %d bytes\n", tcpr.user_bytes)
printf("rx_valid: %d bytes\n", tcpr.arrived_valid_bytes)
printf("user packets: %d\n", tcpr.user_packets)
printf("ack packets : %d\n", tcpr.ack_cnt)
printf("sdu delay: %.3f us\n", tcpr.SDU_delay_mean * yats.SlotLength / us)
printf("pdu delay: %.3f us\n", tcpr.PDU_delay_mean * yats.SlotLength / us)
printf("user rate: %.3f Mbit/s\n", tcpr.user_bytes * 8 / yats.SimTimeReal / 1e6)
print()
printf("SHAPER:\n")
printf("Loss: %d packets\n", shaper.counter)
printf("Rate: %.3f MBit/s\n", shapedrate*us)
--printf("bw * delay = %.3f\n", shapedrate * tdelay * 2)

result = {}

table.insert(result, tcps.xmitted_bytes)
table.insert(result, tcps.xmitted_user_bytes)
table.insert(result, tcps.received_bytes)
table.insert(result, tcpr.arrived_bytes)
table.insert(result, tcpr.user_bytes)
table.insert(result, tcpr.user_packets)
table.insert(result, tcpr.ack_cnt)

return result