require "yats.stdlib"
require "yats.src"
require "yats.muxevt"
require "yats.misc"
require "yats.graphics"
--
-- Use any number you like here
--
local nsites = 7
local ncores = 6
local naccess = {nsites, 4, 5, 4, 4, 0}
--local naccess = {8, 8, 8, 0, 0, 0}
local ncore = 6
local nsrc = nsites*2
local nprio = 3
local simtime = 60 --seconds
local simsteps = simtime
-- 
local dexsrc = {cbr=0, vbr=1}
local dessrc = {cbr=0, vbr=1}
-- Use this ethernet rate
local ethrate = {
  cbr=100*1e6,
  vbr=10*1e6
}
-- Scale CTD display by factor of 'ctddiv', e.g. 1000: measurement unit = 1000 slots
local ctddiv = 1000
-- Max. CTD awaited
local ctdmax = 50e-6

-- Rates and times
local urate = {
  cbr=750e3, 
  vbr=1500e3
}

local lrate = {
  eth=ethrate.cbr,
  stm1=149*1e6,
  ha1=20*1e6,
  ha2=77*1e6,
}

-- Radio Transport Delay per hop
local dRadio = {
  ha1 = 430e-6,
  ha2 = 230e-6
}

-- Frame Sizes
local sEth = {cbr=1522, vbr=1522}
local sAal5 = {cbr=32*53, vbr=32*53}

-- Mux Parameters
local vcprios = {2,1,0}
local vcqlens = {3000,3000,3000}

-- Init the yats random generator.
yats.sim:setRand(10)
yats.sim:ResetTime()
yats.sim:setSlotLength(0.1e-6)

--
-- Constants
--
local cellsize=8*53
local x,y,dx,dy = 0, 0, 200, 100


-- Realtime
local function s2t(s)
  return s * yats.SlotLength
end
local function t2s(t)
  return t / yats.SlotLength
end
local function r2s(r)
  return t2s(8*53/r)
end

local lcrate = {
  eth=lrate.eth / cellsize,
  stm1=lrate.stm1 / cellsize,
  ha1=lrate.ha1 / cellsize,
  ha2=lrate.ha2 /cellsize
}

local ucrate = {
  cbr=urate.cbr/48/8, 
  vbr=urate.vbr/48/8
}

local sr = {
  cbr=ucrate.cbr/lcrate.eth, 
  vbr=ucrate.vbr/lcrate.eth
}
local eX = {
  cbr=math.floor(sAal5.cbr/53),
  vbr=math.floor(sAal5.vbr/53)
}
local eB = {
  cbr=math.floor(t2s(eX.cbr/lcrate.eth)),
  vbr=math.floor(t2s(eX.vbr/lcrate.eth))
}
local eS = {
  cbr=math.floor((1/sr.cbr-1)*eB.cbr), 
  vbr=math.floor((1/sr.vbr-1)*eB.vbr)
}
local function sumsites()
  local sum = 0
  for _,v in pairs(naccess) do
    sum = sum + v
  end
  return sum + ncores
end

printf("Rate Parameters:\n")
printf("  User rates:\n")
printf("   User bit rates  urate=%s bit/s\n", tostring2(urate))
printf("   User cell rates ucrate=%s cells/s\n", tostring2(ucrate))
printf("   Cell Count Burst    eX=%s cells\n", tostring2(eX))
printf("   Burst duration      eB=%s slots\n", tostring2(eB))
printf("   Silence duration    eS=%s slots\n", tostring2(eS))
printf("   IAT on Ethernet cbr  d=%d slots\n",r2s(ethrate.cbr))
printf("   IAT on Ethernet vbr  d=%d slots\n",r2s(ethrate.vbr))
printf("  Linerates:\n")
printf("   Line bit rates   lrate=%s bit/s\n", tostring2(lrate))
printf("   Line cell rates lcrate=%s cells/s\n", tostring2(lcrate))
printf("   Rate fraction             sr=%s\n", tostring2(sr))
printf("  Loads:\n")
printf("   acesss with %d sites: k=%f\n", 
       naccess[1], naccess[1]*(urate.cbr + urate.vbr)/lrate.ha1)
