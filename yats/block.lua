---------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: block.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Hierarchical block structures (module: yats).
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- <b>Definition of class 'block':</b><br>
-- The class 'block' is used to establish hierarchichal networks. In order to
-- use this functionality a user defined class must be declared, that inherits 
-- from class 'block', e.g.<br>
-- <code>yats.switch = class(yats.block)</code>.<br>
-- <br>
-- The constructor for a 'block' class looks very similar to the constructor for
-- normal network object classes:<br>
-- <code>function yats.switch:new(param) ... end </code>.<br>
-- <br>
-- Once the user defined class (here 'switch') has been defined, any number of instances
-- can be created and connected.<br>
-- The following rules must be followed during definition of a 'block' object class:<br>
-- <ul>
-- <li> The baseclass's init() routine must be called at the beginning: <code>self.super():init()</code>
-- <li> Wrap all names of internal names using <code>self:localname("mux")</code>, which generates
--    the extended name "switch/mux".
-- <li> Since 'block' object's inputs may be connected to different internal objects
--    an object name must be given in input declaration:
--    <code>self:definp{self:localname("mux"), "in1"}</code>.
-- </ul><br>
-- See the file 'switch.lua' for an example where a multiplexer and a demultiplexer
-- are grouped into a 'block' object 'switch'. An example for the usage of a 'switch'  can 
-- be found in the 'example' folder in luayats/src/lua/examples.
-- <br><br>
-- <b>Internals:</b><br>
-- The 'block' class functions as a proxy to internal objects, that are no longer 
-- visible to the main configuration script.Lua classes for traffic sources.
-- This is achieved in the following way:<br>
-- <ul>
-- <li> All internal objects name are automatically prefixed with the name of the block
--    instance, e.g. switch/mux or switch/demux.
-- <li> Input and output connector (dummy) objects are instantiated inside the block, that
--    connect internal objects to the outer world. The performance penalty for these
--    extra objects is minimal (one function call).
-- <li> The 'new' baseclass 'block' re-implements the following methods from 'root':~
--    <code>connect(), handle(), definp(), defout(), finish()</code>. 
--    If an outer object is connected to 'block' object input connectors 'in1, in2, ...' 
--    are used, which are real C-level objects. When connecting the 'block' object to 
--    the outer world, an output connector 'out1, out2, ...' are
--    used. Input and output connectors simply forward data items.
--    Internal objects are connected to input and output connectors in a normal way.
--  <li> The resulting netlist is still flat.
-- </ul>
---------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

local log = kernel_log

block = class(root)

---------------------------------------------------------------
-- Constructor for hierarchical network objects.
-- @param param table - Parameters to 'block' object instance.
-- @return Reference to a 'block' instance.
function block:init(param)
  self = root:new()
  self.objects = {}
  self.nobjects = 0
  self.ninp = 0
  self.noutp = 0
  self.inputs = {}
  self.outputs = {}
  return self
end

---------------------------------------------------------------
-- Adds a local object to the block.
-- @param obj table Object to be added.
-- @return Reference to the object.
function block:addobj(obj)
  -- Run through output list and prepend block name
  log:debug(string.format("Adding object to block '%s': '%s' of class '%s' (%s)\n", 
			  self.name, obj.name, obj.clname, type(obj)))
  for _, v in pairs(self.objects) do
    assert(v.name ~= obj.name, string.format("Object '%s' in block '%s' already defined.", 
				   obj.name, self.name))
  end
  table.insert(self.objects, obj)
  return obj
end

---------------------------------------------------------------
-- Define the 'block' object's outputs.
-- @param outputs table - A table of outputs. See also yats.html.
-- @return none.
function block:defout(outputs)
  for i = 1, table.getn(outputs) do
    self.noutp = self.noutp + 1
    -- Create an output connector and set it's output to the outputs given
    -- during instantiation of the block
    local obj = self:addobj(dummy{self.name.."/out"..self.noutp, out = outputs[i]})
    self.outputs[obj.name] = obj
  end
end

---------------------------------------------------------------
-- Define one input of the 'block' object.
-- @param out table or string - A output descriptor. See yats.html.
-- @return none.
function block:definp(out)
  self.ninp = self.ninp + 1
  -- Create an input connector and set it's output to the given input.
  local obj = self:addobj(dummy{self.name.."/in"..self.ninp, out = out})
  self.inputs[obj.name] = obj
end

---------------------------------------------------------------
-- Finish a 'block' object class definition.
-- @return References to Lua-object and the name of the object.
function block:finish()
  sim:insert(self)
  return self, self.name
end

---------------------------------------------------------------
-- Extends local names by the block's name.
-- @param name string Local object name.
-- @return string Global object name, e.g. switch/mux.
function block:localname(name)
  return self.name.."/"..name
end

---------------------------------------------------------------
-- Wrapper for connecting objects.
-- @return none.
function block:connect()
  local outcon
  -- Run through the output connectors and use their connect() method.
  for _,outcon in pairs(self.outputs) do
    outcon:connect()
  end
end

---------------------------------------------------------------
-- The 'block' objects <code>handle()</code> method.
-- It is used to get handle response for the peer object. The method is 
-- called during the connect procedure by the peer object.
-- @param peer table - Reference to the object receiving the handle.
-- @param pin string - Input pin name.
-- @return Handle of input pin and the input pin object.
function block:handle(peer, pin)
  -- Look for the correspondig input connector and return it's handle.
  -- self is the destination, peer the source
  log:debug(string.format("  looking for %s\n", self:localname(pin)))
  local obj = self.inputs[self:localname(pin)]
  log:debug(string.format("  %s'proxy (block) requests handle from '%s': ", self.name, obj.name))
  local shand = obj:handle(peer, obj.clname)
  log:debug(string.format("returned [%d]\n", shand))
  return shand, obj
end

---------------------------------------------------------------
-- Notification of simulation time reset.
-- Calls method <code>restim()</code> of all objects included in the block.
-- @return none.
function block:restim()
  for _,v in pairs(self.objects) do
    v:restim()
  end
end

return yats