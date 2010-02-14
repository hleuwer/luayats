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
local N = 4
local SOURCETYPE="cbr"
local RTT = 16*us
local TLINEUP = 2*us
local TLINEDN = 2*us
local SEGSIZE = 64
local CHUNKSIZE = 32
local FLEN = 1518
local N2 = SEGSIZE
local srate = 4 * Gbps
local rates = {
  3*149*Mbps,
  100*Mbps,
  100*Mbps,
  90*Mbps,
  45*Mbps,
  20*Mbps,
  10*Mbps,
  2*Mbps
}
local loads = {
   0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95 
--   1, 1, 1, 1, 1, 1, 1, 1
--   1.1, 1,1, 1,1, 1,1, 1, 1, 1, 1
}

-- Graphic output configuration
local x,y,dx,dy = 0,0,200,100


local ddelta = t2s(1*us)
--local ddelta = 1000
local dupdate = ddelta/40
local dmaxval = 5
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

-- DF Mux 
local mux = yats.muxDF{"mux", ninp = N, active = r2s(1.2 * Gbps, SEGSIZE),
   maxvci = N, buff = 100, out = {"dnline", "line"}}

-- Sources, Senders, Receivers
local src, d2f, msrc, mrx, mbf, snk, mln = {}, {}, {}, {}, {}, {}, {}
local mrxmean = {}

for i = 1, N do

   -- Sources 
   if SOURCETYPE == "cbr" then
      src[i] = yats.cbrsrc{
	 "src-"..i, delta = r2s(rates[i]*1.001, SEGSIZE), vci=i,
	 out = {"d2f-"..i, "dat2fram"}
      }
   else
      src[i] = yats.geosrc{"src-"..i, ed = r2s(rates[i]*loads[i], FLEN), vci = i,
	 out = {"d2f-"..i, "dat2fram"}}
   end
   -- Frame generator
   d2f[i] = yats.dat2fram{"d2f-"..i, flen = FLEN, connid = i, 
      out = {"scheduler-"..i, "in"}}
   -- Sink
   snk[i] = yats.sink{"snk-"..i}

   local n2 = 10*us * rates[i] / 8
   
   -- Sender/Scheduler
   senders[i] = yats.scheduler{"scheduler-"..i, cycle = r2s(1.2 * Gbps, SEGSIZE),
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
      maxval = dmaxval,
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
      maxval = dmaxval,
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
      maxval = dmaxval,
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
     maxval = N2 + RTT * 800*Mbps / 8,
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
for i = 1, N do
   table.insert(instr, {msrc[i], mrx[i], mrxmean[i], mbf[i], mln[i]})
end
local disp = yats.bigdisplay{
   "disp", title="TN PtP", nrow = N, ncol = 5, width = 200, height = 100,
   instruments = instr,
}
disp:show()

----------------------------------------------------------
-- Run
----------------------------------------------------------
printf("Connecting ...\n")
yats.sim:connect()

local T1, T2, T3, T4, T5 = 500, 100, 567, 100, 500 

yats.sim:run(t2s(T1*us))

local rec = receivers[1]
local snd = senders[1]

-- 1. Change rate DOWN - no hold-off time !!!
iup.Message("info", "Change down")

-- 1.1 Change and recalc n1 + n2 for scheduler rate adoptions
log:info(fmt("Changing linerate down"))
rec:setlinerate(rates[1]/3)

yats.sim:run(t2s(T2*us))

-- 2.1 Adopt max. buffer for min. delay after rate change
--local n2, n3 = rec:align(rates[1]/3)

-- 2.2 Adopting scheduler rate
--snd:setn2n3(n2, n3)

-- 3. Run a while at lowered rate
yats.sim:run(t2s(T3*us))

-- 4. Chage rate UP

iup.Message("info", "change up")

-- 4.1 Prepare receiver to new higher rate
--n2, n3 = rec:align(rates[1])

-- 4.3 Prepare sender to new higher rate
--snd:setn2n3(n2, n3)

-- 5. Run a while at low rate but old parameters Wait to Restore
yats.sim:run(t2s(T4*us))

-- 6. Now change the rate => no recalc, because everything has been set.
log:info(fmt("Changing linerate up"))
n2, n3 = rec:setlinerate(rates[1])

yats.sim:run(t2s(T5*us))

----------------------------------------------------------
-- Result output
----------------------------------------------------------
for i = 1, N do
   local src, rx, tx, snk = src[i], receivers[i], senders[i], snk[i]
   local cr = tx:getcreditcount()*(4+2+1)*8/yats.SimTimeReal
   printf("ch-%d: src=%d rx=%d rx-loss=%d ur=%s sk=%d ra=%.1f Mbps cc=%d cr=%.1g Mbps %.1f %% bfmax=%d\n",
	  i, src.counter, rx.counter, rx.lost, rx.underrun, snk.counter, rates[i]/Mbps, tx:getcreditcount(),
	  cr/Mbps, cr/rates[i]*100, rx.maxbuf)
end