require "yats.stdlib"
require "yats.core"
require "yats.n23"
require "yats.graphics"
require "yats.user"
local fmt = string.format

-- Helper: units
local ms, us, ns = 1e-3, 1e-6, 1e-9
local Gbps, Mbps, kbps = 1e9, 1e6, 1e3

-- Fequently used functions
local t2s, r2s, s2t = yats.t2s, yats.r2s, yats.s2t

-- Init Simulation time
yats.sim:setRand(1)
yats.sim:SetSlotLength(1*ns)

-- Logging Setup
yats.log:setLevel("INFO")
yats.kernel_log:setLevel("INFO")
local log = yats.log

-- Simulation Config
local N = 8
local SOURCETYPE="cbr"
local RTT = 16*us  -- expected RTT
local TLINEUP = 2*us
local TLINEDN = 2*us
local SEGSIZE = 64
local CHUNKSIZE = 16
--local FLEN = 1518
local N2 = SEGSIZE
local SRATE = 4 * Gbps
local rates = {
  3*149*Mbps, 
  100*Mbps,
  100*Mbps,
  90*Mbps,
  145*Mbps,
  20*Mbps,
  90*Mbps,
  2*Mbps
}

local loads = {
--   0.5, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95 
   0.5, 1, 1, 1, 1, 1, 1, 1
--   1.1, 1,1, 1,1, 1,1, 1, 1, 1, 1
}

local flens = {
   1518, 512, 64, 1518, 1518, 1518, 1518, 1518
}
-- Graphic output configuration
local x,y,dx,dy = 0,0,200,100

local function gattr(key, val)
   local t = {}
   for k,v in pairs(defattrib) do
      t[k] = v
   end
   t[key] = val
   return t
end

local TDELTA=1*us
local ddelta = t2s(TDELTA)
--local ddelta = 1000
local dupdate = ddelta/100
local dmaxval = 10
local senders = {}
local receivers = {}
local uplines = {}

----------------------------------------------------------------
-- Network
----------------------------------------------------------------
-- Demux PtP to receivers
local dout = {}
for i = 1, N do
  dout[i] = {"receiver-"..i, "in"}
end
local demux = yats.demux{"demux", maxvci = N, nout = N, out = dout}

-- PtP LINK => PHY(demux)
dnline = yats.line{"dnline",delay = t2s(TLINEDN), out = {"demux", "demux"}}

-- Mux 
local mux = yats.mux{"mux", ninp = N, buff = 100, out = {"tcptp", "in"}}

-- PTP TC Layer
local tc = yats.tcptp{
   "tcptp", rate = 1.2 * Gbps, segsize = SEGSIZE,
   out = {"dnline", "line"}
}
-- Sources, Senders, Receivers
local src, d2f, msrc, mrx, mbf, snk, mln = {}, {}, {}, {}, {}, {}, {}
local mrxmean, mcup = {},{}

for i = 1, N do

   -- Sources 
   if SOURCETYPE == "cbr" then
      src[i] = yats.cbrsrc{
	 "src-"..i, delta = r2s(rates[i]*loads[i], flens[i]), vci=i,
	 out = {"d2f-"..i, "dat2fram"}
      }
   else
      src[i] = yats.geosrc{"src-"..i, ed = r2s(rates[i]*loads[i], flens[i]), vci = i,
	 out = {"d2f-"..i, "dat2fram"}}
   end
   -- Frame generator
   d2f[i] = yats.dat2fram{"d2f-"..i, flen = flens[i], connid = i, 
      out = {"scheduler-"..i, "in"}}
   -- Sink
   snk[i] = yats.sink{"snk-"..i}

   local n2 = 10*us * rates[i] / 8
