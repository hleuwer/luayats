--
-- Example input file
--
require "yats"
require "yats.statist"

nbatches = 10
batchsize = 5
lag = 5
nsrc = 60
lev = 0.95
precision = 0.1
slots = 1e5

-- Set the following expression to true, in order to control random number generation
if true then
  yats.sim:setRand(1)
end

-- Some predefined values
source = {}

--A number of Bernoulli sources:
for i = 1, nsrc do
  source[i] = yats.bssrc{"bs"..i, ex=32, es=2720, delta=15, vci = i, out = {"multiplexer", "in"..i}}
end

--A multiplexer with buffer size 20
mux = yats.mux{"multiplexer", ninp = nsrc, buff = 20, out = {"sink","sink"}}

--A sink which "eats" the data
sink = yats.sink{"sink"}

-- A confidence interval
conf = yats.confid{"confid", level = lev}

local function sample(n)
  mux:resLoss()
  for i = 1, nsrc do
    source[i]:resCounter()
  end
  yats.sim:run(slots)
  sent = 0
  for i = 1, nsrc do
    sent = sent + source[i]:getCounter()
  end
  received = sink:getCounter()
  local rv = mux:getLosses(1, nsrc)/ sent
  return rv
end

-- warm up: 1 run
yats.sim:run(slots)

local result = {}
local trial = 1

-- Run and build the samples
while true do
  local corr, width, mean, var
  for i = 1, nbatches do
    for j = 1, batchsize do
      conf:add(sample(i))
    end
  end
  corr = conf:getCorr(lag, batchsize)
  mean, var = conf:getMean()
  width = conf:getWidth(lev)

  print(string.format("trial %d: len=%d corr=%g mean=%g, var=%g ", 
		      trial, conf:getLen(), corr, conf:getMean()))
  print(string.format("trial %d: lo=%g up=%g width=%g prec=%g\n", trial,
		      conf:getLo(lev), conf:getUp(lev), conf:getWidth(lev), width/mean))
  table.insert(result, corr)
  table.insert(result, mean)
  table.insert(result, var)
  table.insert(result, width)

  -- Check, whether to finish.
  if width / mean < precision then
    print("Done: precision of " .. precision .. " reached.")
    break
  end
  trial = trial + 1
end
return result