-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2009.
-- @author Herbert Leuwer.
-- @release 4.0 $Id: menu.lua 435 2010-02-13 17:07:19Z leuwer $
-- @description Luayats - Main GUI element - Notebook.
--<br>
--<br><b>module: gui</b><br>
-- <br>
-- This module is a helper for editor.lua handling menus.
-- NOTE:
-- The module exports the main menu as 'gui.menu'.
-----------------------------------------------------------------------------------

require "yats.gui.images"
require "yats.gui.runctrl"
local cfg = require "yats.config"
local conf = cfg.data

module("gui", package.seeall)


notebook = nil
local yes, no, on, off = "YES", "NO", "ON", "OFF"

--
-- Collection of the menu actions for easy usage from multiple places
-- Note, that the real content is implemented as notebook methods.
--
menuactions = {
   file = {
      new = function(self) notebook:append(nil, nil) end,
      open = function(self) notebook:open() end,
      close = function(self) notebook:close() end,
      save = function(self) notebook:save() end,
      saveas = function(self) notebook:saveas() end,
      exit = function(self) notebook:exit() end
   },
   edit = {
      undo = function(self) notebook:getbuf():undo() end,
      redo = function(self) notebook:getbuf():redo() end,
      copy = function(self) notebook:copy() end,
      cut = function(self) notebook:cut() end,
      paste = function(self) notebook:paste() end
   },
   search = {
      find = function(self) notebook:find() end,
      findnext = function(self) notebook:findnext() end,
      findprev = function(self) notebook:findprev() end,
      findincremental = function(self) 
			   if notebook.incfinder and notebook.incfinder.present == true then
			      notebook:findincremental(true) 
			   else
			      notebook:findincremental() 
			   end
			end, 
      replace = function(self) notebook:replace() end,
      replaceall = function(self) notebook:replaceall() end,
      gotoline = function(self) 
		    if notebook.gotoliner and notebook.gotoliner.present == true then
		       notebook:goto(true) 
		    else
		       notebook:goto() 
		    end
		 end
   },
   view = {
      highlight = function(self) notebook:highlightall() end
   },
   simul = {
      showlog = function(self) notebook:showlog() end,
      clearlog = function(self) notebook:clearlog() end,
      check = function(self) notebook:check() end,
      gc = function(self) notebook:gc() end,
      objbrowse = function(self) 
		     local val
		     if notebook.objtree and notebook.objtree.present == true then
			val = nil
		     else
			val = true
		     end
		     notebook:objectbrowser(val) 
		  end,
      trial_1 = function(self) notebook:trial_1() end,
      run = function(self) 
	       log:debug(string.format("simul.run: state=%s cmd=%s",getState(), command.val or "nil"))
	       if getState() == "IDLE" then
		  runSimulation(notebook:getbuf().value) 
	       else
		  command:set("cmd_go")
	       end
	    end,
      reset = function(self) command:set("cmd_reset") end
   },
   buffers = {
      closeall = function(self) notebook:closeall() end,
      saveall = function(self) notebook:saveall() end,
      next = function(self) notebook:next() end,
      prev = function(self) notebook:prev() end,
      fontify = function(self) notebook:highlightall() end
   },
   options = {
      highlight = function(self) cfg.set("gui", "AutoHighlight", self.value) end,
      savelog = function(self) cfg.set("gui", "SaveLogOnExit", self.value) end,
      setfont = function(self) notebook:deffont() end,
      optdialog = function(self) notebook:optdialog() end,
      editopt = function(self) notebook:editopt() end,
      defaultopt = function(self) notebook:defaultopt() end,
      closeopt = function(self) notebook:closeopt(false) end,
      saveopt = function(self) notebook:saveopt(false) end,
   },
   help = {
      content = function(self) notebook:help() end,
      bindings = function(self) notebook:bindings() end,
      info = function(self) notebook:about() end
   }
}