--   local n2 = N2

   -- Sender/Scheduler
   senders[i] = yats.scheduler{"scheduler-"..i, cycle = r2s(SRATE, SEGSIZE),
      n2 = n2, n3 = RTT * rates[i] / 8, segsize = SEGSIZE, out = {"mux", "in"..i}}
   
   -- Receivers
   receivers[i] = yats.receiver{"receiver-"..i, rtt = RTT, rate = rates[i], n2 = n2,
      meantime = 100*us, watermark = SEGSIZE*1,
      chunksize = CHUNKSIZE, out = {{"upline-"..i, "line"}, {"snk-"..i, "sink"}}}

   --- Source rate
   msrc[i] = yats.meter{
      "msrc-"..i, title = "SRCRATE-"..i, 
      val = {senders[i], "Count"},
      win = {x,y,dx,dy},
      mode = yats.DiffMode,
      nvals = 100,
      maxval = 2,
      delta = ddelta,
      update = dupdate,
      display = false
   }
   -- Credit update rate
   mcup[i] = yats.meter{
      "cup-"..i, title = "CUPRATE-"..i, 
      val = {senders[i], "ival1"},
      win = {x,y,dx,dy},
      mode = yats.DiffMode,
      nvals = 100,
      maxval = 2,
      delta = ddelta,
      update = dupdate,
      display = false
   }
   -- Receive rate 
   mrx[i] = yats.meter{
      "mrx-"..i, title = "CHNRATE-"..i, 
      val = {receivers[i], "Count"},
      win = {x,y,dx,dy},
      mode = yats.DiffMode,
      nvals = 100,
      maxval = TDELTA*rates[i]/SEGSIZE,
      delta = ddelta,
      update = dupdate,
      display = false
   }
  -- Channel  mean rate 
  mrxmean[i] = yats.meter{
     "mrxmean-"..i, title = "MEANRATE-"..i,
     val = {receivers[i], "ival4"},
     mode = yats.AbsMode,
     win = {x,y,dx,dy},
     nvals = 100,
     maxval = 1 * Gbps,
     delta = ddelta,
     update = dupdate,
     display = false
  }

  -- Line rate 
  mln[i] = yats.meter{
     "mln-"..i, title = "LINRATE-"..i,
     val = {snk[i], "Count"},
      win = {x,y,dx,dy},
      mode = yats.DiffMode,
      nvals = 100,
      maxval = TDELTA*rates[i]/CHUNKSIZE+1,
      delta = ddelta,
      update = dupdate,
      display = false
  }
  
  mbf[i] = yats.meter{
     "mbf-"..i, title = "BF-"..i,
     val = {receivers[i], "ival0"},
     mode = yats.AbsMode,
     win = {x,y,dx,dy},
     nvals = 100,
     maxval = 2000,
--     maxval = N2 + RTT * 800*Mbps / 8,
     delta = ddelta, update = dupdate,
     display = false
  }
  -- PtP PHY => LINK(sender)
  uplines[i] = yats.line{
    "upline-"..i,
    delay = t2s(TLINEUP),
    out = {"scheduler-"..i, "credit"}
  }
  -- Configure demux
  demux:signal{i,i,i}
end

----------------------------------------------------------
-- Display
----------------------------------------------------------
local instr = {}
for j,inst in ipairs{msrc, mrxmean, mbf, mcup, mln} do
   instr[j] = {}
   for i,v in ipairs{1,2,3,8} do
      --   table.insert(instr, {msrc[i], mrx[i], mrxmean[i], mbf[i], mln[i]})
      --      table.insert(instr, {msrc[i], mrxmean[i], mbf[i], mln[i]})
      instr[j][i] = inst[v]
   end
end
local disp = yats.bigdisplay{
   "disp", title="TN PtP", nrow = 5, ncol = table.getn(instr[1]), width = 200, height = 100,
   instruments = instr,
}
disp:show()

----------------------------------------------------------
-- Run
----------------------------------------------------------
printf("Connecting ...\n")
yats.sim:connect()

local T1 = 15000*us

yats.sim:run(t2s(T1))

----------------------------------------------------------
-- Result output
----------------------------------------------------------
local sumrate = 0
for i = 1, N do
   local src, rx, tx, snk = src[i], receivers[i], senders[i], snk[i]
   local cr = tx:getcreditcount()*(4+2+1)*8/yats.SimTimeReal
   printf("ch-%d: src=%d rx=%d rx-loss=%d ur=%s sk=%d ra=%.1f Mbps cc=%d cr=%.1g Mbps %.1f %% maxbuf=%d\n",
	  i, src.counter, rx.counter, rx.lost, rx.underrun, snk.counter, rates[i]/Mbps, tx:getcreditcount(),
	  cr/Mbps, cr/rates[i]*100, rx.maxbuf)
   printf("mux: losstotal=%d loss[%d]=%d\n", mux:getLossTot(), i, mux:getLoss(i)) 
   sumrate = sumrate + rates[i]
end
printf("Sum of linerate=%.4E\n", sumrate/Mbps)