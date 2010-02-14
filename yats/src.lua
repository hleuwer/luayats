---------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: src.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Traffic source classes.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for traffic sources.
---------------------------------------------------------------

require "yats.tcpip" -- for cbrfram source

module("yats", yats.seeall)

--==========================================================================
-- CBR Cell Source Object.
--==========================================================================

_cbrsrc = cbrsrc
--- Definition of source object 'cbrsrc' (CBR Source).
cbrsrc = class(_cbrsrc)

--- Constructor for class 'cbrsrc'.
-- Constant bit rate (CBR) source.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>delta<br>
--    Cell rate in ticks
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function cbrsrc:init(param)
  self = _cbrsrc:new()
  self.name = autoname(param)
  self.clname = "cbrsrc"
  self.parameters =  {
    delta = true, vci = true, out = true
  }

  -- Adjust parameters.
  self:adjust(param)
  -- 1. create a yats object
  
  -- Set paramaters.
  self.delta = param.delta
  self.vci = param.vci
  
  -- Init output table.
  self:defout(param.out)
  
  -- Init input table.
  -- no inputs
  
  -- Finish with C++ act() if necesary
  return self:finish()
end


--==========================================================================
-- GEO Cell Source Object.
--==========================================================================

_geosrc = geosrc			
--- Definition of source object 'geosrc' (Bernoulli Source).
geosrc = class(_geosrc)		

--- Constructor for class 'geosrc'.
-- Bernoulli Source.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>ed<br>
--    Mean of geometrical distributed cell distance
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function geosrc:init(param)		
  self = _geosrc:new()			
  self.name = autoname(param)
  self.clname = "geosrc"
  self.parameters =  {
    ed = true, vci = true, out = true
  }

  -- Adjust parameters.
  self:adjust(param)				
  
  -- Set paramaters
  self.ed = param.ed				
  self.vci = param.vci			
 
  -- Init output table
  self:defout(param.out)			

  -- Init input table
  -- no inputs

  -- Finish with C++ act() if necesary
  return self:finish()			
end						

--==========================================================================
-- BSSRC Cell Source Object.
--==========================================================================
_bssrc = bssrc
--- Definition of source object 'bssrc' (ON-OFF Source).
bssrc = class(_bssrc)

--- Constructor for class 'bssrc'.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>ex<br>
--    Mean number of cells per burst
-- <li>es<br>
--    Mean duration of OFF (silence) phase (in time slots)
-- <li>delta<br>
--    Cell distance in ON phase (in time slots)
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>des<br>
--    If set to 1, use "es" in a deterministic way instead of geomtrically 
--    distributing the duration of the OFF period
-- <li>dex<br>
--    If set to 1, use "ex" in a deterministic way instead of geomtrically 
--    distributing the duration of the ON period
-- <li>out<br>
--    Connection to successor
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function bssrc:init(param)
  self = _bssrc:new()
  self.name = autoname(param)
  self.clname = "bssrc"
  self.parameters =  {
    ex = true, es = true, delta = true, vci = true, des = false, dex = false, out = true
  }

  -- Adjust parameters.
  self:adjust(param)

  -- Init remind: setting default values in the form ... = param.ex or <some-default>
  -- Set paramaters
  self.ex = param.ex 
  self.es = param.es
  self.delta = param.delta
  self.vci = param.vci
  -- Init output table
  self:defout(param.out)

  local a,b = self:finish()
  self:SetDeterministicEx(param.dex or 0)
  self:SetDeterministicEs(param.des or 0)

  -- Finish with C++ act() if necesary
  return a,b
end


--==========================================================================
-- LIST Cell Source Object.
--==========================================================================

_listsrc = listsrc
--- Definition of source object 'listsrc' (Explicit Cell Sequence).
listsrc = class(_listsrc)