--
-- Menukeys
--
menukeys = {
   [iup.K_cN] = menuactions.file.new,
   [iup.K_cO] = menuactions.file.open,
   [iup.K_cS] = menuactions.file.save,
   [-iup.K_cS] = menuactions.file.saveas,
   [iup.K_cW] = menuactions.file.close,
   [iup.K_cP] = menuactions.file.print,
   [iup.K_cQ] = menuactions.file.exit,
   [iup.K_cZ] = function(self, c)
		   if string.find(iup.GetGlobal("MODKEYSTATE"), "S") then
		      notebook:getbuf():redo()
		   else
		      notebook:getbuf():undo()
		   end
		end,
-- Ctrl+C/X/V are covered by the buffer itself 
--   [iup.K_cC] = menuactions.edit.copy,
--   [iup.K_cX] = menuactions.edit.cut,
--   [iup.K_cV] = menuactions.edit.paste,
   [iup.K_cF] = menuactions.search.find,
   [iup.K_cG] = function(self, c)
		   if string.find(iup.GetGlobal("MODKEYSTATE"), "S") then
		      notebook:findprev()
		   else
		      notebook:findnext()
		   end
		end,
   [iup.K_cH] = function(self, c)
		   notebook:highlightall()
		end,
   [iup.K_cJ] = menuactions.search.gotoline,
   [iup.K_cK] = menuactions.search.findincremental,
   [iup.K_cR] = menuactions.search.replace,
   [iup.K_F5] = menuactions.simul.run,
   [iup.K_F7] = menuactions.simul.objbrowse,
   [iup.K_F8] = menuactions.simul.reset,
   [iup.K_F9] = menuactions.simul.check,
   [iup.K_F6] = menuactions.buffers.next,
   [-iup.K_F6] = menuactions.buffers.prev,
   [iup.K_F1] = menuactions.help.content,
   [iup.K_F3] = menuactions.simul.trial_1
}

--
-- Menuitems - used by menu and toolbar. 
--
function useimg(img)
   if cfg.driver == "Win32" then
      return nil
   else
      return img
   end
end

