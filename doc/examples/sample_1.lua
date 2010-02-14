--
-- Example input file
--
require "yats.src"

-- Set the following expression to true, in order to control random 
-- number generation
if false then
  yats.sim:setRand(1)
end

-- Some predefined values
nsrc, load  = 10, 0.95
source = {}

--A number of Bernoulli sources:
for i = 1, nsrc do
  source[i] = yats.geosrc{
    "bernoulli"..i, 
    ed = nsrc/load, 
    vci = i, 
    out = {"multiplexer", "in"..i}
  }
end

--A multiplexer with buffer size 20
mux = yats.mux{
  "multiplexer", 
  ninp = nsrc, 
  buff = 20, 
  out = {"sink","sink"}
}

--A sink which "eats" the data
sink = yats.sink{"sink"}

--simulate 100000 time slots
yats.sim:run(100000)

-- print results
sent, received = 0, 0;

for i = 1, nsrc do
  sent = sent + source[i]:getCounter()
end
received = sink:getCounter()

-- Print (use string concatenation)
print("cells sent: " .. sent)
print("cells received: " .. received)

-- Print (use c-like format expression)
print(string.format("CLR in mux: %.3f percent", 
		    mux:getLosses(1, nsrc) *100 / sent))

-- end of example
