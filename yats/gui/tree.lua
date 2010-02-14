------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: tree.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Graphical user interface (GUI)
-- <br>
-- <br><b>module: gui</b><br>
-- <br>
-- This module defines the binding and object browsing functions for Luayats.

require "iuplua"
require "iupluacontrols"
local cfg = require "yats.config"
require "toluaex"

module("gui", package.seeall)

-- We may want to do some printing - just debugging.
local noprintf = function(...) return end
local printf = function(fmt, ...) io.write(string.format(fmt, ...)) end
--+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-- Generic browser helpers
--+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-- Some local variable
local yes,no = "YES", "NO"
local on,off = "ON", "OFF"
local collapsed = "COLLAPSED"
local expanded = "EXPANDED"

-- Helpers to manage IUP trees.
local function getState(tree, id)
   id = id or tree.value
   return iup.GetAttribute(tree, "STATE"..id)
end

-- Expand a node.
local function expand(tree, id)
   id = id or tree.value
   iup.SetAttribute(tree, "STATE"..id, expanded)
end

-- Collapse a node.
local function collapse(tree, id)
   id = id or tree.value
   iup.SetAttribute(tree, "STATE"..id, collapsed)
end

-- Collapse all
local function collapseall(tree)
   tree.value = "ROOT"
   collapse(tree, 0)
   tree.redraw = yes
end

local function expandall(tree)
   local mark = tree.value
   expand(tree, 0)
   tree.dlg.cursor = busy
   iup.Flush()
   tree.value = "LAST"
   local last = tonumber(tree.value)
   for i = 1, tree.count do
     expand(tree, i)
   end
   tree.redraw = yes
   tree.value = mark
   tree.dlg.cursor = arrow
   iup.Flush()
 end

-- Menu callback: open or close a branch.
local function openclose_cb(self)
   local state = getState(self.tree)
   if state == expanded then
      collapse(self.tree)
      self.title = "&Expand"
   else
      expand(self.tree)
      self.title = "&Collapse"
   end
   self.tree.redraw = yes
end


-- Menu callback: open or close all branches.
local function opencloseall_cb(self, cmd)
   local menu = iup.GetParent(self)
   if cmd == "expand" then
      expandall(self.tree)
   else
      collapseall(self.tree)
   end
end


local function treemenu(tree)
   local menu = iup.menu{
      tree = tree,
      iup.item{
	 title = "&Collapse",
	 action = openclose_cb,
      },
      iup.item{
	 title = "&Expand all",
	 active = yes,
	 action = function(self) 
		     opencloseall_cb(self, "expand") 
		  end,
      },
      iup.item{
	 title = "Collapse &all",
	 active = yes,
	 action = function(self) 
		     opencloseall_cb(self, "collapse") 
		  end,
      }
   }
   return menu
end

-- Tree callback: select a node
local function selection(self, id, selstate)
   if self.visible == no then return end
   if selstate == 1 then
      if getState(self, id) == expanded then
	 self.menu[1].title = "&Collapse"
      else
	 self.menu[1].title = "&Expand"
      end
   end
   --  self.value = id
end

-- Tree callback: right click mouse
local function rightclick(self, id)
   self.menu:popup(iup.MOUSEPOS,iup.MOUSEPOS)
   return iup.DEFAULT
end

-- Tree callback: doubleclick leaf
local function executeleaf(self, id)
end

local function renamenode(self, id, name)
end
--+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-- Yats Binding Browser
-- A tree control to show Luayats Bindings.
--+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

-- Revert sort order of a 'key' list.
local function revers(t)
   table.sort(t, function(u,v) 
		    if u.key < v.key then 
		       return true 
		    end
		 end)
   return t
end

-- Build the tree table.
local function addTreeTab(t, id, x)
   t[id] = {branchname = "Classes ("..#x.classes..")"}
   for i,v in ipairs(revers(x.classes)) do
      local u = {branchname = string.format("%s", v.key)}
      u[1] = {branchname = string.format("Methods (%d)", 
					 #v.methods)}
      for j,w in ipairs(v.methods) do
	 u[1][j] = w.key
      end
      u[2] = {branchname = string.format("C Members (%d)", 
					 #v.cmembers)}
      for j,w in ipairs(v.cmembers) do
	 u[2][j] = w.key
      end
      u[3] = {branchname = string.format("Lua Members (%d)", 
					 #v.lmembers)}
      for j,w in ipairs(v.lmembers) do
	 u[3][j] = w.key
      end
      u[4] = {branchname = string.format("Base classes (%d)", 
					 #v.bases)}
      for j,w in ipairs(v.bases) do
	 u[4][j] = w.key
      end
      t[id][i] = u
   end
   t[id+1] = {branchname = "Variables (" .. #x.variables..")"}
   for i,v in ipairs(revers(x.variables)) do
      t[id+1][i] = string.format("%s", v.key)
   end
   t[id+2] = {branchname = "Functions (" .. #x.functions..")"}
   for i,v in ipairs(revers(x.functions)) do
      t[id+2][i] = string.format("%s   %s", v.key, tostring(v.val))
   end
   t[id+3] = {branchname = "Numeric Constants (" .. #x.numeric_constants..")"}
   for i,v in ipairs(revers(x.numeric_constants)) do
      t[id+3][i] = string.format("%s = %g", v.key, v.val)
   end
   t[id+4] = {branchname = "String Constants ("..
      table.getn(x.string_constants)..")"}
   for i,v in ipairs(revers(x.string_constants)) do
      t[id+4][i] = string.format("%s = %s", v.key, tostring(v.val))
   end
   t[id+5] = {branchname = "Boolean Constants ("..
      table.getn(x.bool_constants)..")"}
   for i,v in ipairs(revers(x.bool_constants)) do
      t[id+5][i] = string.format("%s = %s", v.key, tostring(v.val))
   end
end

--
-- Build 
local function buildTreeTab(info)
   local t = {branchname = "MODULES (" .. #info.modules..")"}
   for i,v in ipairs(revers(info.modules)) do
      t[i] = {branchname = v.key}
      addTreeTab(t[i], 1, v.module)
   end
   return t
end

-- Tree callback: open a branch
local function branchopen(self, id)
end

-- Tree callback: close a branch
local function branchclose(self, id)
end

------------------------------------------------------------------------------
-- Construct and show a dialog that shows luayats bindings.
-- @param hide string - "hide" hides the dialog, "show" or nil shows it.
-- @return none.
------------------------------------------------------------------------------
function buildBindings(info)

   local tree = iup.tree{
      scrollbar = yes,
      addexpanded = no,
      branchopen_cb = branchopen,
      branchclose_cb = branchclose,
      selection_cb = selection,
      rightclick_cb = rightclick,
      executeleaf_cb = executeleaf,
      renamenode_cb = renamenode
   }
   tree.menu = treemenu(tree)

   local tab = buildTreeTab(info)

   local dlg = iup.dialog{
      icon = images.app,
      title = "Luayats Bindings",
      size = "THIRDxTHIRD",
      iup.vbox {
	 tree,
	 iup.hbox{
	    iup.fill{},
	    iup.button{
	       title = "&Close",
	       image = images.ok,
	       action = function(self)
			   local dlg = iup.GetDialog(self)
			   dlg:hide()
			end
	    }
	 }
      }
   }
   dlg:map()
   iup.TreeSetValue(tree, tab)
   tree.dlg = dlg
   return dlg
end

return gui