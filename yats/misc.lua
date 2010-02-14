-----------------------------------------------------------------------------------
-- @title LuaYats - Miscellanous.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: misc.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats Miscellanous Lua classes
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-----------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- Delay Line.
--==========================================================================

_line = line
--- Definition of class 'line'.
line = class(_line)

--- Constructor for class 'line'.
-- A simple line with a given transmission delay.
-- @param param table - Parameter List
-- <ul>
-- <li> name (optional)<br>
--    Name of the display; default: "objNN"
-- <li> delay<br>
--    Delay in ticks 
-- <li> out<br>
--    Connection to successor 
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>
-- </ul>.
-- @return table -  Reference to object instance.
function line:init(param)
  self = _line:new()
  self.name=autoname(param)
  self.clname = "line"
  self.parameters = {
    delay = true,
    out = true
  }
  self:adjust(param)
  
  -- 2. set object vars in yats object
  assert(param.delay > 0, "line: impossible delay < 1: "..(param.delay or "nil"))
  self.delay = param.delay
  
  -- 3. init outputs
  self:defout(param.out)
  
  -- 3. init input table
  --  self.inputs = {{name = self.name, shand = 1}}
  self:definp(self.clname)
	 
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end


--==========================================================================
-- SINK: Sink.
--==========================================================================

_sink = sink
--- Definition of class 'sink'.
sink = class(_sink)

--- Constructor for class 'sink'.
-- A sink typically terminates a data stream and free's the frame.
-- @param param table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN"
-- </ul>
-- @return table - Reference to object instance.
function sink:init(param)
  self = _sink:new()
  self.name=autoname(param)
  self.clname = "sink"
  self:adjust(param)
  
  -- 2. set object vars in yats object
  
  -- 3. init outputs
  -- no outputs
  
  -- 4. init input table
  --self.inputs = {{name=self.name, shand = 1}}
  self:definp(self.clname)
  
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- Dummy Object
--==========================================================================
--- Definition of class 'dummy'.
_dummyObj = dummyObj
dummyObj = class(_dummyObj)
dummy = dummyObj
--- Constructor for class 'dummy'.
-- The dummy object is a trivial node, that simply forwards a received frame
-- to it's successor. It's main use is in block structures to connect 
-- to objects inside a block.
-- @param param table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li> out<br>
--    Connection to successor 
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>
-- </ul>.
-- @return table - Reference to object instance.
function dummy:init(param)
  self = _dummyObj:new()
  self.name = autoname(param)
  self.clname = "dummy"
  self.parameters = {out = true}
  self:adjust(param)
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- Meas Object
--==========================================================================

_meas = meas
--- Definition of 'meas' object.
meas = class(_meas)

--- Constructor for class 'meas'.
-- @usage ref = yats.meas:new{[name=]"objname",vci=N,maxtim=N,out={"next", "pin"}}]].
-- A measurement class for cell count and cell transfer delay.
-- @param param table - Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> vci<br>
--    vci to measure. 
-- <li> maxtim<br>
--    Max. delay awaited (required to build a histogram). 
-- <li> out<br>
--    Connection to successor.<br>
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>.
-- </ul>
-- @return table - Reference to object instance.
-- @see meas3:init.
function meas:init(param)
  self = _meas:new()
  self.name = autoname(param)
  self.clname = "meas"
  self.parameters = {
    vci = true, maxtim = true, out = true
  }
  self:adjust(param)
  self:definp(self.clname)
  self:defout(param.out)
  assert(param.maxtim > 0, "meas: maxtim > 0 required.")
  self.maxtim = param.maxtim 
  assert(param.vci, "meas: parameter 'vci' required.")
  self.vci = param.vci
  return self:finish()
end

--==========================================================================
-- Meas2 Object
--==========================================================================

_meas2 = meas2
--- Definition of 'meas2' object.
meas2 = class(_meas2)

