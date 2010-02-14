-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2003.
-- @author Herbert Leuwer.
-- @release 3.0 $Id: object.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Object oriented programming.
-- <br>
-- <br><b>module: _G</b><br>
-- <br>
-- This module supports object oriented programming in Lua 5.0. 
-- Inheritance is implemented using metamethods.<br>
-- <br>
-- <b>Class Definition and Method <code>new()</code></b>
-- <pre>
--    aClass = class([base])<br>
--    <br>
--    -- Implicit Constructor/Initializer - Form 0<br>
--    -- Object table is implictly created and object<br>
--    -- is implicitly initialised from a parameter table.<br>
--<br>
--    -- Constructor/Initialiser - Form 1: single parameters<br>
--    -- Object table is implictly created<br>
--    function aClass:init(p1, p2, ...)<br>
--      self.p1 = p1<br>
--      self.p2 = p2<br>
--      ...<br>
--    end<br>
--<br>
--    -- Constructor/Initialiser - Form 2: a single table with parameters<br>
--    -- Object table is implictly created<br>
--    function aClass:init(pt)<br>
--       self.p1 = pt.p1<br>
--       self.p2 = pt.p2<br>
--       ...<br>
--    end<br>
--<br>
--    -- Constructor - Form 3: an explicit constructor function with either<br>
--    --                       single parameters or a parameter table<br>
--    -- Object table is implictly created<br>
--    function aClass:new(v1, v2, ...)<br>
--      -- any work before creating the actual object table<br>
--      self = self:create{<br>
--        p1 = p1,<br>
--        p2 = p2,<br>
--        ...<br>
--      }<br>
--      ...<br>
--    end<br>
--<br>
--    -- A simple mthod.<br>
--    function aClass:doprint(a)<br>
--      print(a)<br>
--    end<br>
-- </pre>
-- Form 0 does not need any explicitly constructor definition.<br>
-- Note, that parameters are only initialised automatically if all
-- parameters are defined in a single parameter table.
-- <br>
-- If the constructor form 1 or 2 are used, the object table
-- is implicitly created and the <code>init()</code> method simple initializes
-- the object instance. While form 1 takes single parameters
-- The name of the constructor can freely chosen. However, it is 
-- recommended to use the name <code>new</code>. Those forms are useful to
-- provide default values for unequipped parameters.<br>
--<br>
-- While the forms 1 and 2 provide a reference to the object instance
-- as implicit parameter <code>self</code>, form 3 provides a reference to the
-- class. It can therefore be used to dynamically add methods to the
-- class. The function <code>create(...)</code> which is defined for all classes
-- returns a table to a new object instance with all metamethods set
-- for inheritance. Note, that the name of the constructor can be freely
-- chosen. However, it is recommended to use the name <code>new(...)</code>.
--
-- The function <code>create(...)</code> is implicitly called during object
-- instantiation for constructor forms 0, 1 and 2.<br>
-- <br>
-- <b>Object Instantiation and method reference</b>
-- <pre>
--     -- Constructor Form 0, 2: <br>
--     anInstance = aClass{p1=v1, p2=v2, ..} <br>
-- <br>
--     -- Constructor Form 1: <br>
--     anInstance = aClass(v1, v2, v3) <br>
-- <br>
--     -- Constructor Form 3: <br>
--     anObject = aClass:new(args) <br>
-- <br>
--     -- Method call: <br>
--     anObject:doprint("hello") <br>
-- </pre>
-- The package provides a '__call' metamethod, that provides the 
-- possibility to instantiate an object via a function call.
--
-- This function is not available for constructor form 3 where the
-- defined constructor must be called explicitly.<br>
-- <br>
-- <b>Object Initialisation</b><br>
-- There is a build-in intialisation function <code>init(params)</code>, which
-- copies parameters given in table <code>params</code> into the table 
-- representing the object instance. This initialisation function can
-- be overloaded by providing a user defined initialisation routine 
-- <code>init(...)</code> which is the called instead of the build-in 'init' 
-- function.<br>
-- Note, that the build-in 'init' function only accepts tables as 
-- parameters, while a user defined 'init' accepts any kind of parameters.
-- The function <code>create(...)</code> forwards any parameter transparently 
-- to the init function. See an example below that accepts a number of 
-- different parameters:
-- <pre>
--    aClass = class() <br>
--    function aClass:init(clname, a, s) <br>
--      self.clname = clname <br>
--      self.private = {a = a, s = s} <br>
--      return self <br>
--    end <br>
-- </pre>
-- The initialisation function can return a value, which is from now on
-- used as object table. Typically this can be ignored. However, there
-- are cases where this is interesting, e.g. during inheritance from
-- userdata objects.<br>
-- <br>
-- <b>Inheritance</b><br>
-- Inheritance is achieved by supplying the name of the baseclass to the
-- class definition. All methods and public data are inherited by derived
-- classes:
-- <pre>
--    aDerivedClass = class(aBaseClass)
-- </pre>
-- 
-- <br>
-- <b>Build-in Methods</b><br>
-- <br>
-- The following methods are always provided:
-- <pre>
--    superclass()         returns reference to base class or nil. <br>
--    this()               returns reference to running object. <br>
--    isa(anyClassName)    checks whether anyClassName and the running task
--                         are actually the same.
-- </pre>
-- <br>
-- <b>Inheritance from C++ objects</b><br>
-- <br>
-- Unfortunately, it is not possible to directly inherit from such an object, 
-- because C++ instances are of type <code>userdata</code> and Lua instances are of 
-- type <code>table</code> and it is not possible to set metamethods for userdata
-- from a Lua script. Note, that this is also true for tolua, where the C++ 
-- instance appears as table in Lua, but is actually a userdaa type.<br>
-- <br>
-- However, with tolua it is possible to utilise the inheritance chain
-- of the C universe. Any userdata object can be extended using Lua defined
-- methods, which are properly called even from derived objects. Tolua
-- searches it's inheritance chain until a proper userdata object is found
-- that provides the requested function.
-----------------------------------------------------------------------------------

