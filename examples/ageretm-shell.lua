s = "examples/ageretm.lua"
local f, err = loadfile(s)
assert(f, err)
res, err = f()
return res