--- Constructor for class 'meas2'.
-- A measurement class for cell count and cell transfer delay.
-- @param param table - Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> vci<br>
--    vci to measure. 
-- <li> maxtim<br>
--    Max. delay awaited (required to build a histogram). 
-- <li> out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}.
-- </ul>
-- @return table - Reference to object instance.
-- @see meas3:init.
function meas2:init(param)
  self = _meas2:new()
  self.name = autoname(param)
  self.clname = "meas2"
  self.parameters = {
    maxctd = true, maxiat = true, vci = false, out = true
  }
  self:adjust(param)
  assert(param.maxctd > 0, self.name..": parameter 'maxctd' > 0 required.")
  self.ctd_max = param.maxctd + 1
  assert(param.maxctd > 0, self.name..": parameter 'maxiat' > 0 required.")
  self.iat_max = param.maxiat + 1
  self.vci = param.vci or -1
  if not param.vci then 
    self.inp_type = DataType
  else
    self.inp_type = CellType
  end
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- Meas3 Object
--==========================================================================

_meas3 = meas3
--- Definition of class 'meas3'.
meas3 = class(_meas3)

--- Constructor for class 'meas'.
-- Measurement device for inter arrival times and cell transfer delays.<br>
-- <code>ref = yats.meas3:new{[name=]"objname",[slotlength=D,][StatTimer=0.1s,][ [ctd=true]|[ctd={min,max}, ctddiv=N], ][ [iat=true]|[iat={min,max}, iatdiv=10], ][erange=true,][out={"next", "pin"}]}</code>.
-- For further details, see file header in meas3.c.
-- @param param table - Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> slotlength<br>
--    Length of a timeslot in real-time. 
-- <li> stattimer (optional)<br>
--    Update of rate in seconds. Default: 0.1 s. 
-- <li> ctd (optional)<br>
--    Cell transfer delay - given either as boolean value or min/max value. 
--    Default: ignored, if not given. <br>
--    a) {min, max}: Provide cumulative mean value and extreme values (min/max). 
--                   Updated "on-the-fly" with each arrival. <br>
--    b) true or false: if true: ctd_min = ctd_max = 0. 
-- <li> ctddiv (optional)<br>
--    Only used in conjunction with given 'ctd' values. Measured values are
--    first divided by 'ctddiv'. The pair ctd(min,max) specifies the figures after 
--    dividing by 'ctddiv'. The division influences 'only' the detailed distribution. 
--    'ctd_min', 'ctd_max' and 'ctd_mean' remain unscaled. 
-- <li> iat (optional)<br>
--    Inter arrival time - given either as boolean value or min/max value. 
--    Default: ignored, if not given. <br>
--    a) {min, max}: Provide cumulative mean value and extreme values (min/max). 
--                   Updated "on-the-fly" with each arrival. <br>
--    b) true or false: if true: iat_min = iat_max = 0. 
-- <li> iatdiv<br>
--    Only used in conjunction with given 'iat' values. Measured values are
--    first divided by 'iatdiv'. iat(min,max) specifies the figures after 
--    dividing by 'iatdiv'. The division influences 'only' the detailed distribution. 
--    'iat_min', 'iat_max' and 'iat_mean' remain unscaled. 
-- <li> connid or vci (optional)
--    If not given, all data objects are counted under index 0. If connid is given,
--    frames are expected. If vci is given, cells are counted. 
--    Either frames or cells are measured per connection id (connid or vci) in the
--    the range {min, max}. 
-- <li> erange (optional)
--    If 'erange' is given: out-of-range cells or frames cause an error. 
--    Otherwise: only the overall counter is incremented. 
-- <li> out (optional)
--    Connection to successor. If omitted, this device is a sink. <br>
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>.
-- </ul>.
-- @return table - Reference to object instance.
-- @see meas:init.
function meas3:init(param)
  self = _meas3:new()
  self.name = autoname(param)
  self.clname = "meas3"
  self.parameters = {
    slotlength = false,
    stattimer = false,
    ctd = false,
    ctddiv = false,
    iat = false,
    iatdiv = false,
    erange = false,
    connid = false,
    vci = false,
    out = false
  }
  
  self:adjust(param)
  
  -- scan parameters and initialise
  assert(not param.StatTimer or param.StatTimer > 0, "meas3: StatTimer must be > 0.")
  self.StatTimer = param.StatTimer or 1.0
  self.StatTimerInterval = self:TimeToSlot(self.StatTimer)
  alarml(self.evtStatTimer, self.StatTimerInterval)
  
  if param.ctd then
    -- ctd is given
    self.doMeanCTD = 1
    if type(ctd) == "boolean" then
      -- no range - set defaults
      self.ctd_min = 0
      self.ctd_max = 0
    else
      -- range - read it.
      self.ctd_min = param.ctd[1]
      self.ctd_max = param.ctd[2] + 1
      assert(self.ctd_min <= self.ctd_max, "meas3: invalid {ctd_min,ctd_max} pair.")
    end
    self.ctd_div = 1
    if self.ctd_max > 0 and param.ctddiv then
      self.ctd_div = param.ctddiv
      assert(self.ctd_div > 0, "meas3: ctd_div > 0 required.")
    end
  else
    -- parameter ctddiv is ignored.
    self.doMeanCTD = 0
    self.ctd_min = 0
    self.ctd_max = 0
    self.ctd_div = 1
  end
  
  -- same game as for ctd
  if param.iat then
    self.doMeanIAT = 1
    if type(param.iat) == "boolean" then
      self.iat_min = 0
      self.iat_max = 0
    else
      self.iat_min = param.iat[1]
      self.iat_max = param.iat[2]
      assert(self.iat_min <= self.iat_max, "meas3: invalid {iat_min, iat_max} pair.")
    end
    self.iat_div = 1
    if self.iat_max > 0 and param.iatdiv then
      self.iat_div = param.iatdiv
      assert(self.iat_div > 0, "meas3: iat_div > 0 required.")
    end
  else
    self.doMeanIAT = 0
    self.iat_min = 0
    self.iat_max = 0
    self.iat_div = 1
  end
  
  -- provide input data type
  self.inp_type = DataType
  if param.vci then 
    self.inp_type = CellType
    self.idx_min = param.vci[1]
    self.idx_max = param.vci[2]+1
  elseif param.connid then
    self.inp_type = FrameType
    self.idx_min = param.connid[1]
    self.idx_max = param.connid[2]+1
  end
  -- idx_max is the first index not used: histogram is stored dense:
  -- e.g. connid={1,3} => min,max = 1,4 => storage indices: min,max = 4-1 = 3
  self.idx_max = self.idx_max - self.idx_min
  if self.inp_type  == DataType then
    self.idx_min = 0
    self.idx_max = 1
  end
  self.doRangeError = param.errange or 0
  
  -- output is optional
  if param.out then
    self:defout(param.out)
  end
  self:definp(self.clname)
  return self:finish()