menuitems = {
   separator = function() return iup.separator{} end,
   file = {
      new = iup.item{title="&New\t\t\tCtrl+N", titleimage = useimg(images.new), action = menuactions.file.new},
      open = iup.item{title="&Open ...\t\tCtrl+O", titleimage = useimg(images.open), action = menuactions.file.open},
      save = iup.item{title="&Save\t\tCtrl+S", titleimage = useimg(images.save), action = menuactions.file.save, active = no},
      saveas = iup.item{title="Save &as ...\tShift+Ctrl+S", titleimage = useimg(images.saveas), action = menuactions.file.saveas, active = yes},
      close = iup.item{title="&Close\t\tCtrl+W", titleimage = useimg(images.close), action = menuactions.file.close, active = yes},
      print = iup.item{title="&Print ...\t\tCtrl+P",  titleimage = useimg(images.print), action = menuactions.file.print, active = no},
      exit = iup.item{title="E&xit\t\t\tCtrl+Q", titleimage = useimg(images.cancel), action = menuactions.file.exit},
   },
   edit = {
      undo = iup.item{title="&Undo\tCtrl+Z", titleimage = useimg(images.undo), action=menuactions.edit.undo, active = no},
      redo = iup.item{title="&Redo\tShift+Ctrl+Z", titleimage = useimg(images.redo), action=menuactions.edit.redo, active = no},
      copy = iup.item{title="&Copy\tCtrl+C", titleimage = useimg(images.copy), action=menuactions.edit.copy, active = no},
      cut = iup.item{title="C&ut\t\tCtrl+X", titleimage = useimg(images.cut), action=menuactions.edit.cut, active = no},
      paste = iup.item{title="&Paste\tCtrl+V", titleimage = useimg(images.paste), action=menuactions.edit.paste, active = yes}
   },
   search = {
      find = iup.item{title="&Find ...\t\t\tCtrl+F", titleimage = useimg(images.find), action = menuactions.search.find},
      findnext = iup.item{title="Find &next\t\t\tCtrl+G", titleimage = nil, action = menuactions.search.findnext},
      findprev = iup.item{title="Find &prev\t\t\tShift+Ctrl+G", titleimage = nil, action = menuactions.search.findprev},
      findincremental = iup.item{title="Find &incremental\tCtrl+K", action = menuactions.search.findincremental, value = off},
      replace = iup.item{title="&Replace ...\t\tCtrl+R", action = menuactions.search.replace},
      gotoline = iup.item{title="&Goto line ...\t\tCtrl+J", titleimage = nil, action = menuactions.search.gotoline, value = off},
   },
   view = {
      highlight = iup.item{title="&Highlight buffer\tCtrl+H", titleimage = nil, action = menuactions.view.highlight, 
	 active = cfg.capabilities().canHighlight},
   },
   simul = {
      run = iup.item{title="&Go\t\t\tF5", titleimage = useimg(images.run), action = menuactions.simul.run, active = no},
      reset = iup.item{title="&Reset\t\tF8", titleimage = useimg(images.reset), action = menuactions.simul.reset, active = no},
      loglevel = iup.item{title = "&Logging Levels ...", action = menuactions.simul.loglevel},
      showlog = iup.item{title="&Show Log", titleimage = nil, action = menuactions.simul.showlog},
      clearlog = iup.item{title="&Clear Log", titleimage = nil, action = menuactions.simul.clearlog},
      check = iup.item{title = "Syntax &check\tF9", titleimage = useimg(images.check), action = menuactions.simul.check, active = no},
      gc = iup.item{title = "&Collect Garbage", titleimage = nil, action = menuactions.simul.gc, active = yes},
      trial_1 = iup.item{title = "Trial\t\tF3", titleimage = nil, action = menuactions.simul.trial_1, active = yes},
      objbrowse = iup.item{title = "&Object Browser\tF7", titleimage = useimg(images.home), action = menuactions.simul.objbrowse}
   },
   buffers = {
      next = iup.item{title = "&Next\t\t\tF6", titleimage = useimg(images.next), action = menuactions.buffers.next},
      prev = iup.item{title = "&Previous\tShift+F6", titleimage = useimg(images.prev), action = menuactions.buffers.prev},
      saveall = iup.item{title = "&Save All", titleimage = useimg(images.saveall), action = menuactions.buffers.saveall},
      closeall = iup.item{title = "&Close All", titleimage = useimg(images.closeall), action = menuactions.buffers.closeall},
   },
   options = {
--      highlight = iup.item{title = "&Auto highlight", titleimage = nil, action = menuactions.options.highlight, autotoggle = on, value = off},
--      savelog = iup.item{title = "&Save Log on exit", titleimage = nil, action = menuactions.options.savelog, autotoggle = on, value = off},
      setfont = iup.item{title = "Select &Font ...", titleimage = nil, action = menuactions.options.setfont},
      editopt = iup.item{title = "&Edit Configuration ...", titleimage = nil, action = menuactions.options.editopt },
      defaultopt = iup.item{title = "&Default Configuration ...", titleimage = nil, action = menuactions.options.defaultopt, active = no},
      closeopt = iup.item{title = "&Close Configuration w/o Save", titleimage = nil, action = menuactions.options.closeopt, active = no},
      saveopt = iup.item{title = "Close and &Save Configuration", titleimage = nil, action = menuactions.options.saveopt, active = no},
      optdialog = iup.item{title = "&Preferences  ...", titleimage = useimg(images.tools), action = menuactions.options.optdialog}
   },
   help = {
      content = iup.item{title = "&Content ...\tF1", titleimage = useimg(images.help), action = menuactions.help.content},
      bindings = iup.item{title = "&Bindings ...", titleimage = useimg(images.home), action = menuactions.help.bindings},
      info = iup.item{title = "&Info ...", titleimage = useimg(images.info), action = menuactions.help.info}
   }
}

--
-- File Menu
--
recentmenu = iup.menu{
   map_cb = function(self) self.mapped = true end
}
filemenu = iup.submenu {
   title = "&File",
   iup.menu {
      menuitems.file.new,
      menuitems.file.open,
      menuitems.file.save,
      menuitems.file.saveas,
      menuitems.file.close,
      menuitems.separator(),
      iup.submenu{
	 title = "&Rescent Files ...",
	 recentmenu,
      },
      menuitems.separator(),
      menuitems.file.print,
      menuitems.separator(),
      menuitems.file.exit
   }
}

