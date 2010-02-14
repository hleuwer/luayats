--
-- Example for statistical evaluation:
-- "Batch Means"
--
require "yats"
require "yats.statist"

local N = 5  -- batchsize
local M = 10   -- batches
local nsrc = 60
local lev = 0.95
local target = 0.35
local slots = 1e5
local result = {}

-- Set the following expression to true, in order to control random number generation
if true then
  yats.sim:setRand(1)
end

-- A confidence interval
local confN = yats.confid{"confidN", level = lev}
local confM = yats.confid{"confidM", level = lev}

for buff = 10,100,10 do

  yats.sim:reset()
  yats.sim:flushEvents()
  confM:flush()

  -- Some predefined values
  local source = {}

  --A number of Bernoulli sources:
  for i = 1, nsrc do
    source[i] = yats.bssrc{"bs"..i, ex=32, es=2720, delta=15, vci = i, out = {"multiplexer", "in"..i}}
  end
  

  --A multiplexer with buffer size 20
  local mux = yats.mux{"multiplexer", ninp = nsrc, buff = buff, out = {"sink","sink"}}

  --A sink which "eats" the data
  local sink = yats.sink{"sink"}


  local function sample(n)
    mux:resLoss()
    for i = 1, nsrc do
      source[i]:resCounter()
    end
    yats.sim:run(slots)
    local sent = 0
    for i = 1, nsrc do
      sent = sent + source[i]:getCounter()
    end
    local received = sink:getCounter()
    return mux:getLosses(1, nsrc)/ sent
  end

  -- warm up: 1 run
  yats.sim:run(slots)
  local trial = 1
  while true do
    for i = 1, M do
      for j = 1, N do
	confN:add(sample(i))
      end
      local meanN = confN:getMean()
      local meanM = confM:add(meanN)
      confN:flush()
    end
    local meanM, varM = confM:getMean()
    local widthM = confM:getWidth(lev)
    local prec = widthM/meanM
    print(string.format("buff=%3d len=%d meanM=%.4g%% {Lo,Up}={%.4g,%.4g} prec=%4g", 
			buff, confM:getLen(), meanM*100, 
			confM:getLo(lev), confM:getUp(lev), widthM/meanM))
    if prec < target then
      table.insert(result, meanM)
      break
    end
  end
end
print(pretty(result))
return result