printf("   core with %d sites:   k=%f\n", 
       sumsites(), sumsites()*(urate.cbr + urate.vbr)/lrate.ha2)

--------------------------------------------------------------------------
-- Site Definition
--------------------------------------------------------------------------
--
-- Access Site class
--
accessSite = class(yats.block)
function accessSite:init(param)
  self = yats.block()
  self.name = autoname(param)
  self.clname = "site"
  self.parameters = {
    rate = true,
    out = true
  }
  self:adjust(param)
  self.rate = param.rate
  self.mx1 = self:addobj(yats.muxPrio{
			   self:localname("mux1"), ninp=2, maxvci=256, 
					  nprio=nprio, service=r2s(lrate.stm1), 
					  mode="async", 
					  out={self:localname("mux2"), "in1"}})
  self.mx2 = self:addobj(yats.muxPrio{
			   self:localname("mux2"), ninp=2, maxvci=256,
					  nprio=nprio, service=r2s(self.rate), 
					  mode="async", 
					  out={self:localname("out1")}})
  for i = 0, nprio-1 do
    self.mx1:setQueueMax(i, vcqlens[i+1])
    self.mx2:setQueueMax(i, vcqlens[i+1])
  end
  for vc = 1, 256 do
    if math.mod(vc,2) == 1 then
      self.mx1:setPriority(vc,2)
      self.mx2:setPriority(vc,2)
    elseif math.mod(vc,2) == 0 then
      self.mx1:setPriority(vc,1)
      self.mx2:setPriority(vc,1)
    end
  end
  self:definp{self:localname("mux1"), "in1"}
  self:definp{self:localname("mux1"), "in2"}
  self:definp{self:localname("mux2"), "in2"}
  self:defout(param.out)
  return self:finish()
end

function accessSite:getLosses()
  return self.mx1:getLosses(1,2)
    + self.mx2:getLosses(1,2)
end

--
-- Core Site class
--
coreSite = class(yats.block, accessSite)
function coreSite:init(param)
  self = yats.block()
  self.name = autoname(param)
  self.clname = "site"
  self.parameters = {
    rate = true,
    out = true
  }
  self:adjust(param)
  self.rate = param.rate
  self.mx1 = self:addobj(yats.muxPrio{
			   self:localname("mux1"), ninp=2, maxvci=256, 
					  nprio=nprio, service=r2s(lrate.stm1), 
					  mode="async", 
					  out={self:localname("mux2"), "in1"}})
  self.mx2 = self:addobj(yats.muxPrio{
			   self:localname("mux2"), ninp=2, maxvci=256,
					  nprio=nprio, service=r2s(self.rate), 
					  mode="async", 
					  out={self:localname("out1")}})
  self.mx3 = self:addobj(yats.muxPrio{
			   self:localname("mux3"), ninp=2, maxvci=256,
					  nprio=nprio, service=r2s(lrate.stm1), 
					  mode="async", 
					  out={self:localname("mux2"),"in2"}})
  for i = 0, nprio-1 do
    self.mx1:setQueueMax(i, vcqlens[i+1])
    self.mx2:setQueueMax(i, vcqlens[i+1])
    self.mx3:setQueueMax(i, vcqlens[i+1])
  end
  for vc = 1, 256 do
    if math.mod(vc,2) == 1 then
      self.mx1:setPriority(vc,2)
      self.mx2:setPriority(vc,2)
      self.mx3:setPriority(vc,2)
    elseif math.mod(vc,2) == 0 then
      self.mx1:setPriority(vc,1)
      self.mx2:setPriority(vc,1)
      self.mx3:setPriority(vc,1)
    end
  end
  self:definp{self:localname("mux1"), "in1"}
  self:definp{self:localname("mux1"), "in2"}
  self:definp{self:localname("mux3"), "in1"}
  self:definp{self:localname("mux3"), "in2"}
  self:defout(param.out)
  return self:finish()