for i = 1, #conf.Recent do
   iup.Append(recentmenu, iup.item{
		 title = conf.Recent[i],
		 action = function(self)
			     notebook:open(self.title, false)
			  end
	      })
   if recentmenu.mapped == true then
      iup.Map(recentmenu)
   end
   iup.Refresh(recentmenu)
end
--
-- Edit Menu
--
editmenu = iup.submenu {
   title = "&Edit", 
   iup.menu {
      menuitems.edit.undo,
      menuitems.edit.redo,
      menuitems.separator(),
      menuitems.edit.cut,
      menuitems.edit.copy,
      menuitems.edit.paste,
--      menuitems.separator(),
   }
}

--
-- Search Menu
--
searchmenu = iup.submenu{
   title = "&Search",
   iup.menu{
      menuitems.search.find,
      menuitems.search.findnext,
      menuitems.search.findprev,
      menuitems.search.findincremental,
      menuitems.separator(),
      menuitems.search.replace,
      menuitems.separator(),
      menuitems.search.gotoline,
   }
}
--
-- View Menu
--
viewmenu = iup.submenu{
   title = "&View",
   iup.menu{
      menuitems.view.highlight
   }
}
--
-- Run Menu
--
runmenu = iup.submenu{
   title = "&Simulation",
   iup.menu{
      menuitems.simul.run,
      menuitems.simul.reset,
      menuitems.separator(),
      menuitems.simul.objbrowse,
      menuitems.separator(),
      menuitems.simul.showlog,
      menuitems.simul.clearlog,
      menuitems.separator(),
      menuitems.simul.check,
      menuitems.separator(),
      menuitems.simul.gc,
      menuitems.separator(),
      menuitems.simul.trial_1,
   }
}

--
-- Buffer Menu
-- Note: radio attribute and dynamic menu items do not work correctly
--
buffersmenu = iup.submenu{
   iup.menu{
      menuitems.buffers.prev,
      menuitems.buffers.next,
      menuitems.buffers.saveall,
      menuitems.buffers.closeall,
      map_cb = function(self) self.mapped = true end,
      unmap_cb = function(self) self.mapped = false end,
      open_cb = function(self) 
		   local buf = notebook:getbuf()
		   buf.menuitem.titleimage = useimg(images.run)
		end,
      addsep = function(self)
		  local item = menuitems.separator()
		  iup.Append(self, item)
		  if self.mapped then
		     iup.Map(item)
		  end
		  iup.Refresh(item)
		  self.sep = item
	       end,
      delsep = function(self)
		  local sep = self.sep
		  iup.Detach(sep)
		  iup.Destroy(sep)
		  self.sep = nil
	       end,
      append = function(self, buf, nbuffer) 
		  if nbuffer == 1 then 
		     self:addsep() 
		  end
		  local item = iup.item{
		     title = buf.rawtitle,
		     titleimage = useimg(images.check),
		     buf = buf,
		     action = function(self) 
				 local buf = notebook:getbuf()
				 buf.menuitem.titleimage = nil
				 notebook.value = self.buf
				 self.titleimage = useimg(images.run)
			      end
		  }
		  buf.menuitem = item
		  log:debug(string.format("append menu: %s %s", tostring(item), item.title))
		  local retval = iup.Append(self, item)
		  if self.mapped then
		     iup.Map(item)
		  end
		  iup.Refresh(self)
		  self:select(buf)
	       end,
      remove = function(self, buf, nbuffer)
		  log:debug(string.format("remove menu: %s %s", tostring(buf.menuitem), buf.menuitem.title))
--		  iup.Unmap(buf.menuitem)
		  iup.Detach(buf.menuitem)
		  iup.Refresh(self)
		  if nbuffer == 1 then self:delsep() end
		  self:select(buf)
		  return buf.menuitem
	       end,
      select = function(self, buf, oldbuf)
		  local oldbuf = oldbuf or notebook:getbuf()
		  if not oldbuf then return end
		  oldbuf.menuitem.titleimage = nil
		  buf.menuitem.titleimage = useimg(images.run)
	       end,
      reuse = function(self, buf)
		 buf.menuitem.title = buf.rawtitle
	      end,
   },
   title = "&Buffers",
}

