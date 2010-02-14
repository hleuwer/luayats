local lxp = require "lxp"
require "lxp.lom"

local tinsert = table.insert

module("yats", yats.seeall)

doxy = {}


local function getmember(tab)
   local m = {}
   for _,v in ipairs(tab) do
      if type(v) == "table" then
	 m[v.tag] = v[1]
      end
   end
   return m
end

local function getclass(tab)
   local cl = {bases={}, methods={}, vars={}, enums={}, namedenums={}}
   for _, v in ipairs(tab) do
      if type(v) == "table" then
	 if v.tag == "name" then
	    cl.name = v[1]
	 elseif v.tag == "filename" then
	    cl.filename = v[1]
	 elseif v.tag == "base" then
	    tinsert(cl.bases, v[1])
	 elseif v.tag == "member" then
	    local m = getmember(v)
	    if v.attr.kind == "function" then
	       cl.methods[m.name] = m
	    elseif v.attr.kind == "variable" then
	       cl.vars[m.name] = m
	    elseif v.attr.kind == "enumvalue" then
	       cl.enums[m.name] = m
	    elseif v.attr.kind == "enumeration" then
	       cl.namedenums[m.name] = m
	    end
	 end
      end   
   end
   return cl
end

local function classindex(tab)
   local t = {}
   local duplicates = {}
   for _, v in pairs(tab) do
      if type(v) == "table" then
	 if v.tag and v.tag == "compound" then
	    if v.attr and v.attr.kind == "class" or v.attr.kind == "struct" then
	       local cl = getclass(v)
	       if t[cl.name] then
		  tinsert(duplicates, cl.name)
	       end
	       t[cl.name] = cl
	    end
	 end
      end
   end
   return t, duplicates
end

function doxy:new(tagfile)
   local t = {}
   setmetatable(t, {__index = doxy})
   local fin = assert(io.open(tagfile, "r"))
   local s = string.gsub(fin:read("*a"), "%>%s*","%>")
   fin:close()
   -- parse xml string
   t.tags = lxp.lom.parse(s)
   -- build class index
   t.classes, t.duplicates = classindex(t.tags)
   return t
end



