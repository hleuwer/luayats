collectgarbage("stop")
require "yats.core"
require "yats.graphics"
require "yats.stdlib"
require "yats.statist"
require "yats.logging.console"
require "yats.logging.ring"
require "yats.dummy"
require "yats.misc"
require "yats.muxdmx"
require "yats.muxevt"
require "yats.src"
require "yats.tcpip"
require "yats.user"
require "yats.polshap"
require "yats.agere"
require "yats.rstp"
require "yats.block"
require "yats.switch"
require "yats.n23"
require "yats.htk"
require "yats.gui.editor"
require "yats.gui.menu"
require "yats.gui.toolbar"
require "yats.gui.runctrl"
require "toluaex"
require "lxp"
require "lxp.lom"
local htk = require "yats.htk"
require "yats.doxy"
require "lfs"
collectgarbage("restart")

local tinsert = table.insert
local methods_cache = {}
local members_cache = {}
local bases_cache = {}
local usecache = false
module("yats", yats.seeall)

local log = logging.console("INF %level %simtime (%realtime) %message\n")
log:setLevel("WARN")
log:info(string.format("LOGGER INF created: %s", os.date()))

local function sort(t, what)
--   log:debug("Sorting "..what)
   table.sort(t, function(u,v)
		    if u.key < v.key then return true end
		 end)
   return t
end

