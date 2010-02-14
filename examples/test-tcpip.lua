require "yats.stdlib"
require "yats.tcpip"
require "yats.polshap"
require "yats.graphics"

--                           Meter         Meter                Meter
--                           Window        Framerate   Delay    QLen, Loss
--                                                              
--  CBRFRAME ---data-------> TCPIPSEND --> MEASURE --> LINE --> SHAPER --> TCPIPREC --> SINK
--           <--start/stop--     /|\                                          | 
--                                |                                           |
--                                :------------------Ack----------------------'
--

-- Utils
local slots = yats.time2slot

-- Init the yats random generator.
printf("Init random generator.\n")
yats.sim:SetRand(1)
printf("slot length is %f\n", yats.SlotLength)
printf("Setting slot length.\n")
yats.sim:setSlotLength(1e-6)
printf("slot length is %f\n", yats.SlotLength)
--yats.kernel_log:setLevel("DEBUG")
-- Parameters to play with
local mtu = 1500
local mss = mtu-40
local wnd = 17500
local buf = 65535
local nagle = true
local fretr = true
local win = {300,300,600,300}
local simtime = 10            -- 10s
local ncycles = slots(simtime)     
local tdelay = 0.0003
local delay = slots(tdelay)    -- 1 ms
local bitrate = 1*1e6        -- bit/s
local shapedrate = 50*1e6
local blocksize = 4096
local txproctime = 100e-6
local rxproctime = txproctime
local ackdel = 200e-3
local tcptick = 500e-3
local qlen = 5
local logretr = false
-- A frame source generating n packets back to back - backpressured.
-- Simply feeds the TCP sender. Packet size is of no meaning.
local src = yats.cbrframe{
  "src", delta = 1, len = blocksize, start_time = 1, 
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
  out = {nc(),{ "lineack", "line"}}
}
-- Line for acknowledge frames
local lineack = yats.line{"lineack", delay = delay, 
  out = {"msack", "meas2"}
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

-- Bigdisplay for all the single displays above
local disp = yats.bigdisplay{
  "disp", title = "TCP Monitor", nrow = 4, ncol = 1, width = 300, height = 100,
  instruments = {
    {mload},
    {mcwnd},
    {mqlen},
    {mloss}
  }
}
disp:show()
disp:setattrib("usepoly", 1)
disp:setattrib("polytype", cd.OPEN_LINES)
disp:setattrib("polystep", 1)
disp:setattrib("drawwidth", 1)

-- Connect and go
yats.sim:connect()
local ncyc,n = 1e4,0
while n < ncycles do  
  yats.sim:run(ncyc)
--  print("ms.count = ", ms.counter)
  n = n + ncyc
end

printf("TRANSMITTER:\n")
printf("tx_bytes: %d bytes\n", tcps.xmitted_bytes)
printf("tx_user : %d bytes\n", tcps.xmitted_user_bytes)
printf("rx_bytes: %d bytes\n", tcps.received_bytes)
printf("retransmitted bytes: %d bytes\n", tcps.rexmitted_bytes)
printf("ack packets: %d\n", tcps.received_acks)
printf("retransmit timeout : %d\n", tcps.rexmto)
printf("userrate: %.3f Mbit/s\n", tcps.xmitted_user_bytes * 8 / simtime / 1e6)
print()
printf("RECEIVER:\n")
printf("rx_bytes: %d bytes\n", tcpr.arrived_bytes)
printf("rx_user : %d bytes\n", tcpr.user_bytes)
printf("rx_valid: %d bytes\n", tcpr.arrived_valid_bytes)
printf("user packets: %d\n", tcpr.user_packets)
printf("ack packets : %d\n", tcpr.ack_cnt)
printf("sdu delay: %.3f s\n", tcpr.SDU_delay_mean * yats.SlotLength)
printf("pdu delay: %.3f s\n", tcpr.PDU_delay_mean * yats.SlotLength)
print()
printf("user rate: %.3f Mbit/s\n", tcpr.user_bytes * 8 / simtime / 1e6)
printf("delay = %.3f s\n", delay)
printf("bw * delay = %.3f\n", shapedrate * tdelay * 2)

result = {}

table.insert(result, tcps.xmitted_bytes)
table.insert(result, tcps.xmitted_user_bytes)
table.insert(result, tcps.received_bytes)
table.insert(result, tcpr.arrived_bytes)
table.insert(result, tcpr.user_bytes)
table.insert(result, tcpr.user_packets)
table.insert(result, tcpr.ack_cnt)

return result