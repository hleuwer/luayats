-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: polshap.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Policers and shapers.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for policers and shapers.
-----------------------------------------------------------------------------------

module("yats", yats.seeall)

--==========================================================================
-- Leaky Bucket Object.
--==========================================================================

_lb = lb
--- Definition of sourceo object 'lb' (Leaky Bucket).
lb = class(_lb)

--- Constructor for class 'lb'.
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
function lb:init(param)
  self = _lb:new()
  self.name = autoname(param)
  self.clname = "cbrsrc"
  self.parameters =  {
    inc = true, dec = true, size = true,  vci = false, 
    out = true
  }
  self:adjust(param)
  -- 1. create a yats object
  
  -- 2. set object vars in yats object
  self.lb_inc = param.inc
  self.lb_dec = param.dec
  self.vci = param.vci
  if self.vci then
    self.inp_type = CellType
  else
    self.inp_type = DataType
  end
  self.lb_size = 0
  self.last_time = 0

  -- 3. init output table
  self:defout(param.out)
  
  -- 3. init input table
  self:definp(self.clname)
  
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- Peak Rate Shaper  Object.
--==========================================================================
_shap2 = shap2
--- Definition of sourceo object 'shaper2' (Shaper).
shap2 = class(_shap2)

--- Constructor for class 'shap2'.
-- Peak rate shaper with buffer.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>delta<br>
--    Peak cell rate in ticks. 
-- <li>buff<br> 
--    Cell buffer size. 
-- <li>out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table -  Reference to object instance.
function shap2:init(param)
  self = _shap2:new()
  self.name = autoname(param)
  self.clname = "shap2"
  self.parameters =  {
    delta = true, buff = true, out = true
  }
  self:adjust(param)
  -- 2. set object vars in yats object
  assert(param.delta > 1, self.name ..": delta must be >= 1.")
  self.delta = param.delta
  self.q_max = param.buff 
  self.q_len = 0
  self.next_time = 0
  self. bucket = 0
  self.delta_short = self.delta
  self.delta_long = self.delta_short + 1
  self.splash = self.delta_long - self.delta

  -- Outputs 
  self:defout(param.out)
  
  -- Inputs
  self:definp(self.clname)
  
  return self:finish()
end

return yats