end

function coreSite:getLosses()
  return self.mx1:getLosses(1,2)
    + self.mx2:getLosses(1,2)
    + self.mx3:getLosses(1,2)
end

--
-- Create Sources
--
local vcinew = 0
local function vcnext()
  vcinew = vcinew + 1
  return vcinew
end

function crSrc(vci, ex, es, suc, des, dex, ethrate)
  local src = yats.bssrc{
    "src"..vci, ex = ex, es = es, des=des or dessrc, dex=dex or dexsrc,
    delta = r2s(ethrate or lrate.eth), vci = vci, out = suc
  }
  yats.log:info("Source %s vci=%d created", src.name, vci)
  return src
end

--
-- Create Access Sites.
--
function createAccessSites(cix, subsites, core)
  if subsites == 0 then return nil, "no subsites" end
  local vcm1, vcm2
  for i = 1, subsites do
    local vc1, vc2 = vcnext(), vcnext()
    if i == 1 then vcm1, vcm2 = vc1, vc2 end
    local ix = cix+i
    local site = accessSite{"access"..ix, rate=lrate.ha1, out={{"aline"..ix,"line"}}}
    if i < subsites then 
      out = {"access"..cix+i+1, "in1"} 
    else 
      out = {"measaccess"..cix, "meas3"} 
    end
    local lin = yats.line{"aline"..ix, delay = t2s(dRadio.ha1), out=out}
    local src = crSrc(vc1, eX.cbr, eS.cbr, {"access"..ix, "in2"},
		      dessrc.cbr, dexsrc.cbr, ethrate.cbr)
    src = crSrc(vc2, eX.vbr, eS.vbr, {"access"..ix, "in3"},
		      dessrc.vbr, dexsrc.vbr, ethrate.vbr)
    yats.log:info("Access site %d created: vci=%d,%d", ix, vc1, vc2)
  end
  local meas = yats.meas3{"measaccess"..cix, vci = {vcm1,vcm2}, ctd = {0,t2s(ctdmax)}, 
    ctddiv = ctddiv, out = core
  }
  meas.vcim = {vcm1, vcm2}
  return meas
end

--
-- Create Core Sites.
--
function createCoreSites(ncores)
  local measaccess, meascore = {}, {}
  local subs = naccess
  for i = 1, ncores do
    measaccess[i] = createAccessSites(i*100, subs[i] or 0, {"core"..i, "in4"})
    local site = coreSite{"core"..i, rate=lrate.ha2, out = {{"cline"..i, "line"}}}
    local lin = yats.line{"cline"..i, delay = t2s(dRadio.ha2), out = {"meascore"..i, "meas3"}}
    if i < ncores then
      out = {"core"..i+1,"in1"}
    else
      out = {"sink", "sink"}
    end
    meascore[i] = yats.meas3{"meascore"..i, vci = {1,2}, ctd = {0,t2s(ctdmax)}, 
      ctddiv = ctddiv, out = out
    }
    local vc1, vc2 = vcnext(), vcnext()
    local src = crSrc(vc1, eX.cbr, eS.cbr, {"core"..i, "in2"},
		      dessrc.cbr, dexsrc.cbr, ethrate.cbr)
    src = crSrc(vc2, eX.vbr, eS.vbr, {"core"..i, "in3"},
		      dessrc.vbr, dexsrc.vbr, ethrate.vbr)
    yats.log:info("Core site %d created: vci=%d,%d", i, vc1, vc2)
  end
  local snk = yats.sink{"sink"}
  return measaccess, meascore, snk
end

--
-- Create the Network
--
ameas, cmeas, snk = createCoreSites(ncores)

--
-- Measurements
--
hist, mtr = {}, {}

