require "yats.stdlib"

-- Control ageretm tests
_G.nrun = 3
_G.sourcetype = "cbr"

-- Enable/Disable C-level debug outputs
yats.cdebug = false

-- Enable output redirection
redir = true

-- Set user and kernel log level
--yats.log:setLevel("DEBUG")
--yats.kernel_log:setLevel("DEBUG")

-- IO redirection
local _print = print
_G._print = print
local _printf = printf
local _write = io.write
local _stdout = io.stdout

if redir == true then
  _G.print=function(...) end
  _G.printf=function(...) end
  io.stdout = {
    write = function(...) end
  }
end
--[[
readline.readline = function(p)
		      return "exit"
		    end
]]

local function gcinfo()
   return 0, collectgarbage("count")
end

local function equal(t1, t2)
  local i,v 
  local res = true
  for i,v in ipairs(t1) do
    if (not t2[i]) or (tostring2(t2[i]) ~= tostring2(v)) then
      _stdout:write(string.format("UNEQUAL PATH 1: res[%d]=%s ref[%d]=%s\n",
			  i, tostring2(v), i, tostring2(t2[i])))
      res = false
    end
  end
  for i,v in ipairs(t2) do
    if (not t1[i]) or (tostring2(t1[i]) ~= tostring2(v)) then
      _print(i,v)
      _stdout:write(string.format("UNEQUAL PATH 2: res[%d]=%s ref[%d]=%s\n",
			  i, tostring2(v), i, tostring2(t1[i])))
      res = false
    end
  end
  return res
end

local function dotest(t, mode, run)
  local tfile = t[1]
  local talias = t[2]
  local tprolog = t[3]
  local gcmem, gcthres = gcinfo()
  if mode == "w" then
    _stdout:write(string.format("W %d %s [%s]: ", run, tfile, talias))
  else
    _stdout:write(string.format("T %d %s [%s]: ", run, tfile, talias))
  end
  _stdout:flush()
  local f, err = loadfile("./examples/"..tfile..".lua")
  local rf = "./examples/results/"..tfile.."-result.lua"
  assert(not err, err)
  local tenv = {}
  setmetatable(tenv, {__index = _G})
  if tprolog and type(tprolog) == "function" then
    setfenv(tprolog, tenv)
    tprolog()
  end
  setfenv(f, tenv)
  local a,b = gcinfo()
  yats.sim:reset()
  local c,d = gcinfo()
  local res, err = f()
  assert(not err, err)
  if mode == "w" then
    local fout, err = io.open(rf, "w+")
    assert(fout, err)
    fout:write("return "..pretty(res).."\n")
    fout:close()
  elseif mode == "t" then
    local fr, err = loadfile(rf)
    assert(not err, err)
    local ref = fr()
    if not equal(res, ref) then
      _stdout:write(string.format("failed [%d,%d,%d]\n", a,c,d))
      os.exit(1)
    end
--    assert(equal(res, ref), "Test failed")
  end
  if mode == "w" then
    _stdout:write(string.format("done [%d,%d,%d]\n", a,c,d))
  else
    _stdout:write(string.format("passed [%d,%d,%d]\n", a,c,d))
  end
end

tests = {
  {"test-1", "basic - mux, demux, cbr source"},
  {"test-1-src", "basic - mux, demux, cbr, bs, list, geo sources"},
  {"test-2", "luadummy"},
  {"test-3", "meas"},
  {"test-4", "hist2"},
  {"test-4-muxfrmprio", "muxFrmPrio 1"},
  {"test-ethbridge-1", "transparent bridge"},
  {"test-4-muxdf", "muxDF"},
  {"test-4-muxaf", "muxAF"},
  {"test-4-muxdist", "muxDist"},
  {"test-4-muxwfq", "muxWFQ"},
  {"test-4-muxprio", "muxPrio"},
  {"test-5", "meter: implicit display"},
  {"test-5-attach", "meter: attached display"},
  --  {"test-7", "luacontrol: luayats cli, event callback"}, -- omitted because it requires interactive input
  {"test-8", "block: switch"},
  {"test-9", "block: superswitch"},
  --  "test-10", -- omitted since it needs dedicated compilation for tracing
  {"test-11", "rstp: 2 bridges"},
  {"test-11a", "rstp: 2 bridges, change forward delay"},
  {"test-11b", "rstp - 2 bridges, unplug port bidir"},
  {"test-11b1", "rstp - 2 bridges, LinkDown"},
  {"test-11b2", "rstp - 2 bridges, pathcost change forth and back"},
  {"test-11c", "rstp - 2 bridges, unplug port rx"},
  {"test-11d", "rstp - 2 bridges, unplug port tx"},
  {"test-11e", "rstp - 2 bridges, inspectors"},
  {"test-11-pvst", "rstp: 2 pvst bridges (2 vid)"},
  {"test-11b-pvst", "rstp: 2 prvs bridges (2 vid), unplug port"},
  {"test-11b-2-pvst", "rstp: 2 prvs bridges (2 vid), disable port"},
  {"test-11b-3-pvst", "rstp: 2 prvs bridges (2 vid), unplug+disable port"},
  {"pvlan-1", "real bridge example"},
  {"rstp-test-ring", "rstp: ring network"},
  {"test-12b", "cell/frame sources demo"},
  {"test-tcpip", "tcpip connection"},
  {"ageretm-bigdisplay-attach", "sdwrr packet scheduling, display move"},
  {"ageretm", "sdwrr packet scheduling",
    function() display = true end
  },
  {"ageretm-shell", "sdwrr packet scheduling, from other script"},
  {"ageretm-color", "sdwrr packet scheduling different colors"},
  {"ageretm-bigdisplay", "sdwrr packet scheduling with CBR sources",
    function () _G.sourcetype="cbr" end},
  {"ageretm-bigdisplay", "sdwrr packet scheduling with BS source (determin.)",
    function () _G.sourcetype="bs" end},
  {"ageretm-bigdisplay", "sdwrr packet scheduling with BS source (random)",
    function () _G.sourcetype="bs" _G.bs_deterministics=0 end},
  {"ageretm-bigdisplay", "sdwrr packet scheduling with GEO sources",
    function () _G.sourcetype="geo" end},
  {"test-13", "confidence object"},
  {"test-14", "confidence replications"},
  {"test-15", "confidence batch means"},
  {"test-16", "independent replications"}
}

local mode = os.getenv("LUAYATSTESTMODE") or LUAYATSTESTMODE or "t"
local runs = os.getenv("LUAYATSTESTRUNS") or LUAYATSTESTRUNS or 1
for j = 1, runs do
  for i,v in ipairs(tests) do
    dotest(v, mode, j)
  end
end

os.exit(0)