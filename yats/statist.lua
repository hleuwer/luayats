-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: statist.lua 420 2010-01-06 21:39:36Z leuwer $
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- @description LuaYats - Statistics.
-- Lua class for statistics.
-----------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- Multiplexer Object 
--==========================================================================

--- Definition of class 'confid'
_confidObj = confidObj
confidObj = class(_confidObj)
confid = confidObj

--- Constructor for class 'confid'.
-- The confid objects calculates a confidence interval. It provides methods
-- to add values to a dynamic array and to calculate statistical properties of
-- the samples of these values.
-- @param param table - parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> level<br>
--    Confidence level: 0.9, 0.95, 0.975 and 0.99 are supported.
-- </ul>
-- @return table - reference to object instance.
function confidObj:init(param)
  self = _confidObj:new()
  self.name = autoname(param)
  self.clname = "confid"
  self.parameters = {
    level = true
  }
  local l = param.level or 0.95
  assert(l == 0.9 or l == 0.95 or l == 0.975 or  l == 0.99,
	 self.name.." only levels 0.9/0.95/0.975/0.99 available")
  self:adjust(param)
  return self:finish()
end

confidObj._getCorr = confidObj.getCorr

function confidObj:getCorr(lag, batchsize)
  assert(lag <= self.nVals, string.format("%s: lag too large, only %d values available",
					  self.name, self.nVals))
  return self:_getCorr(lag, batchsize)
end

return yats
