--
-- Example for statistical evaluation:
-- "Independent Replications"
--
require "yats"
require "yats.statist"

local M = 10 -- # of samples
local N =  5 -- # of replications
local nsrc = 60
local lev = 0.95
local slots = 1e5
local result = {}

-- Set the following expression to true, in order to control random number generation
if true then
  yats.sim:setRand(1)
end

-- We need means fo M samples in N replications
local confM = yats.confid{"confidM", level = lev}
local confN = yats.confid{"confidN", level = lev}

-- Loss = f(buffer size)
for buff = 10,100,10 do
  -- The replications
  for i = 1, N do

    -- Replications are independent => reset simulation
    yats.sim:reset()
    yats.sim:flushEvents()
  
    --A number of Bernoulli sources:
    local source = {}
    for i = 1, nsrc do
      source[i] = yats.bssrc{"bs"..i, ex=32, es=2720, delta=15, vci = i, out = {"multiplexer", "in"..i}}
    end
    
    --A multiplexer with buffer size 'buff'
    local mux = yats.mux{"multiplexer", ninp = nsrc, buff = buff, out = {"sink","sink"}}
    
    --A sink which "eats" the data
    local sink = yats.sink{"sink"}
    
    -- Function to produce a sample
    local function sample()
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
    
    -- Flush the sample confidence
    confM:flush()

    -- warm up: 1 run => delete initial samples
    yats.sim:run(slots)
    
    -- collect samples
    for j = 1, M do
      confM:add(sample())
    end
    -- Capture mean of a single run
    local meanM = confM:getMean()
    confN:add(meanM)
    yats.sim:reset()
    yats.sim:flushEvents()
  end
  -- Calculate widthN, meanN and varN
  local widthN = confN:getWidth(lev)
  local meanN, varN = confN:getMean()
  
  print(string.format("buff=%3d len=%d meanN=%.4g%% {Lo,Up}={%.4g,%.4g} prec=%.4g", 
		      buff, confN:getLen(), meanN*100, confN:getLo(lev), confN:getUp(lev),
		  widthN/meanN))
  table.insert(result, meanN)
  -- Next buff => clear confN
  confN:flush()
end
print(pretty(result))
return result