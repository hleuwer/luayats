-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2009.
-- @author Herbert Leuwer.
-- @release 4.0 $Id: toolbar.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Main GUI element - Notebook.
--<br>
--<br><b>module: gui</b><br>
-- <br>
-- This module is a helper for editor.lua handling the toolbar.
-----------------------------------------------------------------------------------
local cfg = require "yats.config"
require "yats.gui.images"
require "yats.gui.menu"

module("gui", package.seeall)

local on, off, yes, no = "ON", "OFF", "YES", "NO"
--============================================================================
-- Toolbar Defnition
--============================================================================
--
-- Available buttons
--
local buttons = {
   new = {image = images.new, tip = "Create a new file", mmap = menuitems.file.new, active = yes},
   open = {image = images.open, tip = "Open file to edit", mmap = menuitems.file.open, active = yes},
   save = {image = images.save, tip = "Save current buffer", mmap = menuitems.file.save, active = no},
   close = {image = images.close, tip = "Close current buffer", mmap = menuitems.file.close, active = yes},
   print = {image = images.print, tip = "Print current buffer", mmap = menuitems.file.print, 
      active = cfg.capa.canPrint},
   undo = {image = images.undo, tip = "Undo last change", mmap = menuitems.edit.undo, active = no},
   redo = {image = images.redo, tip = "Redo last change", mmap = menuitems.edit.redo, active = no},
   copy = {image = images.copy, tip = "Copy selection to clipboard", mmap = menuitems.edit.copy, active = no},
   cut = {image = images.cut, tip = "Cut selection to clipboard", mmap = menuitems.edit.cut, active = no},
   paste = {image = images.paste, tip = "Paste selection to clipboard", mmap = menuitems.edit.paste, active = yes},
   find = {image = images.find, tip = "Find text in current buffer", mmap = menuitems.search.find, active = yes},
--   replace = {image = images.replace, tip = "Replace text in curent buffer", mmap = menuitems.search.replace, active = yes},
   check = {image = images.check, tip = "Check syntax", mmap = menuitems.simul.check, active = no},
   prev = {image = images.prev, tip = "Previous Buffer", mmap = menuitems.buffers.prev, active = yes},
   next = {image = images.next, tip = "Next Buffer", mmap = menuitems.buffers.next, active = yes},
   run = {image = images.run, tip = "Run or stop the simulation", mmap = menuitems.simul.run, active = no},
   reset = {image = images.reset, tip = "Reset the simulation", mmap = menuitems.simul.reset, active = no},
   pref = {image = images.tools, tip = "Adjust preferences", mmap = menuitems.options.optdialog},
   help = {image = images.help, tip = "Help contents", mmap = menuitems.help.content}
}
--
-- Button selection
--
local buttonlist = {
   "new", "open", "save", "close", "print",
   "undo", "redo",
   "copy", "cut", "paste",
   "find", 
   -- Reactivate once we find a good icon for windows
--   "replace",
   "prev", "next",
   "check", "run", "reset",
   "pref", "help"
}
------------------------------------------------------------------------------
-- Create a toolbar.
-- @param flat Make the toolbar items look flat (no button boundaries).
-- @return Toolbar handle.
------------------------------------------------------------------------------
function toolbar(flat)
   local flat = flat or yes

   local tb = iup.hbox{
      buttons = {},
      expand = yes,
--      expand = horizontal,
      append = function(self, but, name)
		  local newbut = iup.button{
		     image = but.image,
		     tip = but.tip,
		     active = but.active,
		     expand = no,
		     flat = flat,
		     title = "",
		  }
		  if not but.action then
		     if but.mmap then
			newbut.mmap = but.mmap
			newbut.action = but.mmap.action
			newbut.mmap.bmap = newbut
		     end
		  end
		  self.buttons[name] = newbut
		  iup.Append(self, newbut)
		  return newbut
	       end,
      remove = function(self, index)
		  iup.Detach(self[index])
		  iup.Destroy(self[index])
	       end
   }
   for _,v in ipairs(buttonlist) do
      tb:append(buttons[v], v)
   end
   return tb
end

return gui