object = {}

module("_G")

--- Declaration of a 'proxy' class.
-- @param methods table - Table with method names.
-- @param access function - Function returning userdata to C++ object.
-- @return Reference to proxy class.
function proxyclass(methods, access, baseclass)
  local cl = class(baseclass)
  cl:map(methods, access)
  return cl
end

--- Declaration of a class.
-- @param cbase C-level baseclass or nil.
-- @param lbase Lua-level baseclass.
-- @return Reference to class.
function class(cbase, lbase)

   -- create a table for the new class
   local new_class = {}
   
   -- create a metatable for the new class
   local class_mt = {__index = new_class}
   
   --- Initializer - used in constructor. Copies fields as public members to the class.
   -- This routine is overwritten by a user-suplied 'init' routine.
   -- @param init Table with fields to initialize in class.
   -- @return Reference to class instace.
   function new_class:init(init)
      local i,v
      if type(init) == "table" then
	 for i,v in pairs(init) do
	    self[i] = v
	 end
      end
      return self, nil
   end
   
   
   --- Creator - used in constructor to create a class instance. Calls eventually a generic
   -- initialisation routine `init'.
   -- @param init table - Optional table with initialization fields.
   -- @return Instance of class.
   function new_class:create(...)
      local newinst, err, ni = {}, nil, nil
      if table.getn(arg) == 1 and type(arg[1]) == "table" then
	 ni, err = self.init(newinst, arg[1])
	 if ni then newinst = ni end
      else
	 ni, err = self.init(newinst, unpack(arg)) or newinst
	 if ni then newinst = ni end
      end
      if type(newinst) == "table" then
	 -- For pure Lua object we can use metamethods
	 setmetatable(newinst, class_mt)
	 
      elseif type(newinst) == "userdata" then

	 -- For userdata table we cannot set metamethods. Hence
	 -- we must copy the field contents
	 for i,v in pairs(new_class) do 
	    newinst[i] = v 
	 end
	 
	 -- Copy all Lua defined methods from the Lua base class as well
	 if lbase then
	    for i,v in pairs(lbase) do 
	       if type(v) == "function" and i ~= "init" then
		  newinst[i] = v 
	       end
	    end
	 end
      end
      return newinst, err
   end
   
   --- For compatibility: other name.
   new_class.__init = new_class.create
   
   -- Init inheritance for method and data access.
   -- We add also a syntactic sugar to create a class by simply calling the instance object.
   -- e.g. aClass = class(); cl = aClass() instead of cl = aClass:new().
   if cbase ~= nil then
      setmetatable(new_class, {__index = cbase, __call = new_class.create})
   else
      setmetatable(new_class, {__call = new_class.create})
   end

   -- some generic class function that are always helpful
   -- return reference of the instance
   function new_class:this()
      return new_class
   end
   
   -- return reference to super class object
   function new_class:superclass()
      return cbase, lbase
   end
   
   --- A nicer name than superclass.
   new_class.super = new_class.superclass
   
   -- return true if the caller is an instance of the class
   function new_class:isa(class)
      local b_isa = false
      local cur_class = new_class
      -- run through the class hierarchy and test all classes
      while (cur_class ~= nil) and (b_isa == false) do
	 if cur_class == class then
	    b_isa = true
	 else
	    cur_class = cur_class:superclass()
	 end
      end
      return b_isa
   end
   
   local function mapping(method, access)
      return function(self, ...)
		-- Note, that self is not forwarded to the method.
		if access then
		   local obj = access()
		   return obj[method](obj, unpack(arg))
		else
		   local obj = object.getproxyclient(self)
		   return obj[method](obj, unpack(arg))
		end
	     end
   end
   
   function new_class._map(self, methods, access)
      local key, val
      for key, val in methods do
	 self[val] = mapping(val, access)
      end
   end
   
   return new_class
end

--- Set the client in the instance of a proxy class.
-- The function simply assigns a userdata reference to the key
-- inst._PROXYCLIENT.
-- @param inst table Object (instance of proxy class).
-- @param client userdata Reference to userdata (C++ object).
-- @return Referenc to userdata (C++ object).
function object.setproxyclient(inst, client)
  inst._PROXYCLIENT = client
  return client
end

--- Get the client previously set in the instance of a proxy class.
-- @param inst table Object (isntance of proxy class).
-- @return Reference to userdata (C++ object).
function object.getproxyclient(inst)
  return inst._PROXYCLIENT
end