-- Delay Distributions
hist[1] = yats.histo{"hist1", title = "VCI 1"..": CTD", 
  val = {ameas[1], "CTD", 1}, win = {x,y,dx,dy}, delta=t2s(0.1), display=false
}
hist[2] = yats.histo{"hist2", title = "VCI 2"..": CTD", 
  val = {ameas[1], "CTD", 2}, win = {x,y,dx,dy}, delta=t2s(0.1), display=false
}
hist[3] = yats.histo{"hist3", title = "VCI 1"..": CTD", 
  val = {cmeas[ncores], "CTD", 1}, win = {x,y,dx,dy}, delta=t2s(0.1), display=false
}
hist[4] = yats.histo{"hist4", title = "VCI 2"..": CTD", 
  val = {cmeas[ncores], "CTD", 2}, win = {x,y,dx,dy}, delta=t2s(0.1), display=false
}

-- Services
mtr[1] = yats.meter{"mtr1", title = "COUNT 1", val = {yats.sim:getObj("src1"), "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 200, maxval = 100,
  delta = t2s(0.001), update = 70, display = false
}
mtr[2] = yats.meter{"mtr2", title = "COUNT 1", val = {yats.sim:getObj("src2"), "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 200, maxval = 100,
  delta = t2s(0.001), update = 70, display = false
}

-- Summary Rate
mtr[3] = yats.meter{"mtr3", title = "COUNT 1", val = {yats.sim:getObj("sink"), "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 200, maxval = 1000,
  delta = t2s(0.001), update = 70, display = false
}

disp = yats.bigdisplay {
  "disp", title="Cell Transfer Delay", nrow = 4, ncol = 2, width = 200, height = 100, 
  instruments = {
    {mtr[1], hist[1]},
    {mtr[2], hist[2]},
    {mtr[3], hist[3]},
    {nil,    hist[4]},
  }
}
disp:show()
disp:setAttribute("polystep", 1, 1, 1)
disp:setAttribute("polystep", 1, 2, 1)


-- Connection management
yats.sim:connect()

-- Run simulation: 1. path
for i = 1,simsteps do
  io.write("\r                                                     ")
  io.write(string.format("\rtime simulated: %f sec",yats.SimTimeReal))
  io.flush()
  yats.sim:run(t2s(simtime/simsteps))
end

for i = 1, 4 do
  hist[i]:store2file("log-"..i..".txt", "\t")
end

-- Display CTD
printf("Cell Transfer Delay VC 1 and 2 per site:\n")
for i = 1, ncores do
  for vc = 1,2 do
    printf("  CTD Core site %d: vci=%d: mean=%f ms, min=%f ms, max=%f ms\n", i, vc,
	   s2t(cmeas[i]:getMeanCTD(vc-1)*1000),
	   s2t(cmeas[i]:getMinCTD(vc-1)*1000),
	   s2t(cmeas[i]:getMaxCTD(vc-1))*1000)
  end
end
for i = 1, ncores do
  for j = 1, 2 do
    if ameas[i] then
      local vc = ameas[i].vcim[j]
      printf("  CTD Access ring %d: vci=%d: mean=%f ms, min=%f ms, max=%f ms\n", i, vc,
	     s2t(ameas[i]:getMeanCTD(j-1)*1000),
	     s2t(ameas[i]:getMinCTD(j-1)*1000),
	     s2t(ameas[i]:getMaxCTD(j-1))*1000)
    end
  end
end
-- Display some values from the Multiplexer.
printf("Losses:")
local sumloss = 0
for i = 1, ncores do
  for j = 1, naccess[i] do
    local site = yats.sim:getObj("access"..i*100+j)
    local loss = site:getLosses()
    sumloss = sumloss + loss
    if loss > 0 then
      printf("  access site %s: %d\n", site.name, site:getLosses())
    end
  end
  local site = yats.sim:getObj("core"..i)
  local loss = site:getLosses()
  sumloss = sumloss + loss
  if loss > 0 then
    printf("  core site %s: %d\n", site.name, site:getLosses())
  end
end
printf("  total loss: %d\n", sumloss)
printf("Source and Sink cell counters:\n")
-- Display source counters.
for i = 1, 2 do
  printf("  vci=%d: src=%d snk=%d\n", i, yats.sim:getObj("src"..i):getCounter(), snk:getCounter())
end