--
-- View Menu
--
optionsmenu = iup.submenu{
   title = "&Options",
   iup.menu{
      menuitems.options.optdialog,
      menuitems.separator(),
      iup.submenu{
	 title = "&Configuration",
	 iup.menu{
	    menuitems.options.editopt,
	    menuitems.options.defaultopt,
	    menuitems.options.closeopt,
	    menuitems.options.saveopt,
	 }
      },
--      menuitems.separator(),
--      menuitems.options.setfont,
--      menuitems.separator(),
--      menuitems.options.highlight,
--      menuitems.options.savelog,
--      highlight = menuitems.options.highlight,
--      savelog = menuitems.options.savelog,
      map_cb = function(self)
--		  self.savelog.value = cfg.get("gui", "SaveLogOnExit", true) or off
--		  self.highlight.value = cfg.get("gui", "AutoHighlight", true) or off
	       end,
   }
}
--
-- Help Menu
--
helpmenu = iup.submenu{
   title = "&Help",
   iup.menu{
      menuitems.help.content,
      menuitems.help.bindings,
      menuitems.help.info
   }
}

local function update(item, val)
   if string.upper(item.active) ~= string.upper(val) then
      item.active = val
      item.bmap.active = val
   end
end
--
-- Main Menu
--
menu = iup.menu{
   filemenu,
   editmenu,
   searchmenu,
   viewmenu,
   runmenu,
   buffersmenu,
   optionsmenu,
   helpmenu,
   file = filemenu[1],
   edit = editmenu[1],
   search = searchmenu[1],
   run = runmenu[1],
   buffers = buffersmenu[1],
   options = optionsmenu[1],
   help = helpmenu[1],
   processkey = function(self, c)
		   local func = menukeys[c]
		   log:debug(string.format("menu.processkey: c=%d %s", c, tostring(func)))
		   if func then
		      func(self, c)
		      return iup.DEFAULT
		   else
		      return iup.CONTINUE
		   end
		end,
   update = function(self)
	       local buf = notebook:getbuf()
	       local nbuf = notebook:getnbuf()
	       local val
	       local menuitems = menuitems

	       if not buf then return end

	       local n = #buf.value
	       if n > 0 then val = yes else val = no end
	       update(menuitems.simul.check, val)
--	       menuitems.simul.check.active = val
--	       menuitems.simul.check.bmap.active = val
		  
	       if buf.changed == true then val = yes else val = no end
	       update(menuitems.file.save, val)
--	       menuitems.file.save.active = val
--	       menuitems.file.save.bmap.active = val

	       if buf.selection then val = yes else val = no end
	       update(menuitems.edit.copy, val)
--	       menuitems.edit.copy.active = val
--	       menuitems.edit.copy.bmap.active = val

	       update(menuitems.edit.cut, val)
--	       menuitems.edit.cut.active = val
--	       menuitems.edit.cut.bmap.active = val

	       if buf.undostack:depth() == 0 then val = no else val = yes end
	       update(menuitems.edit.undo, val)
--	       menuitems.edit.undo.active = val
--	       menuitems.edit.undo.bmap.active = val

	       if buf.redostack:depth() == 0 then val = no else val = yes end
	       update(menuitems.edit.redo, val)
--	       menuitems.edit.redo.active = val
--	       menuitems.edit.redo.bmap.active = val

	       if buf.changed ~= buf.oldchanged then
		  if buf.changed == true then 
		     val = true  
		  else 
		     val = false 
		  end
		  notebook:settitle(buf.rawtitle, val)
		  buf.oldchanged = buf.changed
	       end

	       if notebook.configbuf then val = yes else val = no end
	       menuitems.options.saveopt.active = val
	       menuitems.options.closeopt.active = val
	       menuitems.options.defaultopt.active = val
--	       menu.buffers:rename(notebook:gettitle())
	    end
}

return gui