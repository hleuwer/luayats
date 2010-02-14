require "yats"
require "yats.stdlib"
require "yats.statist"

local lev = 0.9
local N = 10000
-- Init the random generator
yats.sim:setRand(1)

-- Create confidence object
cid = yats.confid{"conf", level = lev}

-- Add a couple of values
for i=1,N do
  cid:add(yats.random(100000))
end

-- Show a couple of values
for k,v in pairs({0, yats.random(N), yats.random(N), N}) do
  print(string.format("val[%d]=%d", k,v))
end

-- Show all kind of statistics for this confidence interval
local result = {}
table.insert(result, cid:getLen())
local v = cid:getMean()
table.insert(result, v)
v = cid:getVar()
table.insert(result, v)
for _, l in pairs{0.9, 0.95, 0.975, 0.99} do
   print(l)
  table.insert(result, cid:getLo(l))
  table.insert(result, cid:getUp(l))
  table.insert(result, cid:getWidth(l))
  print(cid:getLo(l), cid:getUp(l), cid:getWidth(l))
  print(string.format("low, upper, width = %g, %g, %g",
		      cid:getLo(l), cid:getUp(l), cid:getWidth(l)))
end
v = cid:getMin()
table.insert(result, v)
v = cid:getMax()
table.insert(result, v)
table.insert(result, math.floor(cid:getCorr(1,100)*10000))
--table.insert(result, cid:getCorr(1,100))
table.insert(result, cid:getFairInd())
print(string.format("len = %d", cid:getLen()))
print(string.format("min, var = %g, %g", cid:getMean()))
print(string.format("var, min = %g, %g", cid:getVar()))
print(string.format("min, max = %g, %g", cid:getMin(), cid:getMax())) 
print(string.format("min, max = %g, %g", cid:getMin(), cid:getMax())) 
print(string.format("corr = %g", cid:getCorr(1,100)))
print(string.format("fairness = %g", cid:getFairInd()))

print(pretty(result))
return result