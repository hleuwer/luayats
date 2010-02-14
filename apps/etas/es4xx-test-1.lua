require "yats"
require "yats.stdlib"
require "yats.graphics"
require "apps.etas.es4xx"


-- Helper: units
local ms, us, ns = 1e-3, 1e-6, 1e-9
local Gbps, Mbps, kbps = 1e9, 1e6, 1e3
local GHz, MHz, kHz = 1e9, 1e6, 1e3

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
local FLEN = 64
local samprate = 10 * kHz
local FRATE = 1.2 * samprate
local WINSIZE = 32
local T1 = 10

-- frame generator
local src = yats.cbrsrc{
   "src", delta = r2s(FRATE, FLEN), vci=1,
   out = {"es4xx", "in"}
}
-- ES4xx
local es = yats.es4xx{
   "es4xx", out = {"sock", "in"},
   samprate = samprate,
   produce = function(self) return 1234 end,
   winsize = WINSIZE,
   qlen = 100,
   cycle = r2s(FRATE, FLEN), 
}

-- Socket   
local sk = yats.sock{
   "sock", out = {"es4xx", "fc"},
   winsize = WINSIZE,
   qlen = 100,
   cycle = r2s(FRATE, FLEN)
}

printf("Connecting ...\n")
yats.sim:connect()

print("Running:", t2s(T1))
yats.sim:run(t2s(T1))