--- Constructor for class 'listsrc'.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>ntim<br>
--    Number of entries in cell distance table
-- <li>delta<br>
--    Cell distance table (in time slots)
-- <li>rep (optional)<br>
--    Repeat (wrap) when end of table reached
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function listsrc:init(param)
  self = _listsrc:new()
  self.name = autoname(param)
  self.clname = "listsrc"
  self.parameters =  {
    ntim = false, delta = true, vci = true, rep = false, out = true
  }
  -- Adjust parameters.
  self:adjust(param)

  -- Set paramaters.
  if param.ntim then
    assert(param.ntim >= 1, "invalid N")	
    self.ntim = param.ntim
  end
  self.rep = param.rep or false

  local j=0
  for i,v in ipairs(param.delta) do
    assert(v >= 1, "invalid DELTA")
    j = i
  end
  if not param.ntim then self.ntim = j end
  assert(j == self.ntim, "No. of DELTAs must be equal to ntim")	
  
  for i,v in ipairs(param.delta) do
        self:SetTims(i-1,v)
  end

  self.vci = param.vci
  
  -- Init output table
  self:defout(param.out)

  -- Init input table
  -- no inputs
  
  -- Finish with C++ act() if necesary
  return self:finish()
end

--==========================================================================
-- Distribution Source
--==========================================================================
_distsrc = distsrc
--- Definition of source object 'distsrc' (DIST Source).
distsrc = class(_distsrc)
--- Constructor for class 'distsrc'.
-- Distribution (DIST) source.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>dist<br>
--    Reference to distribution object
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function distsrc:init(param)
  self = _distsrc:new()
  self.name = autoname(param)
  self.clname = "distsrc"
  self.parameters =  {
    dist = true, vci = true, out = true
  }
  -- Adjust parameters.
  self:adjust(param)
  
  -- Set paramaters
  self.vci = param.vci

  -- Distribution object.
  assert(param.dist, "invalid distribution object")
  self.dist = param.dist

  -- Distribution table.
  local msg = GetDistTabMsg:new_local()
  local err = self.dist:special(msg, nil)
  assert(not err, err);
  self:setTable(msg:getTable())

  -- Init output table
  self:defout(param.out)
  
  -- Init input table
  -- no inputs
  
  -- Finish with C++ act() if necesary
  return self:finish()
end

--==========================================================================
-- MMBP Cell Source Object.
--==========================================================================

_mmbpsrc = mmbpsrc
--- Definition of source object 'mmbpsrc' (CBR Source).
mmbpsrc = class(_mmbpsrc)

--- Constructor for class 'mmbpsrc'.
-- Marcov Modulated Bernoulli Process (MMBP) source.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li>eb<br>
--    Mean burst duration in slots
-- <li>es<br>
--    Mean silence duration in slots
-- <li>ed<br>
--    Mean cell distance inside the burst in slots
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function mmbpsrc:init(param)
  self = _mmbpsrc:new()
  self.name = autoname(param)
  self.clname = "mmbpsrc"
  self.parameters =  {
    eb = true, es = true, ed = true, vci = true, out = true
  }

  -- Adjust parameters.
  self:adjust(param)
  
  -- Set paramaters.
  self.eb = param.eb
  self.es = param.es
  self.ed = param.ed
  self.vci = param.vci
  
  -- Init output table.
  self:defout(param.out)
  
  -- Init input table.
  -- no inputs
  
  -- Finish with C++ act() if necesary
  return self:finish()
end

