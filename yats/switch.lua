---------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: switch.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Example block structure.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- An example for block structures. This example assembles a
-- data switch consisting of a multiplexer with a following
-- demultiplexer. The switch object can be instantiated
-- and used in a simulation script in the same way as any other
-- node object.<br>
-- <br>
-- Note: The switch object is not really usable node object.

module("yats", yats.seeall)

require "yats.block"
local log = log

--- Definition of a 'switch' class (Example).
switch = class(block)

--- Constructor of  'switch' class.
-- See node object 'mux' for parameters.
function switch:init(param)
  self = block()
  -- Generic part
  self.name = autoname(param)
  self.clname = "switch"
  self.parameters = {
    ninp = true,
    buff = true,
    out = true,
    maxvci = true
  }
  -- Adjust parameters to lower case and do parameter checking.
  self:adjust(param)

  -- Create internal objects
  self.mux = self:addobj(mux{
			   self:localname("mux"), 
			   ninp = param.ninp, 
			   buff = param.buff, 
			   out = {self:localname("demux")}
			 })
  local dout = {}
  for i = 1, table.getn(param.out) do
    dout[i] = {self:localname("out"..i)}
  end
  self.demux = self:addobj(demux{
			     self:localname("demux"), 
			     maxvci = param.maxvci, 
			     nout = table.getn(param.out), 
			     out = dout
			   })
  -- Add input connectors
  for i = 1, param.ninp do
    self:definp{self:localname("mux"), "in"..i}
  end

  -- Add output connectors
  self:defout(param.out)

  -- Finish the block definition
  return self:finish()
end

--- Wrappers to internal object methods.
function switch:signal(msg) self.demux:signal(msg) end
function switch:getRouting()  return self.demux:getRouting() end
function switch:getQLen()  return self.mux:getQLen() end
function switch:getQMaxLen() return self.mux:getQMaxLen() end
function switch:getLoss(n) return self.mux:getLoss(n) end
function switch:getLossVCI(n) return self.mux:getLoss(n) end

return yats