local method_pattern = {
   "^__%w+", "^_%w+", "^%.%w+",
   "isa", "super", 
   --   "superclass", 
   "this", 
   "create"
}
local member_pattern = {
   "^%.%w+", "^_%w+", "^._%w+", "tolua_.+"
}
local modules = {}
for i,v in pairs(yats) do
   if type(v) == "table" and v.__magic__ and (v.__magic__ == "module") then
      table.insert(modules, i)
   end
   log:debug(string.format("%d modules recognized", #modules))
end

local function filter(k, patt)
   for _,p in ipairs(patt) do
      if string.find(k, p) then return false end
  end
  return true
end

local function get_methods(cl)
   log:debug(string.format("get_methods: %s %s", yats._classnames[cl], type(cl)))
   if usecache and methods_cache[cl] then
      log:debug(string.format("%s already scanned", yats._classnames[cl]))
      return unpack(methods_cache[cl])
   end
   local t = t or {}
   local w = w or {}
   local mycache = {}
   local delok = nil
   for k,v in pairs(cl) do
      if type(v) == "function" then
	 if filter(k, method_pattern) == true then
	    if k == "delete" then
	       delok = true
	    end
	    if cl[k] then
	       table.insert(t,{key=k, val=v, typ="Lua/C", cname = yats._classnames[cl]})
	       mycache[k] = true
	    end
	    if cl["_"..k] then
	       table.insert(t,{key="_"..k, val=v, typ="Lua/C wrapped", cname = yats._classnames[cl]})
	       mycache["_"..k] = true
	    end
	 end
      end
   end
   if not delok then
      table.insert(w, "missing destructor 'delete()'")
   end
   if cl.superclass then
      local scl = cl:superclass()
      if scl then
	 for k,v in pairs(scl) do
	    if type(v) == "function" then
	       if filter(k, method_pattern) == true then
		  if k == "delete" then
		     delok = true
		  end
		  table.insert(t,{key=k, val=v, typ="C", cname = yats._classnames[scl]})
	       end
	    end
	 end
      end
   end
   methods_cache[cl] = {t,w}
   return t, w
end


local function get_members(cl, what)
   local t = {}
   if usecache and members_cache[what] and members_cache[what][cl] then return members_cache[what][cl] end
   log:debug("get_members: "..(what or "?").." "..(yats._classnames[cl] or "?"))
   if what == "c" then
      if cl['.get'] then
	 -- a wrapped class
	 for k,v in pairs(cl['.get']) do
	    if filter(k, member_pattern) == true then
	       table.insert(t,{key=k})
	    end
	 end
      end
   elseif what == "l" then
      for k,v in pairs(cl) do
	 if filter(k, member_pattern) == true then
	    if type(v) ~= "function" then
	       table.insert(t,{key=k})
	    end
	 end
      end
   end
   sort(t, "members")
   members_cache[what] = {}
   members_cache[what][cl] = t
   return t
end

local cache = nil
local function build_cache()
   cache = {}
   for i,v in pairs(yats) do
      local x = rawget(yats, i)
      if type(x) == "table" or type(x) == "userdata" then
	 cache[tostring(x)] = i
      end
   end
   for _, mod in ipairs(modules) do
      for i,v in pairs(yats[mod]) do
	 local x = rawget(ieeebridge, i)
	 if type(x) == "table" or type(x) == "userdata" then
	    cache[tostring(x)] = i
	 end
      end
   end
end

local function findinyats(val)
   if not cache then
      build_cache()
   end
   return cache[tostring(val)]
end

local bases_cache = {}
local function get_bases(cl)
   log:debug(string.format("get_bases: %s %s", yats._classnames[cl], type(cl)))
   if usecache and bases_cache[cl] then return bases_cache[cl] end
   local t = {}
   -- Try Lua bases first
   if cl.superclass then
      -- This is a Lua class - run through all other superclasses as well.
      local scl, lcl = cl:superclass()
      
      while scl do
	 -- Get lua superclasses name from yats module
	 local name = findinyats(lcl)
	 
	 -- Catch this as valid entry - remind to put it at the
	 -- end of the list.
	 if name then  
	    table.insert(t, {key = name.." (lbase)"}) 
	 end
	 
	 -- Get superclasses name from yats module
	 local name = findinyats(scl)
	 
	 -- Catch this as valid entry - remind to put it at the
	 -- end of the list.
	 if name then  
	    table.insert(t, {key = name}) 
	 end
	 
	 
	 -- Are there any more superclasses
	 if scl.superclass then
	    -- perhaps - try next
	    scl = scl:superclass()
	 else
	    -- no - name is obviously '_<name>' because we 
	    if name then
	       cl = yats[name]
	       assert(cl)
	    end
	    break
	 end
      end
   end

   local mt = getmetatable(cl)
   while mt do
      local name = findinyats(mt)
      if name then 
	 table.insert(t, {key = name}) 
      end
      mt = getmetatable(mt)
   end
   bases_cache[cl] = t
   return t
end

local function get_module(module)
   local y = {
      variables = {n=0},
      numeric_constants = {n=0},
      string_constants = {n=0},
      bool_constants = {n=0},
      functions = {n=0},
      classes = {n=0},
   }
   local cache = {}
   yats._classnames = {}
   for k,v in pairs(module) do
      if type(v) == "table" then
	 yats._classnames[v] = k
      end
   end
   y.variables = get_members(module, "c")
   for k,v in pairs(module) do
      if (filter(k, member_pattern) == true) and (filter(k, modules) == true)  then
	 if type(v) == "number" then
	    table.insert(y.numeric_constants,{key=k,val=v})
	 elseif type(v) == "string" then
	    table.insert(y.string_constants,{key=k,val=v})
	 elseif type(v) =="boolean" then
	    table.insert(y.bool_constants,{key=k,val=v})
	 elseif type(v) == "function" then
	    table.insert(y.functions,{key=k, val=v})
	 elseif type(v) == "table" then
	    local cmembers = get_members(v, "c")
	    local lmembers = get_members(v, "l")
	    local m,w = get_methods(v)
	    local b = get_bases(v)
	    if cache[_tostring(v)] == nil then
	       table.insert(y.classes, {key=k, methods = m, warnings = w,
					cmembers = cmembers, lmembers = lmembers, bases = b
			})
	       cache[_tostring(v)] = true
	    end
	 end
      end
   end
   -- sort everything
   sort(y.numeric_constants, "numbers")
   sort(y.string_constants, "strings")
   sort(y.bool_constants, "booleans")
   sort(y.functions, "functions")
   sort(y.classes, "classes")
   return y
end

function getInfo(self)
   members_cache = {}
   methods_cache = {}
   bases_cache = {}
   local y = get_module(self)
   y.modules = {}
   for k,v in ipairs(modules) do
      table.insert(y.modules, {key=v, module=get_module(yats[v])})
   end
   return y
end

function showInfo(self, what)
   local x = self:getInfo()
   local prefix, pprefix
   if string.find(what, "a") then
      what = "vfnbsc"
   end
   table.insert(x.modules, 1, {key="yats"})
   for j,w in ipairs(x.modules) do
      if w.key == "yats" then
	 y = x
	 prefix = ""
	 pprefix = "yats"
      else
	 y = w.module
	 prefix = "yats."
	 pprefix = "yats."..w.key
      end
      printf("=== MODULE '%s%s' ===\n", prefix, w.key)
      for i = 1, string.len(what) do
	 local c = string.sub(what, i, i)
	 if c == "n"  then
	    printf("\nNUMERIC CONSTANTS (%s): %d\n", pprefix, #y.numeric_constants)
	    for i,v in ipairs(y.numeric_constants) do
	       printf("  %-32s %g\n", v.key, v.val)
	    end
	 elseif c == "s" then
	    printf("\nSTRING CONSTANTS (%s): %d\n", pprefix, #y.string_constants)
	    for i,v in ipairs(y.string_constants) do
	       printf("  %-32s '%s'\n", v.key, tostring(v.val))
	    end
	 elseif c == "b" then
	    printf("\nBOOLEAN CONSTANTS (%s): %d\n", pprefix, #y.bool_constants)
	    for i,v in ipairs(y.bool_constants) do
	       printf("  %-32s  %s\n", v.key, tostring(v.val))
	    end
	 elseif c == "f" then
	    printf("\nFUNCTIONS (%s): %d\n", pprefix, #y.functions)
	    for i,v in ipairs(y.functions) do
	       printf("  %-32s  %s\n", v.key, tostring(v.val))
	    end
	 elseif c == "v" then
	    printf("\nVARIABLES (%s): %d\n", pprefix, #y.variables)
	    for i,v in ipairs(y.variables) do
	       printf("  %-32s\n", v.key)
	    end
	 elseif c == "c" then
	    printf("\nCLASSES (%s): %d\n\n", pprefix, #y.classes)
	    for i,v in ipairs(y.classes) do
	       printf("  CLASS '%s' (%s)\n\n", v.key, pprefix)
	       printf("    Methods: %d\n", #v.methods)
	       for j,m in ipairs(v.methods) do
		  printf("      %-32s %s %s\n", m.key, tostring(m.val), m.typ)
	       end
	       print()
	       printf("    Members (C): %d \n", #v.cmembers)
	       for j,m in ipairs(v.cmembers) do
		  printf("      %-32s\n", m.key)
	       end
	       print()
	       printf("    Members (Lua): %d\n", #v.lmembers)
	       for j,m in ipairs(v.lmembers) do
		  printf("      %-32s\n", m.key)
	       end
	       print()
	       printf("    Base classes: %d\n", #v.bases)
	       for j, m in ipairs(v.bases) do
		 printf("      %s\n", m.key)
	       end
	       print()
	       printf("    Warnings: %d\n", #v.warnings)
	       for j,m in ipairs(v.warnings) do
		  printf("      %s [%s]\n", m, v.key)
	       end
	       print()
	    end
	 end
      end
      printf("\n\n")
   end
end

-- Find anchor of form <classname>#<function> and return 
-- anchorfile plus anchor
local function find_luadoc(files, classname, funcname)
   -- iterate through all file to find the link
   for _, f in ipairs(files) do
      for _, s in ipairs(f.content) do
	 if string.find(s, "#"..classname..":"..funcname) then
	    return f.name, "#"..classname..":"..funcname
	 end
      end
   end
end


-- Create the data base
function getLuadocInfo(binding, docdir)
   local files = {}
   local flist
   -- read all html docs into tables for later search
   for f in lfs.dir(docdir) do
      local fn = docdir.."/"..f
      if lfs.attributes(fn,"mode") == "file" then
	 local t = {}
	 for l in io.lines(fn) do
	    tinsert(t, l)
	 end
	 tinsert(files, {name = fn, content = t})
      end
   end
   -- put anchors in info data base
   local modlist = {}
   for _, w in ipairs(binding.modules) do
      if w.key == "_M" then
	 mname = "yats"
	 m = binding
      else
	 mname = w.key
	 m = w.module
      end
      local classes = {}
      for _, v in ipairs(m.classes) do
	 local methods = {}
	 for _, f in pairs(v.methods) do
	    --		     print("   ", v.key, f.key)
	    local anchorfile, anchor = find_luadoc(files, v.key, f.key)
	    if anchorfile then
	       f.luadoclink = {anchorfile=anchorfile, anchor=anchor}
	       methods[f.key] = f.luadoclink
	    end
	 end
	 classes[v.key] = methods
      end
      modlist[mname] = classes
   end
   return modlist
end

function htmlInfo(self, tagfile, luadoc)
   local ht = {}
   local prefix, pprefix

   -- Read doxygen tags
   local dtags = yats.doxy:new(tagfile)
   assert(#dtags.duplicates == 0)

--   local fout = io.open("yy", "w")
--   fout:write(pretty(dtags.classes).."\n")
--   fout:close()

   -- Get Binding
   local b = self:getInfo()

   -- Construct html page
   -- stylesheet
   tinsert(ht, [[
		 <link media="screen" rel="stylesheet"
		 href="style.css"
		 type="text/css">]])
   tinsert(ht, "<body>")
   -- title
   tinsert(ht, htk.TITLE{"Luayats - Reference Index"})
   tinsert(ht, "\n")
   -- logo
   tinsert(ht,[[
		 <div id = "logo" style="top: 12px; height: 129px; left: 0px; text-align: center; width: 925px;" >
		    <a name="home2" id="home2"></a>
		    <a href="http://www.lua.org">
		    <img style="border: 0px solid ; left: 0px; top: 6px; width: 115px; height: 118px; float: left;"
		 id="lualogo" alt="www.lua.org" src="luayats.png"
		 name="lualogo" hspace="20" /></a></div>
	      ]])
   tinsert(ht, "\n")
   -- header
   tinsert(ht,[[
		 <div id="header">
		    <h1 style="height: 120px; margin-left: 0px; width: 928px;">
		    <big><big><a name="home" id="home"></a><br />Luayats</big></big>
		    <br />Network Simulation&nbsp;with YATS and Lua</h1>
		    </div>
	      ]])
   -- Left Navigation
   tinsert(ht,[[
		 <div id="leftnavigation">
		    <ul> <li style="margin-left: 0px; width: 185px;">
		    <a class="current" href="index.html">Home</a></li>
		    <li><a href="index.html#license">License</a></li>
		    <li><a href="index.html#features">Features</a></li>
		    <li><a href="index.html#download">Download</a></li>
		    <li><a href="index.html#installation">Installation</a></li>
		    <li style="list-style-type: none; list-style-image: none; list-style-position: outside;">
		    <a href="introduction.html#introduction">User Manual</a></li>
		    <li style="list-style-type: none; list-style-image: none; list-style-position: outside;">
		    <a href="refindex.html">API Reference Index</a></li>
		    <li style="list-style-type: none; list-style-image: none; list-style-position: outside;">
		    <a href="progmanual.html">Programmer's Manual</a><ul></ul></li> 
		    <li><a href="index.html#whatsnew">What's New</a></li>
		    <li><a href="index.html#links">Links</a></li> 
		    <li><a href="index.html#todo">ToDo</a></li>
		    <li><a href="index.html#credits">Credits</a></li>
		    </ul></div>
	      ]])
   tinsert(ht, "\n")
   -- Summary of modules
   tinsert(ht, [[<div id="content"]])
   tinsert(ht,[[<div id="modules"]])
   tinsert(ht, htk.H1{"Modules"})
   for _, m in ipairs(b.modules) do
      tinsert(ht, htk.A{href="#"..m.key, "yats."..m.key})
      tinsert(ht, htk.BR{})
   end
   tinsert(ht,[[</div]])
   tinsert(ht, "\n")

   -- Table defaults
   local tabtemplate = function() 
			  return {
			     border = 0, bgcolor = "#e0e0e0",
			     cellpadding = 6, nowrap = true
			  } 
		       end
   local colmaxtemplate = 5

   -- Summary of Classes
   tinsert(ht,[[<div id="classes"]])
   tinsert(ht, htk.H1{"Classes"})
   local m
   for _, w in ipairs(b.modules) do
      if w.key == "_M" then
	 mname = "yats"
	 m = b
      else
	 mname = w.key
	 m = w.module
      end
      local col, colmax = 0, colmaxtemplate
      local tab = tabtemplate()
      tinsert(ht, htk.A{name=w.key, htk.H2{string.format("classes in module: %s", mname)}})
      local tr={}
      for _, v in ipairs(m.classes) do
	 tinsert(tr, htk.TD{htk.A{href="#"..v.key, v.key}})
	 col = col + 1
	 if col == colmax then
	    col = 0
	    tinsert(tab, htk.TR(tr))
	    tr = {}
	 end
      end
      tinsert(ht, htk.TABLE(tab))
      tinsert(ht, "\n")
   end
   tinsert(ht,[[</div]])
   tinsert(ht, "\n")
   -- Details
   tinsert(ht, [[<div id="details"]])
   for _, w in ipairs(b.modules) do
      if w.key == "_M" then
	 mname = "yats"
	 m = b
      else
	 mname = w.key
	 m = w.module
      end
      -- classes
      local docdir = "cpp/html/"
      for _, v in ipairs(m.classes) do
	 tinsert(ht, htk.A{name=v.key, htk.H4{string.format("class: %s.%s", mname, v.key)}})

	 -- methods
	 if #v.methods > 0 then
	    tinsert(ht, htk.H3{"Methods"})
	    local tab, tr = tabtemplate(), {}
	    local col, colmax = 0, colmaxtemplate
	    if #v.methods < colmax then colmax = #v.methods end
	    for _, f in pairs(v.methods) do
	       local href
	       local fkey = f.key
	       local vkey = v.key
	       -- We modify class key for inherited methods (Lua <- C)
	       local cl = dtags.classes[vkey]
	       if not cl then
		  vkey = f.cname
	       end
	       -- We modify function key for de-/constructors
	       if fkey == "delete" then fkey = "~"..vkey end
	       if fkey == "new" or fkey == "new_local" then fkey = vkey end
	       -- look up with modified key 'vkey'
	       cl = dtags.classes[vkey]
	       if cl then
		  -- class found in binding
		  local ref = cl.methods[fkey]
		  if ref then
		     -- method found in binding: o.k.
		     href = docdir..ref.anchorfile.."#"..ref.anchor
		  else
		     -- method not found in binding: look in luadoc
		     local methods = luadoc[mname][v.key]
		     if methods then
			ref = methods[f.key]
			if ref then
			   href = string.sub(ref.anchorfile, 5)..ref.anchor
			else
			   href = "unknown"
			end
		     else
			href = "unknown"
		     end
		  end
	       else
		  -- class not found in binding: look in luadoc database
		  local methods = luadoc[mname][v.key]
		  if methods then
		     ref = methods[f.key]
		     if ref then
			href = string.sub(ref.anchorfile, 5)..ref.anchor
		     else
			href = "unknown"
		     end
		  else
		     href = "unknown"
		  end
	       end
	       local x = f.cname or "unknown"
	       if href == "unknown" then
		  tinsert(tr, htk.TD{string.format("%s", f.key).."<br>["..f.typ.."]".."<br>["..x.."]"})
	       else
--		  tinsert(tr, htk.TD{htk.A{href=href, 
--				string.format("%s", f.key)}.."<br>["..f.typ.."]".."<br>["..x.."]"})
		  tinsert(tr, htk.TD{htk.A{href=href, 
				string.format("%s", f.key)}.."<br>["..f.typ.."]"})
	       end
	       col = col + 1
	       if col == colmax then
		  col = 0
		  tinsert(tab, htk.TR(tr))
		  tr = {}
	       end
	    end
	    tinsert(ht, htk.TABLE(tab))
	    tinsert(ht, "\n")
	 end

	 -- variables
	 if #v.lmembers + #v.cmembers > 0 then
	    tinsert(ht, htk.H3{"Members"})
	    local tab, tr = tabtemplate(), {}
	    local col = 1
	    local col, colmax = 0, colmaxtemplate
	    if #v.lmembers + #v.cmembers < colmax then colmax = #v.lmembers+#v.cmembers end
	    for _, f in ipairs(v.lmembers) do
--	       tinsert(tr, htk.TD{htk.A{href="unkown", f.key}})
	       tinsert(tr, htk.TD{f.key})
	       col = col + 1
	       if col == colmax then
		  col = 0
		  tinsert(tab, htk.TR(tr))
		  tr = {}
	       end
	    end
	    for _, f in ipairs(v.cmembers) do
	       local cl = dtags.classes[v.key]
	       local ref, href
	       
	       if cl then
		  ref = cl.vars[f.key] or cl.enums[f.key] or cl.namedenums[f.key]
		  if ref then
		     href = docdir..ref.anchorfile.."#"..ref.anchor
		  else
		     href = "unkown"
		  end
	       else
		  href = "unknown"
	       end
	       tinsert(tr, htk.TD{htk.A{href=href, f.key}})
	       col = col + 1
	       if col == colmax then
		  col = 0
		  tinsert(tab,htk.TR(tr))
		  tr = {}
	       end
	    end
	    tinsert(ht, htk.TABLE(tab))
	    tinsert(ht, "\n")
	 end
      end
   end
   tinsert(ht,[[</div]])
   tinsert(ht,[[</div]])
   -- footer
   tinsert(ht,[[
		 <div id="footer"><small>(c) 2003-2009 Herbert Leuwer, April 2009&nbsp;&nbsp;&nbsp;
		    <a href="mailto:herbert.leuwer@t-online.de">Contact</a></small></div>
	      ]])
   tinsert(ht, "</body>")
   tinsert(ht, "\n")
   return ht
end


if _TEST == true then
  local y = yats:getInfo()
  local cmd = "x = "..tostring(y)
  local f, err = loadstring(cmd)
  if f then 
    f() 
    print(tostring(y))
    print(pretty(x))
  end
--  print(pretty(y))
end

return yats