--==========================================================================
-- GMDP Cell Source Object.
--==========================================================================
local function gmdp_params(self, param)

  -- Adjust parameters
  self:adjust(param)

  -- Set paramaters.
  assert(param.dist or param.ex, "parameter 'ex' or 'dist' expected.")
  self.n_stat = param.nstat

  -- Allocate C-level tables
  self:allocTables(self.n_stat)

  -- The following steps require parameters which are only visible
  -- to Lua. So we do this here instead of act.
  for i = 0, self.n_stat - 1 do
    self:setDelta(param.delta[i+1], i)
  end

  if param.ex then
    for i = 1, self.n_stat do
      local a = getGeo1Handler(param.ex[i])
      local b = getGeo1Table(a)
      self:setTable(getGeo1Table(getGeo1Handler(param.ex[i])), i-1)
    end
  elseif param.dist then
    self.dist = {}
    local msg = GetDistTabMsg:new_local()
    for i = 1, self.n_stat do
      assert(param.dist[i], "invalid distribution object.")
      self.dist[i] = param.dist[i]
      local err = self.dist[i]:special(msg, nil)
      assert(not err, err);
      self:setTable(msg:getTable(), i-1)
    end
  else
    error("parameter 'ex' or 'dist' expected.")
  end

  for i = 1, self.n_stat * self.n_stat do
    self:setTrans(param.trans[i], i-1)
  end
  local ok = false
  for i = 1, self.n_stat do
    if self:getDelta(i-1) ~= 0 then ok = true end
  end
  assert(ok, string.format("%s: only state with zero bitrates", self.name))

  self.vci = param.vci
end

_gmdpsrc = gmdpsrc
--- Definition of source object 'gmdpsrc' (CBR Source).
gmdpsrc = class(_gmdpsrc)

--- Constructor for class 'gmdpsrc'.
-- Generally Modulated Determistic Process (GMDP) source.
-- The GMDP source comes in two flavours:
-- <br> a) with arbitrarily distributed phase durations.
-- <br> b) with geometrically distributed phase durations
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" <br>
-- 1. Arbitrarily distributed phase durations
-- <li>delta<br>
--    List of cell spacings
-- <li>dist<br>
--    Distribution of the number of cells per state
-- <li>trans<br>
--    Transition probablities
--<br>
-- 2. Geometrically distributed phase durations
-- <li>nstat<br>
--    Number of states
-- <li>delta<br>
--    List of cell spacings
-- <li>ex<br>
--    Mean number of cells per state
-- <li>trans<br>
--    Transition probabilities
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
function gmdpsrc:init(param)
  self = _gmdpsrc:new()
  self.name = autoname(param)
  self.clname = "gmdpsrc"
  self.parameters =  {
    nstat = true, delta = true, ex = false, dist = false,
    trans = true, vci = true, out = true
  }

  -- Adjust and set paramaters.
  gmdp_params(self, param)

  -- Init output table.
  self:defout(param.out)
  
  -- Init input table.
  -- no inputs
  
  -- Finish with C++ act() if necesary
  return self:finish()
end


--==========================================================================
-- GMDPSTOP Cell Source Object.
--==========================================================================

_gmdpstop = gmdpstop
--- Definition of source object 'cbrsrc' (CBR Source).
gmdpstop = class(_gmdpstop, "u")
--- Constructor for class 'gmdpstop'.
-- Generally Modulated Determistic Process (GMDP) start/stop source.
-- The GMDP source comes in two flavours:
-- <br> a) with arbitrarily distributed phase durations.
-- <br> b) with geometrically distributed phase durations
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN" <br>
-- 1. Arbitrarily distributed phase durations
-- <li>delta<br>
--    List of cell spacings
-- <li>dist<br>
--    Distribution of the number of cells per state
-- <li>trans<br>
--    Transition probablities
--<br>
-- 2. Geometrically distributed phase durations
-- <li>nstat<br>
--    Number of states
-- <li>delta<br>
--    List of cell spacings
-- <li>ex<br>
--    Mean number of cells per state
-- <li>trans<br>
--    Transition probabilities
-- <li>vci<br>
--    Virtual connection id for the cell
-- <li>out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table -  Reference to object instance.
-- @see gmdpstop:init.
function gmdpstop:init(param)
  self = _gmdpstop:new()
  self.name = autoname(param)
  self.clname = "gmdpstop"
  self.parameters =  {
    nstat = true, delta = true, ex = false, dist = false,
    trans = true, vci = true, out = true
  }
  
  -- Adjust and set paramaters.
  gmdp_params(self, param)

  -- Init output table.
  self:defout(param.out)

  -- Init input table.
  self:definp("start")
  
  -- Finish with C++ act() if necesary
  return self:finish()
end

return yats