end

--==========================================================================
-- Distribution
--==========================================================================
_distrib = distrib
--- Definition of class 'line'.
distrib = class(_distrib)
--- Constructor for class 'distrib'.
-- An object providing a transformation table to other objects.
-- Various options exist to define the distribution.
--
-- <li> File<br>
--  Lua script file containing the distribution.<br>
--   The script must return a table identical to the table
--   format specified below under 'Table'.
-- <li> Table<br>
--   A table containing the distribution in the format. Use this to
--   define a distribution directly in the simulation script:
--   For every x value the propability p(x) is given. Values
--   with p(x) = 0 can be omitted.
-- <li> Function<br>
--   There are the following pre-defined distributions:
--   - Exponential (geometric) distribution with exponent 'e':<br>
--     <code>yats.distrib.geometric(e)]]</code><br>
--   - Binomial distribution with 'n' samples and probability 'p':<br>
--     <code>[yats.distrib.binomial(n, p)</code>.
-- 
-- @param param table - Parameter List
-- <ul>
-- <li> name (optional)<br>
--    Name of the display; default: "objNN"
-- <li> dist<br>
--    string - Filename of Lua script returning a table<br>
--      <code>yats.distrib{name = "aName", dist = "aFile"}</code><br>
--<br>
--    table - Table directly describing the distribution<br>
--       <code>yats.distrib{name = "aName", dist = { {2, 0.5}, {5, 0.5} }</code><br>
--<br>
--    function - Function returning a table with the distribution<br>
--         <code>yats.distrib{name = "aName", dist = yats.distrib:geometric(20.5)</code> or<br>
--         <code>yats.distrib{name = "aName", dist = yats.distrib:binomial(10, 0.3) </code>
-- </ul>.
-- @return table -  Reference to object instance.
function distrib:init(param)
  self = _distrib:new()
  self.name = autoname(param)
  self.clname = "distrib"
  self.parameters = {
    out = false, dist = true, distargs = false
  }

  self:adjust(param)
  self.DISTR_ERR = 1e-5
  if type(param.dist) == "table" then
    distrib.tab(self, param.dist)

  elseif type(param.dist) == "function" then
    param.dist(self, param.distargs)

  elseif type(param.dist) == "string" then
    self:file(param.dist)
  end
	  
  return self:finish()
end

function distrib:tab(t)
  local x, prob, xold, sum, mysum, pdf = 0, 0, 0, 0, 0, 0
  -- Check given table first
  for k, v in pairs(t) do
    x, prob = v[1], v[2]
    assert(x >= 1, "X lower than 1 encountered.")
    assert(x > xold, "Non-ascending X encountered.")
    xold = x
    assert(prob >= 0 and prob <= 1, "Invalid probability encountered")
    sum = sum + prob
  end
  assert(math.abs(1.0 - sum) < 1e-5, 
	 "distribution icconsistent: PDF of largest X is not equal to one")
  -- Now build the distribution
  local i = 1
  for pos = 1, RAND_MODULO, 1 do
    local z = (pos - 1 + 0.5) / RAND_MODULO
    while pdf < z do
      x, prob = t[i][1], t[i][2]
      i = i + 1
      pdf = pdf + prob / sum
      mysum = mysum + prob
    end
--    print("pos="..(pos-1), "pdf="..pdf, "prob="..prob, "mysum="..mysum, "z="..z)
    self.table[pos] = x
  end
end

function distrib:file(fname)
  local f, e = loadfile(fname)
  assert(f, e)
  local t = f()
  self:tab(t)
end


function distrib:geometric(param)
  local e = param.e or param[1]
  assert(e >= 1.0, "E cannot be smaller than 1.0 for geometric distribution.")
  local sum = 1.0
  local prob
  local qgeo = (e - 1.0) / e
  local pdf = 0.0
  local x = 0
  local mysum = 0
  for pos = 1, RAND_MODULO,1 do
    local z = (pos - 1 + 0.5) / RAND_MODULO
    while pdf < z do
      if x == 0 then
	prob = 1.0 - qgeo
	x = 1
      else
	x = x + 1
	prob = prob * qgeo
      end
      pdf = pdf + prob / sum
      mysum = mysum + prob
    end
--    print("pos="..(pos-1), "pdf="..pdf, "prob="..prob, "mysum="..mysum, "z="..z, "x="..x)
    self.table[pos] = x
  end
  return nil
end

function distrib:binomial(param)
  local n = param.n or param[1]
  local p = param.p or param[2]
  assert(n > 0, "N must be larger than 0 for binomial distribution.")
  assert(p >= 0 and p <= 1, "Invalid probability.")
  local sum = 1.0
  local prob
  local pdf = 0.0
  local x = 0
  local mysum = 0
  local xmin, xmax = 1, n
  for pos = 1, RAND_MODULO,1 do
    local z = (pos - 1 + 0.5) / RAND_MODULO
    while pdf < z do
      if x == 0 then
	x = 1
      elseif x <= xmax then
	x = x + 1
      else
	return "fatal error in binomial distribution calculation"
      end
      prob = self:binom(xmax, x - 1) * math.pow(p, x - 1) * 
	math.pow(1 - p, xmax - (x - 1))
      pdf = pdf + prob / sum
    end
    mysum = mysum + prob
--    print("pos="..(pos-1), "pdf="..pdf, "prob="..prob, "mysum="..mysum, "z="..z)
    self.table[pos] = x
  end
  return nil
end

function distrib:show()
  local xold = nilDistSrc
  for pos = 1, RAND_MODULO,1 do
    x = self.table[pos]
    if x ~= xold then
      print(string.format("%d  %g", pos, self.table[pos]))
      xold = x
    end
  end
end

function distrib:get()
  local t = {}
  for pos = 1, RAND_MODULO, 1 do
    table.insert(t, pos, self.table[pos])
  end
  return t
end

return yats