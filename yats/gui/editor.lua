-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2009.
-- @author Herbert Leuwer.
-- @release 4.0 $Id: editor.lua 427 2010-01-23 22:52:36Z leuwer $
-- @description Luayats - Main GUI element - Notebook.
--<br>
--<br><b>module: gui</b><br>
-- <br>
-- This module implements a multi-window text editor and provides 
-- GUI elements for editing and simulation.
-----------------------------------------------------------------------------------

local iup = require "iuplua"
require "iupluacontrols"
local logging = require "yats.logging.console"
local logging = require "yats.logging.ring"

require "yats.gui.images"
require "yats.gui.menu"
require "yats.gui.toolbar"
require "yats.gui.options"
require "yats.gui.runctrl"
require "yats.gui.tree"

local cfg = require "yats.config"
local lexer = require "yats.gui.lexer"

module("gui", package.seeall)

local conf = cfg.data

--============================================================================
-- Logging facility
--============================================================================
local function printf(fmt, ...)
   io.stdout:write(string.format(fmt.."\n", ...))
end
log = logging[conf.yats.LogType]("EDT "..conf.yats.LogPattern)
log:setLevel(conf.gui.LogLevel)
log:info(string.format("LOGGER EDT created: %s", os.date()))

--[[
REMINDER:
- keep search and replace pattern when leaving editor for later use.
]]
--============================================================================
-- Configuration constants
--============================================================================
local maxundo = 5

--============================================================================
-- Constants
--============================================================================
local colors = {
   red = "255 0 0",
   dark_red = "128 0 0",
   red2 = "200 0 0",
   green = "0 255 0",
   dark_green = "0 128 0",
   blue = "0 0 255",
   dark_blue = "0 0 128",
   magenta = "255 0 255",
   dark_magenta = "128 0 128",
   cyan = "0 255 255",
   dark_cyan = "0 128 128",
   black = "0 0 0",
   white = "255 255 255",
   grey = "128 128 128",
   dark_grey = "64 64 64",
   nil
}
-- Some IUP constants
local yes, no, on, off = "YES", "NO", "ON", "OFF"
local new, normal, cancelled = "1", "0", "-1"
local horizontal = "HORIZONTAL"
local vertical = "VERTICAL"
local both = "BOTH"
local aleft, aright, acenter = "ALEFT", "ARIGHT", "ACENTER"
local busy, arrow = "BUSY", "ARROW"
local driver = iup.GetGlobal("DRIVER")

-- Forward declarations
local finder, incrementalfinder, replacer = nil, nil, nil, nil
notebook = nil
--============================================================================
-- Helpers
--============================================================================
---
-- Basename of file given by fullpath
---
local function basename(path)
   local s = string.gsub(path, "(.*)[/\\]([^/\\]+)$", "%2")
   return s
end

---
-- Dirname and filename of a file given by fullpath
---
local function splitname(path)
   local dn, fn 
   string.gsub(path, "(.*)[/\\]+([^/\\]+)$", function(p1, p2) dn=p1 fn=p2 end)
   return dn or "./", fn or path
end


local function unix2dos(s)
   return string.gsub(s, "\n", "\r\n")
end

local function dos2unix(s)
   return string.gsub(s, "\r\n", "\n")
end

local function setval(s)
   if driver == "Win32" and cfg.capabilities().formatText == yes then
      return "\n" .. (s or "")
   else
      return s
   end
end
--============================================================================
-- Statusbar Definition
--============================================================================
function statusbar()

   local function gcinfo()
      return tostring("gc: "..math.floor(collectgarbage("count"))).." kB"
   end

   local labelcursor = iup.label{
      title=string.format("(%d,%d)", 1, 1),
      alignment = aleft,
      minsize = "80x"
   }
   local labelclock = iup.label{
      title=gcinfo().. " " .. os.date("%d.%m.%y %R"), 
      alignment = aright
   }
   local progress = iup.progressbar{
      expand = horizontal, min = 0, max = 1, rastersize = "100x18", visible = yes
   }
   local labelmessage = iup.label{
      title = string.rep(" ", 24), expand = horizontal, alignment = aleft
   }
   local labelsimtime = iup.label{
      title = string.rep(" ", 24), expand = horizontal, alignment = aleft
   }
   local zbox = iup.zbox{
      progress,
      labelmessage,
      labelsimtime,
      expand = horizontal,
      progress = progress,
      message = labelmessage,
      simtime = labelsimtime,
      alignment = acenter,
      value = labelmessage,
      
   } 
   zbox.value = labelmessage
   local sb = iup.hbox{
      labelcursor,
      iup.label{separator=vertical, alignment = aleft},
      zbox,
      iup.label{separator=vertical, alignment = aright},
      labelclock,
      zbox = zbox,
      expand = horizontal,
      cursor = labelcursor,
      clock = labelclock,
      progress = progress,
      message = labelmessage,
      simtime = labelsimtime,
      update = function(self)
		  self.cursor.title = cursorinfo()
	       end,
      updateclock = function(self)
		       self.clock.title = gcinfo().." "..os.date("%d.%m.%y %R")
		    end
   }
   return sb
end
--============================================================================
-- Notebook Definition
--============================================================================
notebook = iup.tabs{
   nbuffer = 0,
   expand = yes,
   bufindex = 1,
   getnbuf = function(self) return tonumber(self.nbuffer) end,
   setnbuf = function(self, n) self.nbuffer = n end,
   incnbuf = function(self) self.nbuffer = tonumber(self.nbuffer) + 1 end,
   decnbuf = function(self) self.nbuffer = tonumber(self.nbuffer) - 1 end,
   -- callbacks
   map_cb = function(self) self.mapped = true end,
   tabchange_cb = function(self, new, old)
		     log:debug(string.format("tabchange: %s: %s to %s", tostring(self), tostring(old), tostring(new)))
		     menu.buffers:select(new)
		  end
}

------------------------------------------------------------------------------
-- Return notebook's dialog handle.
-- @param self Notebook handle.
-- @return Dialog handle.
------------------------------------------------------------------------------
function notebook.dialog(self)
   return iup.GetDialog(self)
end

-- 
-- Create an undo or redo stack.
--
local function dostack(buf, name)
   local stack = {
      states = {},
      max = conf.gui.MaxUndo,
      buf = buf,
      name = name,
      S = "IDLE",
      depth = function(self)
		 return #self.states
	      end,
      push = function(self, state)
		log:info(string.format("%s:%s push: state.saved=%s",
				       tostring(self.buf), self.name, tostring(state.saved)))
		table.insert(self.states, 1, state)
		return #self.states
	     end,
      pop = function(self)
	       local buf = self.buf
	       if #self.states == 0 then return nil, "empty" end
	       local state = table.remove(self.states, 1)
	       log:info(string.format("%s:%s pop: state.saved=%s",
				      tostring(self.buf), self.name, tostring(state.saved)))
	       return state, #self.states
	    end,
      flush = function(self)
		 self.states = {}
	      end
   }
   return stack
end

------------------------------------------------------------------------------
-- Create a text buffer.
-- @param self Notebook handle.
-- @param content New content.
-- @return Reference to the buffer.
------------------------------------------------------------------------------
function notebook.buffer(this, content)
   local fmt = yes
   local buf = iup.text{
      multiline=yes,
      value = setval(content),
      formatting = formatText,
      font = conf.gui.EditorFont,
      expand = yes,
      changed = false,
      saved = false,
      edited = false,
      applying = false,
      highlighted = false,
      padding = "5x",
      getfocus_cb = function(self)
		       this:findincremental(true)
		       this:goto(true)
		       self.focus = true
		    end,
      killfocus_cb = function(self)
			self.focus = false
		     end,
      k_any = function(self, c)
		 print("buf.k_any:", c)
		 if iup.isCtrlXkey(c) or c >= iup.K_F1 then
		    print("continue")
		    return iup.CONTINUE
		 else
		    print("default")
		    return iup.DEFAULT
		 end
	      end,
      caret_cb = function(self, lin, col, pos)
		    if not self.hlock then
		       notebook:dialog().statusbar.cursor.title = string.format("(%d,%d)", lin, col)
		    end
		 end,
      valuechanged_cb = function(self,...)
			   if self.command then
			      self.command = nil
			   else
			      notebook:mark(true)
			   end
			end,
      map_cb = function(self)
--		  local state = self:catch()
--		  self.undostack:push(state)
	       end,
      action = function(self, c, newval, ...)
		  if not self.applying then
		     if not self.focus then
			self.undostack:push(self:catch())
		     else
			if self.undostack:depth() == 0 then
			   self.undostack:push(self:catch())
			   menuitems.edit.undo.active = on
			   menuitems.edit.undo.bmap.active = on
			end
		     end
		  end
	       end,
      button_cb = function(self, but, pressed, x, y, status)
		     if but == iup.BUTTON1 then
			   if pressed == 1 then
			      self.oldcaret = self.caret
			   elseif pressed == 0 then
			      if self.caret ~= self.oldcaret then
				 -- Why would we need this? Delete if not!
--				 local state = self:catch()
--				 self.undostack:push(state)
			      end
			   end
			end
		     end,
      --------------------------------------------------------------
      -- Destroy a buffer.
      -- @param self Buffer handle.
      -- @return none.
      destroy = function(self)
		   if self.menuitem then
		      iup.Destroy(self.menuitem)
		   end
		   iup.Destroy(self)
		end,
      --------------------------------------------------------------
      -- Catch the state of a buffer (for undo/redo).
      -- The state can be used to restore a buffer.
      -- @param self Buffer handle.
      -- @return Catched state.
      catch = function(self)
		 local state = {
		    caret = self.caret,
		    selection = self.selection,
		    content = self.value,
		    changed = self.changed,
		    saved = self.saved,
		    highlighted = self.highlighted
		 }
		 log:info(string.format("%s: catch state.saved=%s len=%d %d", 
					tostring(self), tostring(state.saved), string.len(state.content), string.len(self.value)))
		 return state
	      end,
      --------------------------------------------------------------
      -- Apply state to a buffer.
      -- @param self Buffer handle.
      -- @param state State catched via buf:catch().
      -- @return none.
      apply = function(self, state)
		 log:info(string.format("%s: apply state.saved=%s len=%d", 
					tostring(self), tostring(state.saved), string.len(state.content)))
		 self.applying = true
		 self.value = setval(state.content)
		 self.selection = state.selection
		 self.changed = state.changed
		 self.saved = state.changed
		 if state.highlighted then
		    notebook:highlightall()
		 end
		 self.caret = state.caret
		 self.applying = false
	      end,
      --------------------------------------------------------------
      -- Write buffer content to file.
      -- If no filename is given the buffer path property is used.
      -- The path property is set.
      -- @param self Buffer handle.
      -- @param fname Filename (full path).
      -- @return none.
      write = function(self, fname)
		 local fname = fname or self.path
		 local fd = io.open(fname, "w+")
		 local rv, err
		 rv, err = fd:write(self.value)
		 fd:close()
		 if not rv then 
		    error(string.format("Cannot write buffer to file %q", fname))
		    return nil, err
		 end
		 self.path = fname
		 self.saved = true
		 return true
	      end,
      --------------------------------------------------------------
      -- Read file into buffer. 
      -- If no filename is given the buffer path property is used.
      -- @param self Buffer handle.
      -- @param fname Filename.
      read = function(self, fname)
		local fname = fname or self.path
		local fd = io.open(filename, "r")
		local s, err = fd:read("*a")
		fd:close()
		if not s then 
		   error(string.format("Cannot read from file %q", fname))
		   return nil, err 
		end
		return s
	     end,
      --------------------------------------------------------------
      -- Retrieve cursor position in buffer.
      -- @param self Buffer handle.
      -- @return Cursor position (characters).
      getcursor = function(self)
		     return tonumber(self.caretpos)
		  end,
      --------------------------------------------------------------
      -- Set the cursor position in a buffer.
      -- @param self Buffer handle
      -- @param pos New cursor position (characters).
      -- @return none.
      setcursor = function(self, pos)
		     self.caretpos = pos
		  end,
      --------------------------------------------------------------
      -- Get the cursor position in a buffer (row and col).
      -- @param self Buffer handle.
      -- @return Cursor position row, col.
      getcaret = function(self)
		    local pos = self.caret
		    local row, col
		    string.gsub(pos, "(%d+),(%d+)$", function(x,y)
						       row, col = tonumber(x), tonumber(y)
						    end)
		    return row, col
		 end,
      --------------------------------------------------------------
      -- Set the cursor position in a buffer (row and col).
      -- @param self Buffer handle.
      -- @param row Row of new cursor position.
      -- @param col Column of new cursor position.
      -- @return none.
      setcaret = function(self, row, col)
		    row, col = tonumber(row), tonumber(col)
		    self.caret = string.format("%d,%d", row, col)
		 end,

      --------------------------------------------------------------
      -- Scroll to the given position.
      -- @param self Buffer handle.
      -- @param row Row to scroll to.
      -- @param col Column to scroll to.
      -- @return none.
      scroll = function(self, row, col)
		  row, col = tonumber(row), tonumber(col)
		  local pos = iup.TextConvertLinColToPos(self, row, col)
		  print("pos=", pos)
		  self.caretpos = pos
		  self.scrolltopos = pos
	       end,
      --------------------------------------------------------------
      -- Retrieve coordinates of selection in a buffer.
      -- @param self Buffer handle.
      -- @return Start and End of selection (characters).
      getselection = function(self)
			local s = self.selectionpos
			if s then
			   local a, b
			   string.gsub(s, "^(%d+):(%d*)", function(u,v)
							     a = tonumber(u)
							     b = tonumber(v)
							  end)
			   return a, b
			end
			return 1, 1
		     end,
      --------------------------------------------------------------
      -- Select text in a buffer.
      -- @param self Buffer handle.
      -- @param posbeg Start of selection (characters).
      -- @param posend End of selection (characters).
      -- @return none.
      setselection = function(self, posbeg, posend)
			self.selectionpos = string.format("%d:%d", tonumber(posbeg), tonumber(posend))
		     end,
      
      transpos = function(self, backwards, len, posbeg, posend)
		    if backwards == on then
		       local a, b
		       if posbeg then a = len - posbeg end
		       if posend then b = len - posend end
		       return a, b
		    else
		       return posbeg, posend
		    end
		 end,
      --------------------------------------------------------------
      -- Search for a text pattern in a buffer.
      -- Lua pattern matching rules apply.
      -- @param self Buffer handle.
      -- @param pattern Search pattern (Lua style)
      -- @param word Search whole word ('on' or 'off').
      -- @param case Search case sensitive ('on' or 'off').
      -- @param backwards Search backwards ('on' or 'off').
      -- @param wrap Wrap to start/end when end/start is reached.
      -- @param plain Do a plain search w/o pattern matching.
      -- @return true if pattern found, false otherwise.
      search = function(self, pattern, word, case, backwards, wrap, plain)
		  local len = string.len(self.value)
		  local searchtext, cpattern, spattern = "", "", ""
		  if case == on then
		     searchtext = self.value
		     cpattern = pattern
		  else
		     searchtext = string.lower(self.value)
		     cpattern = string.lower(pattern)
		  end
		  if backwards == on then
		     searchtext = string.reverse(searchtext)
		     cpattern = string.reverse(cpattern)
		  end
		  if word == on then
		     spattern = "[^%w%d_]" .. cpattern .. "[^%w%d_]"
--		     spattern = "[%s%p]"..cpattern.."[%s%p]"
		  else
		     spattern = cpattern
		  end
		  local nstart = tonumber(self.nextstart)
		  local start = self:transpos(backwards, len, nstart or self:getcursor())
		  local posbeg, posend = nil, nil
		  log:debug(string.format("start search: back=%s wrap=%s word=%s spattern=%q start=%d nextstart=%s", 
					  backwards, wrap, word, spattern, start, tostring(self.nextstart)))
		  posbeg, posend = string.find(searchtext, spattern, start, plain)
		  log:debug(string.format("   result path 1: beg=%s end=%s", 
					  tostring(posbeg), tostring(posend)))
		  if posbeg == nil then
		     if wrap == on then
			posbeg, posend = string.find(string.sub(searchtext, 1, start - 1), spattern, 1)
			log:debug(string.format("   result path 2: beg=%s end=%s",
						tostring(posbeg), tostring(posend)))
			if not posbeg then
			   self.nextstart = nil
			   return false, posbeg
			end
		     else
			self:setcursor(self:transpos(backwards, len, len))
			self.nextstart = nil
			return false, posbeg
		     end
		  end
		  retval = posbeg
		  if backwards == on then
		     posbeg, posend = self:transpos(backwards, len, posend, posbeg)
		  end
		  if word == on then
		     -- note: in Win32 setselection also sets caret => do not scroll
		     if backwards == on then
			self:setselection(posbeg+1, posend)
			if driver == "GTK" then
			   self.scrolltopos = posbeg
			end
			self.nextstart = posend - 2
		     else
			self:setselection(posbeg, posend - 1)
			if driver == "GTK" then
			   self.scrolltopos = posbeg
			end
			self.nextstart = posbeg + 2
		     end
		  else
		     if backwards == on then
			self:setselection(posbeg, posend + 1)
			if driver == "GTK" then
			   self.scrolltopos = posbeg
			end
			self.nextstart = posend -1
		     else
			self:setselection(posbeg - 1, posend)
			if driver == "GTK" then
			   self.scrolltopos = posbeg - 1
			end
			self.nextstart = posbeg + 1  
		     end
		  end
		  return true, retval
	       end,

      --------------------------------------------------------------
      -- Undo an action in a buffer.
      -- @param self Buffer handle.
      -- @return none.
      undo = function(self)
		local newstate = self:catch()
		local oldstate = self.undostack:pop()
		if oldstate then
		   self:apply(oldstate)
		end
		self.redostack:push(newstate)
	     end,
      --------------------------------------------------------------
      -- Redo a previously undone action in a buffer.
      -- @param self Buffer handle.
      -- @return none.
      redo = function(self)
		local newstate = self:catch()
		local oldstate = self.redostack:pop()
		if oldstate then
		   self:apply(oldstate)
		end
		self.undostack:push(newstate)
	     end,
      --------------------------------------------------------------
      -- Perform a syntax highlighting step in a buffer.
      -- The given lexer token function is used to find a token
      -- in the buffer and highlight it according highlighting rules.
      -- @param self Buffer handle.
      -- @param tok Lexer token function.
      -- @return Found token ('keyword', 'comment', 'string', 'iden') and
      --         end position of the token.
      highlight = function(self, tok)
			 local funcname, fmtiden, funcdef = self.funcname, self.fmtiden, self.funcdef
			 what, content, b, e = tok()
			 if not what then return nil end
			 local fmt = iup.user{}
			 fmt.selectionpos = string.format("%d:%d", b-1, e)
			 if what == "keyword" then
			    if content == "function" then
			       if funcname and funcdef then
				  -- kind: name = function(...)
				  color = colors.blue
				  local fmt = iup.user{selectionpos=funcname, fgcolor = colors.blue}
				  self.addformattag = fmt
				  funcname, funcdef = nil, nil
			       else
				  -- kind: function name(...)
				  fmtiden = true
			       end
			    else
			       funcname = nil
			    end
			    fmt.fgcolor = colors.dark_magenta
			 elseif what == "comment" then
			    fmt.fgcolor = colors.red2
			 elseif what == "string" then
			    fmt.fgcolor = colors.dark_cyan
			 elseif what == "iden" then
			    -- remind, because this might be a function name
			    funcname = string.format("%d:%d", b-1, e)
			    if fmtiden then
			       fmt.fgcolor = colors.blue
			    end
			 else
			    if funcname and content == "=" then
			       funcdef = true
			    else
			       funcdef = false
			    end
			    if fmtiden then
			       fmt.fgcolor = colors.blue
			       if content == "(" then
				  fmt.fgcolor = colors.black
				  fmtiden = nil
			       end
			    end
			 end
			 self.command = true
			 self.addformattag = fmt
			 self.funcname, self.fmtiden, self.funcdef = funcname, fmtiden, funcdef
			 return what, e
		      end
   }
   buf.undostack = dostack(buf, "UNDO")
   buf.redostack = dostack(buf, "REDO")
   log:debug(string.format("buffer created: %s", tostring(buf)))
   return buf
end

------------------------------------------------------------------------------
-- Warning dialog.
-- @param self Notebook handle.
-- @param what Type of warning: 'close', 'find'.
-- @return 'yes', 'no', 'cancel' depending on buttons.
------------------------------------------------------------------------------
function notebook.warn(self, what)
   if what == "close" then
      local retval = iup.Alarm("WARNING", 
			       "File not saved! Save now?", 
			       "Yes", "No", "Cancel")
      if retval == 1 then 
	 return "yes"
      elseif retval == 2 then 
	 return "no"
      else 
	 return "cancel" 
      end
   elseif what == "find" then
      local retval = iup.Message("INFO", "Searched until end of buffer. Text not found")
      return "yes"
   end
end

------------------------------------------------------------------------------
-- Create and append a new buffer
-- @param self Notebook handle.
-- @param title Title of the buffer's tab.
-- @param content Content string.
-- @param tip Tip to show when mouse enters the buffer canvas.
-- @param protected Buffer is protected (true) not (nil, false).
-- @return Reference to the buffer.
------------------------------------------------------------------------------
function notebook.append(self, title, content, tip, protected)
   local content = content or ""
   local pos = self:getnbuf()
   self:incnbuf()
   local title = title or "Untitled-"..self.bufindex
   self.bufindex = self.bufindex + 1
   local buf = self:buffer(content)
   buf.rawtitle = title
   buf.tip = tip
   buf.protected = protected
   log:debug(string.format("notebook.append: buffer appended %s %q", tostring(buf), title))
   iup.Append(self, buf)
   if self.mapped then
      iup.Map(buf)
   end
   iup.Refresh(buf)
   menu.buffers:append(buf, self:getnbuf(), first)
   self:select(buf)
   self:settitle(title)
   return buf
end

------------------------------------------------------------------------------
-- Remove current buffer.
-- @param self Notebook handle.
-- The buffer is not destroyed.
-- @return Handle of removed buffer.
------------------------------------------------------------------------------
function notebook.remove(self)
   local buf = self:getbuf()
   local menuitem = menu.buffers:remove(buf, self:getnbuf())
   self:decnbuf()
   log:debug(string.format("notebook.remove: buffer remove %s %q", tostring(buf), buf.rawtitle))
   iup.Unmap(buf)
   iup.Detach(buf)
   self:select(self:getbuf())
   if buf == self.logbuf then self.logbuf = nil end
   if buf == self.configbuf then self.configbuf = nil end
   return buf
end

------------------------------------------------------------------------------
-- Remove given buffer.
-- @param self Notebook handle.
-- The buffer is not destroyed.
-- @return Handle of removed buffer.
------------------------------------------------------------------------------
function notebook.removebuf(self, buf)
   local menuitem = menu.buffers:remove(buf, self:getnbuf())
   self:decnbuf()
   log:debug(string.format("notebook.remove: buffer remove %s %q", tostring(buf), buf.rawtitle))
   iup.Unmap(buf)
   iup.Detach(buf)
   self:select(self:getbuf())
   if buf == self.logbuf then self.logbuf = nil end
   if buf == self.configbuf then self.configbuf = nil end
   return buf
end

------------------------------------------------------------------------------
-- Reuse current buffer.
-- @param self Notebook handle.
-- @param title Title for this buffer.
-- @param content Content string.
-- @param tip Tip to show when the mouse enters the buffer's canvas.
-- @return Reused buffer.
------------------------------------------------------------------------------
function notebook.reuse(self, title, content, tip)
   local buf = self:getbuf()
   buf.rawtitle = title
   buf.tip = tip
   buf.value = setval(content)
   buf.changed = false
   buf.saved = false
   buf.tok = nil
   buf.undostack:flush()
   buf.redostack:flush()
   menu.buffers:reuse(buf)
   self:settitle(title)
   return buf
end

------------------------------------------------------------------------------
-- Rename the current buffer.
-- @param self Notebook handle.
-- @param title New title.
-- @return none.
------------------------------------------------------------------------------
function notebook.rename(self, title)
   local buf = self:getbuf()
   buf.rawtitle = title
   buf.menuitem.title = title
end

------------------------------------------------------------------------------
-- Retrieve handle of current buffer.
-- @param self Notebook handle.
-- @return Handle of buffer.
------------------------------------------------------------------------------
function notebook.getbuf(self) 
   return self.value_handle, self.valuepos
end

------------------------------------------------------------------------------
-- Retrieve title of current buffer.
-- @param self Notebook handle.
-- @return Title of current buffer.
------------------------------------------------------------------------------
function notebook.gettitle(self)
   return self["tabtitle"..self.valuepos] 
end

------------------------------------------------------------------------------
-- Set title of current buffer.
-- @param self Notebook handle.
-- @param s New Title for the buffer.
-- @param marked Set the change mark '*' in the title.
-- @return none.
------------------------------------------------------------------------------
function notebook.settitle(self, s, marked)
   local buf = self:getbuf()
   buf.rawtitle = s
   if marked == true then
      s = s .. "*"
   end
   self["tabtitle"..self.valuepos] = s
   buf.menuitem.title = s
end

------------------------------------------------------------------------------
-- Mark current buffer changed/unchanged.
-- @param self Notebook handle.
-- @param value Change flag (true/false).
-- @return none.
------------------------------------------------------------------------------
function notebook.mark(self, value)
   local buf = self:getbuf()
   buf.changed = value
end

------------------------------------------------------------------------------
-- Select a buffer.
-- Selects a specific buffer as active buffer.
-- @param self Notebook handle.
-- @param buf Handle to new active buffer.
-- @return none.
------------------------------------------------------------------------------
function notebook.select(self, buf)
   self.value = buf
   menu.buffers:select(buf)
end

------------------------------------------------------------------------------
-- Move to next buffer.
-- @param self Notebook handle.
-- @return Next buffer handle.
------------------------------------------------------------------------------
function notebook.next(self) 
   local oldbuf = self:getbuf()
   self.valuepos = tonumber(self.valuepos) + 1
   menu.buffers:select(self:getbuf(), oldbuf)
   return self:getbuf()
end

------------------------------------------------------------------------------
-- Move to previous buffer.
-- @param self Notebook handle.
-- @return Previous buffer handle.
------------------------------------------------------------------------------
function notebook.prev(self) 
   local oldbuf = self:getbuf()
   self.valuepos = tonumber(self.valuepos) - 1  
   menu.buffers:select(self:getbuf(), oldbuf)
   return self:getbuf()
end

------------------------------------------------------------------------------
-- Open a file using file dialog into a buffer.
-- @param self Notebook handle.
-- @param filename Filename.
-- @param addrescent Flags (boolean) to add file to recent list.
-- @return Buffer content.
------------------------------------------------------------------------------
function notebook.open(self, filename, addrecent)
   local fname
   if not filename then
      local filedlg = iup.filedlg{
	 dialogtype = "OPEN", 
	 title = "Open File", 
	 filter = "*.lua",
	 filterinfo = "Lua Files", 
	 extfilter = "Lua Files|*.lua|Text Files|*.txt|All Files|*;*.*|",
	 allownew = no
      }
      filedlg:popup(iup.CENTER, iup.CENTER)
      local status = filedlg.status
      fname = filedlg.value
      if status == "1" then
	 -- new file
	 local fd = io.open(fname, "w")
	 fd:close()
      elseif status == "-1" then
	 return nil, "cancelled"
      end
   else
      fname = filename
   end
   local fd, err = io.open(fname, "r") -- or io.open(fname, "r+")
   if not fd then return nil, error end
   s, err = fd:read("*a")
   fd:close()
   if not s then return nil, err end
   local buf = self:getbuf()
   buf.path = fname
   local dn, fn = splitname(fname)
   log:debug(string.format("notebook.open: open file %q in directory %q", fn, dn))
   if buf.saved or buf.changed then
      log:debug(string.format("notebook.open: appending buffer %s %q", tostring(buf), fname))
      buf = self:append(fn, s, fname)
   else
      log:debug(string.format("notebook.open: reusing current buffer %s %q", tostring(buf), fname))
      buf = self:reuse(fn, s, fname)
   end
   self:select(buf)
   buf.saved = true
   iup.Flush()
   if conf.gui.AutoHighlight == on then
      notebook:highlightall()
   end
   buf:setcursor(1)
   enableSimControls(on)
   if not addrecent or addrecent == true then
      self:addrecent(fname)
   end
   return s
end

------------------------------------------------------------------------------
-- Add file to notebook's recent file list.
-- @param self Notebook handle.
-- @param fname Filename.
-- @return none.
------------------------------------------------------------------------------
function notebook.addrecent(self, fname)
   local fname = string.gsub(fname, "\\","/")
   for i = 1, #conf.Recent do
      if fname == conf.Recent[i] then return end
   end
   table.insert(conf.Recent, fname)
   if #conf.Recent > conf.gui.MaxRecent then
      table.remove(conf.Recent, 1)
   end
end

------------------------------------------------------------------------------
-- Closes (and saves if necessary) the  current buffer.
-- @param Notebook handle.
-- @return true on success, nil + error on failure.
------------------------------------------------------------------------------
function notebook.close(self)
   local buf, pos = self:getbuf()
   if not buf then return true end
   if buf.protected == true then return nil, "protected" end
   if buf.changed == true then
      local dosave = self:warn("close")
      if dosave == "yes" then 
	 self:save()
	 log:debug(string.format("notebook.close: saved buffer %s", tostring(buf)))
      elseif dosave == "cancel" then 
	 log:debug("notebook.close: saveing cancelled")
	 return nil, "cancelled"
      end
   end
   if self:getnbuf() > 1 then
      log:debug(string.format("notebook.close: buffer remove %s", tostring(buf)))
      -- remove buffer if not the last one
      if tonumber(pos)  == self:getnbuf() - 1 then
	 local delbuf, delpos = buf, pos
--	 self:prev()
	 local buf = self:removebuf(delbuf)
	 buf:destroy()
      else
	 local buf = self:remove()
	 buf:destroy()
      end
   else
      -- reuse buffer if last one
      log:debug(string.format("notebook.close: buffer reuse %s", tostring(buf)))
      self.bufindex = 2
      self:reuse("Untitled-1")
   end
   log:debug("notebook.close: done")
   return true
end

------------------------------------------------------------------------------
-- Close and save all buffers.
-- @param self Notebook handle.
-- @return true on success, nil + error on failure.
------------------------------------------------------------------------------
function notebook.closeall(self)
   self.valuepos = 0
   while self:getnbuf() > 1 do
      if not self:close() then
	 log:debug("notebook.closeall: operation cancelled")
	 return nil, "cancelled"
      end
   end
   if not self:close() then
      log:debug("notebook.closeall: operation cancelled")
      return nil, "cancelled"
   else
      log:debug("notebook.closeall: done")
      return true
   end
end

------------------------------------------------------------------------------
-- Save the current buffer.
-- Asks for a filename if not already saved.
-- @param self Notebook handle.
-- @return true on success, nil + error on failure.
------------------------------------------------------------------------------
function notebook.save(self)
   local buf = self:getbuf()
   if buf.protected then return nil, "protected" end
   if string.find(self:gettitle(),"Untitled") then
      local rv, err = self:saveas()
      if not rv then return rv, err end
   else
      local rv, err = buf:write()
      if not rv then return nil, err end
      self:mark(false)
   end
   return true
end

------------------------------------------------------------------------------
-- Save current buffer unconditionally in file.
-- Asks for a filename.
-- @param self Notebook handle.
-- @return true on success, nil + error on failure.
------------------------------------------------------------------------------
function notebook.saveas(self)
   local buf = self:getbuf()
   local filedlg=iup.filedlg{
      dialogtype="SAVE", 
      title="Save ["..buf.rawtitle.."]", 
      filter="*.lua", 
      filterinfo="Lua files",
      extfilter = "Lua Files|*.lua|Text Files|*.txt|All Files|*;*.*|",
      allownew=yes
   }
   filedlg:popup(iup.CENTER, iup.CENTER)
   local status=filedlg.status
   local fname = filedlg.value
   if status == cancelled then
      return true
   end
   local dn, fn = splitname(fname)
   local rv, err = buf:write(fname)
   if not rv then return nil, error end
   if not buf.protected then
      self:rename(fn)
      self:settitle(buf.rawtitle)
      buf.tip = fname
      self:mark(false)
   end
   return true
end

------------------------------------------------------------------------------
-- Save all unsaved buffers.
-- @param self Notebook handle.
-- @return true on success, nil + error on failure.
------------------------------------------------------------------------------
function notebook.saveall(self)
   local buf = iup.GetNextChild(self, nil)
   while buf do
      self.value = buf
      if not self:save() then
	 return nil, "cancelled"
      end
      menu:update()
      buf = iup.GetNextChild(self, buf)
   end
   return true
end

------------------------------------------------------------------------------
-- Copy selection in current buffer to clipboard.
-- @param self Notebook handle.
-- @return selected text.
------------------------------------------------------------------------------
function notebook.copy(self)
   local buf = self:getbuf()
   self.value_handle.clipboard = "COPY"
   return buf.selected_text
end

------------------------------------------------------------------------------
-- Cut selection in current buffer to clipboard.
-- @param self Notebook handle.
-- @return selected text.
------------------------------------------------------------------------------
function notebook.cut(self)
   local buf = self:getbuf()
   buf:action(iup.K_cC, nil)
   buf.clipboard = "CUT"
   self:mark(true)
   return self:getbuf().selected_text
end

------------------------------------------------------------------------------
-- Paste clipboard content into current buffer at cursor.
-- @param self Notebook handle.
-- @return selected text.
------------------------------------------------------------------------------
function notebook.paste(self)
   local buf = self:getbuf()
   buf:action(iup.K_cV, nil)
   buf.clipboard = "PASTE"
   self:mark(true)
   return self:getbuf().selected_text
end

------------------------------------------------------------------------------
-- Find text using find dialog.
-- Found text is selected.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.find(self)
   if not self.fdlg then
      self.fdlg = finder()
   end
   self.fdlg:show(iup.CENTERPARENT, iup.CENTERPARENT)
end

------------------------------------------------------------------------------
-- Find iternator - next match.
-- Found text is selected.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.findnext(self)
   local buf = self:getbuf()
   local dlg = self.fdlg
   local pattern = dlg:getchild("searchlist").value
   local word = dlg:getchild("toggleword").value
   local case = dlg:getchild("toggleword").value
   local wrap = dlg:getchild("togglewrap").value
   local found = buf:search(pattern, word, case, off, wrap)
   if not found == true then
      notebook:warn("find")
   end
end

function notebook.findprev(self)
   local buf = self:getbuf()
   local dlg = self.fdlg
   local pattern = dlg:getchild("searchlist").value
   local word = dlg:getchild("toggleword").value
   local case = dlg:getchild("toggleword").value
   local wrap = dlg:getchild("togglewrap").value
   local found = buf:search(pattern, word, case, on, wrap)
   if not found == true then
      notebook:warn("find")
   end
end

------------------------------------------------------------------------------
-- Incremental find in current buffer.
-- Found text is selected.
-- Closes dialog element if forceoff is set.
-- @param self Notebook handle.
-- @param forceoff End incremental find.
-- @return none.
------------------------------------------------------------------------------
function notebook.findincremental(self, forceoff)
   local dlg = self:dialog()
   local buf = self:getbuf()
   log:debug(string.format("notebook.findincremental: cursor=%s", buf:getcursor()))
   buf.startincremental = buf:getcursor()
   if not self.incfinder then
      self.incfinder = incrementalfinder()
   end
   if not forceoff and not self.incfinder.present then
--      iup.Insert(dlg.workarea, dlg.statusbar, self.incfinder)
      iup.Append(dlg.workarea, self.incfinder)
      self.incfinder[1].title = "Find:"
      if self.mapped then
	 iup.Map(self.incfinder)
      end
      iup.Refresh(self.incfinder)
      self.incfinder.present = true
      menuitems.search.findincremental.value = on
   else
      iup.Detach(self.incfinder)
      iup.Refresh(self:dialog())
      self.incfinder.present = false
      menuitems.search.findincremental.value = off
   end
end

------------------------------------------------------------------------------
-- Replace text using replace dialog.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.replace(self)
   if not self.rdlg then
      self.rdlg = replacer()
   end
   self.rdlg:show(iup.CENTERPARENT, iup.CENTERPARENT)
end


------------------------------------------------------------------------------
-- Opens the configuration settings dialog.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.optdialog(self)
   local dlg = options()
   dlg:show("HALFxHALF")
end

function notebook.trial_1(self)
   iup.Message("Information", "Reserved for tests.")
end

------------------------------------------------------------------------------
-- Shows logging in new buffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.showlog(self)
   local buf = self.logbuf or self:append("Log Output", nil)
   self.logbuf = buf
   buf.value = setval(table.concat(logging.getring()))
end

------------------------------------------------------------------------------
-- Clear logging buffer and logging ringbuffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.clearlog(self)
   local buf = self.logbuf
   logging.clsring()
   if not buf then return end
   buf.value = setval("")
end

------------------------------------------------------------------------------
-- Syntax highlight current buffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.highlight(self)
   local buf = self:getbuf()
   buf.hlock = true
   if #buf.value > 0 and not buf.tokdone then
      buf.tok = buf.tok or lexer.lua(buf.value, {comments = false, number = true, space = true})
      for i = 1, 20 do
	 local res = buf:highlight(buf.tok)
	 if not res then 
	    buf.tok = nil 
	    buf.tokdone = true 
	    buf.hlock = false
	    buf.caret = caret
	    break 
	 end
      end
   end
   buf.hlock = false
end

------------------------------------------------------------------------------
-- Set cursor and scroll to line.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.goto(self, forceoff)
   local dlg = self:dialog()
   local buf = self:getbuf()
   if not self.gotoliner then
      self.gotoliner = gotoline()
   end
   if not forceoff and not self.gotoliner.present then
      iup.Append(dlg.workarea, self.gotoliner)
      self.gotoliner[1].title = "Goto:"
      if self.mapped then
	 iup.Map(self.gotoliner)
      end
      iup.Refresh(self.gotoliner)
      self.gotoliner.present = true
      menuitems.search.gotoline.value = on
   else
      iup.Detach(self.gotoliner)
      iup.Refresh(self:dialog())
      self.gotoliner.present = false
      menuitems.search.gotoline.value = off
   end
end

------------------------------------------------------------------------------
-- Perform a Luayats garbage collection (complete).
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.gc(self)
   collectgarbage("collect")
   self:dialog().statusbar:updateclock()
end

------------------------------------------------------------------------------
-- Syntax highlight the current buffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.highlightall(self)
   local buf = self:getbuf()
   local max = #buf.value
   local zbox = self:dialog().statusbar.zbox
   local progress = zbox.progress
   zbox.value = progress
   if self.hltimer then self.hltimer:cancel() end
   local step = max/50
   local cursor = buf:getcursor()
   buf.formatting = yes
   if max > 0 then
      buf.undostack:push(buf:catch())
      buf.tok = buf.tok or lexer.lua(buf.value, {comments = false, number = true, space = true})
      local res, oldpos = buf:highlight(buf.tok)
      while res do
	 res, pos = buf:highlight(buf.tok)
	 pos = pos or #buf.value
	 if pos - oldpos > step then
	    progress.value = pos/max
	    oldpos = pos
	    iup.Redraw(progress, 0)
	 end
	 iup.LoopStep()
	 iup.Flush()
      end
      progress.value = 1
      buf.tok = nil
      buf.highlighted = true
   end
   buf.formatting = no
   local tim = self.hltimer or self:timer(2000, function(self)
						   zbox.value = zbox.message
					       end)
   self.hltimer = tim
   tim:start()
   buf:setcursor(cursor)
end

------------------------------------------------------------------------------
-- Show/Hide the status window.
-- @param self Notebook handle.
-- @param onoff Show or hide (true or false).
-- @param text Text to show.
-- @return none.
------------------------------------------------------------------------------
function notebook.statuswindow(self, onoff, text)
   local dlg = self:dialog()
   if not self.statuswin then
      self.statuswin = statuswindow()
   end
   if onoff == true then
      if not self.statuswin.present then
	 iup.Insert(dlg.workarea, dlg.notebook, self.statuswin)
	 if self.mapped then
	    iup.Map(self.statuswin)
	 end
	 iup.Refresh(self.statuswin)
	 self.statuswin.present = true
      end
      self.statuswin:print(text)
   else
      iup.Detach(self.statuswin)
      iup.Refresh(dlg)
      self.statuswin.present = false
   end
end

------------------------------------------------------------------------------
-- Open or close the object browser.
-- @param self Notebook handle.
-- @param onoff Show/Hide object browser.
-- @return none.
------------------------------------------------------------------------------
function notebook.objectbrowser(self, onoff)
   local treetab = buildObjectTree()
   local dlg = self:dialog()
   if not self.objtree then
      self.objtree = objectbrowser(treetab)
   end
   if onoff == true then
      menuitems.simul.objbrowse.title = "Close &Browser"
      iup.Insert(dlg.workspace, dlg.workarea, self.objtree)
      if self.mapped then
	 iup.Map(self.objtree)
	 iup.TreeSetValue(self.objtree.tree, treetab)
      end
      iup.Refresh(self.objtree)
      self.objtree.present = true
   else
      iup.Detach(self.objtree)
      self.objtree.present = nil
      menuitems.simul.objbrowse.title = "Object &Browser"
      iup.Refresh(self:dialog())
   end
end
------------------------------------------------------------------------------
-- Syntax check (Lua) the given buffer.
-- Uses the status window and text selection to direct the user 
-- to syntax problems.
-- @param self Notebook handle.
-- @param buf Buffer handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.checkbuf(self, buf)
   local f, err = loadstring(buf.value)
   if err then
      self:statuswindow(true, err)
      local _,_,s = string.find(err, "^.-:(%d+):.+")
      local line = tonumber(s)
      local a = iup.TextConvertLinColToPos(buf, line, 1)
      local e = iup.TextConvertLinColToPos(buf, line+1, 1)-1
      buf.selectionpos = string.format("%d:%d", a, e)
   else
      iup.Message("Syntax Check", "Well done. No errors!")
   end
end

------------------------------------------------------------------------------
-- Syntax check the current buffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.check(self)
   local buf = self:getbuf()
   self:checkbuf(buf)
end

------------------------------------------------------------------------------
-- Show Luayats help in the system's default browser.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.help(self)
   if driver ~= "Win32" then
      iup.SetGlobal("HELPAPP", conf.yats.BrowserPath)
      log:debug(string.format("Invoking non-win help on path %q", "file://"..conf.yats.DocPath))
      iup.Help("file://"..conf.yats.DocPath)
   else
      log:debug(string.format("Invoking win help on path %q", "file:///"..conf.yats.DocPath))
      iup.Help("file:///"..conf.yats.DocPath)
   end
end

------------------------------------------------------------------------------
-- Save logging ring into 'luayats.log'.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.savelog(self)
   local fout = assert(io.open("luayats.log", "w"))
   if fout then
      fout:write(string.format("%s", table.concat(logging.getring())))
      fout:close()
   end
end

------------------------------------------------------------------------------
-- Edit configuration in a text buffer.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.editopt(self)
   local s = "return ".. pretty(cfg.data)
   local buf = self.configbuf or self:append("Configuration", s, "Luayats Configuration", true)
   buf.protected = true
   self.configbuf = buf
end

------------------------------------------------------------------------------
-- Save opened configuration into $HOME/.luayatsrc.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.saveopt(self)
   local buf = self.configbuf
   if not buf then return end
   buf.protected = false
   buf:write(os.getenv("HOME").."/.luayatsrc")
   self:message("Config written to "..os.getenv("HOME").."/.luayatsrc")
   self:removebuf(buf)
end

------------------------------------------------------------------------------
-- Revert configuration edit buffer content to default.
-- @param self Notebook handle.
-- @param none.
------------------------------------------------------------------------------
function notebook.defaultopt(self)
   local buf = self.configbuf
   local oldval = buf.value
   buf.value = setval("return "..pretty(cfg.default()))
   if oldval ~= buf.value then
      buf.changed = true
      buf.saved = false
   end
end

------------------------------------------------------------------------------
-- Close the configuration edit buffer w/o saveing.
-- @param self Notebook handle.
-- @param none.
------------------------------------------------------------------------------
function notebook.closeopt(self, save)
   local buf = self.configbuf
   self:removebuf(buf)
end


------------------------------------------------------------------------------
-- Exit Luayats.
-- @param self Notebook handle.
-- @param none.
------------------------------------------------------------------------------
function notebook.exit(self)
   log:debug("notebook.exit: about to leave")
   self:saveopt()
   if string.upper(conf.gui.SaveLogOnExit) == on then
      log:debug("notebook.exit: saving log to luayats.log")
      notebook:savelog()
   end
   if string.upper(conf.kernel.SaveConfigOnExit) == on then
      log:debug("notebook.exit: saving configuration")
      cfg.save(conf)
   end
   if notebook:closeall() == true then
      os.exit(0)
   end
end

------------------------------------------------------------------------------
-- Creates a timer.
-- Timer must be started programmatically.
-- @param self Notebook handle.
-- @param time Time in ms.
-- @param action Function with action to perform on timeout.
-- @param cyclic Time shoots cyclically (true) or not (false or nil).
-- @return timer handle.
------------------------------------------------------------------------------
function notebook.timer(self, time, action, cyclic, ...)
   local tim = iup.timer{
      args = {select(1, ...)},
      time = time,
      act = action,
      cyclic = cyclic,
      action_cb = function(this)
		     if not this.cyclic then
			this.act(this, this.args)
			this.docancel = nil
			this.running = nil
			this.run = off
		     else 
			if this.docancel then
			   this.running = nil
			   this.run = off
			   this.docancel = nil
			else
			   if this.act(this, this.args) == "stop" then
			      this.run = off
			      this.running = nil
			   end
			end
		     end
		  end,
      ---------------------------------------------------------
      -- Start given timer.
      -- @param self Timer handle.
      -- @return none.
      start = function(self) 
		 self.run = on 
		 self.running = true 
		 self.docancel = nil
	      end,
      ---------------------------------------------------------
      -- Cancel given timer
      -- @param self Timer handle.
      -- @return none.
      cancel = function(self) 
		  self.docancel = true 
	       end,
      
      ---------------------------------------------------------
      -- Destroy given timer.
      -- @param self Timer handle.
      -- @return none.
      destroy = function(self) iup.Destroy(self) end
   }
   return tim
end

------------------------------------------------------------------------------
-- Display a message for some time in statusbar.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.message(self, msg, time)
   local sbar = self:dialog().workarea.statusbar
   sbar.message.title = " " .. string.sub(msg, 1, 80)
   sbar.zbox.value = sbar.message
   local tim = self.messagetimer or self:timer(time or 2000, function(self) 
								sbar.message.title = ""
							     end)
   self.messagetimer = tim
   tim:start()
end

------------------------------------------------------------------------------
-- Display the simulation state and time in the statusbar.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.simtime(self, func)
   local sbar = self:dialog().workarea.statusbar
   local function showit(self)
      local state = getState()
      sbar.simtime.title = ""
      sbar.simtime.title = string.format("%s: %.4f s [%d]", getState(), func())
      if state == "RUN" or state == "PAUSE" then
	 sbar.zbox.value = sbar.simtime
	 return "continue"
      else
	 return "stopme"
      end
   end
   local tim = self.simtimer or self:timer(250, showit, true)
   sbar.zbox.value = sbar.simtime
   self.simtimer = tim
   tim:start()
end

------------------------------------------------------------------------------
-- Show a dialog with Luayats bindings.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.bindings(self)
   local dlg = self:dialog()
   dlg.cursor = busy
   iup.Flush()
   self.binddialog = self.binddialog or buildBindings(yats:getInfo())
   self.binddialog:show()
   dlg.cursor = arrow
   iup.Flush()
end

------------------------------------------------------------------------------
-- Define font for buffer contents.
-- @param self Notebook handle.
-- @return Font in host OS representation.
------------------------------------------------------------------------------
function notebook.deffont(self)
   local fontdlg = iup.fontdlg{
      title = "Select Font",
      color = "128 0 255",
      value = conf.gui.EditorFont
   }
   fontdlg:popup(iup.CENTER, iup.CENTER)
   if tonumber(fontdlg.status) == 1 then
      return fontdlg.value
   else
      return conf.gui.EditorFont
   end
end

------------------------------------------------------------------------------
-- Set buffer font.
-- @param self Notebook handle.
-- @param font Font to apply.
-- @param command 'all': all buffers, 'current': current buffer.
-- @return none.
------------------------------------------------------------------------------
function notebook.setfont(self, font, flag)
   local buf = self:getbuf()
   if flag == "current" then
      buf.command = true
      buf.font = font
   elseif flag == "all" then
      for i=1,self:getnbuf() do
	 local buf = self[i]
	 buf.command = true
	 buf.font = font
      end
   end
end

------------------------------------------------------------------------------
-- Display an information dialog.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.about(self)
   local version = 
      yats.YATS_VERSION_MAJOR.."."..
      yats.YATS_VERSION_MINOR.."-"..
      yats.LUAYATS_VERSION_MAJOR.."."..
      yats.LUAYATS_VERSION_MINOR

   local dlg = iup.dialog{
      title = "Luayats - Information",
      icon = images.app,
      iup.vbox {
	 gap = 6,
	 margin = "4x4",
	 iup.label{title="Luayats "..version},
	 iup.frame {
	    iup.vbox {
	       iup.label{title="(C) yats: University of Dresden 1995-1998"},
	       iup.label{title="(C) Lua, IUP, IM, CD: Tecgraf/PUC-Rio, http://www.puc-rio.br"},
	       iup.label{title="(C) lualogging: The Kepler Project, http://www.keplerproject.org"},
	       iup.label{title="", separator="HORIZONTAL", expand=yes},
	       iup.label{title="Herbert Leuwer"},
	       iup.label{title="Torsten Mueller"},
	       iup.label{title="Djordje"},
	       iup.label{title="Dmitry Klebanov"},
	       iup.label{title="Thorsten Kaiser"},
	       iup.label{title="", separator="HORIZONTAL", expand=yes},
	       iup.label{title="Contact: herbert.leuwer@t-online.de"}
	    }
	 },
	 iup.hbox{
	    iup.fill{},
	    iup.button{
	       title="&Close", 
	       tip = "Close this dialog",
	       image = images.ok, 
	       action="return iup.CLOSE"
	    }, 
	    iup.button{
	       image = images.info,
	       title="&Show Version and System Info ...", 
	       tip = "Show detailed version information.",
	       action = function(self) notebook:version() end
	    }
	 },
      },
      maxbox=no, minbox=no, resize=no, title="LUAYATS - About"
   }
   dlg:popup()
end

------------------------------------------------------------------------------
-- Display a deeper information dialog.
-- @param self Notebook handle.
-- @return none.
------------------------------------------------------------------------------
function notebook.version(self)
   local version = 
      yats.YATS_VERSION_MAJOR.."."..
      yats.YATS_VERSION_MINOR.."-"..
      yats.LUAYATS_VERSION_MAJOR.."."..
      yats.LUAYATS_VERSION_MINOR
   local info = ""
   local function txt(s)
      info = info .. s .. "\n"
   end
   txt("Luayats version: "..version)
   txt("")
   txt("Libraries:")
   txt("  YATS  "..yats.YATS_VERSION_MAJOR.."."..yats.YATS_VERSION_MINOR)
   txt("  IUP   "..iup.Version())
   txt("  CD    "..cd.Version())
   txt("  tolua 1.0.92a")
   if driver ~= "Win32" then
      txt("  GTK   "..iup.GetGlobal("GTKVERSION"))
   end
   txt("")
   txt("System:")
   txt("  OS Type:      "..iup.GetGlobal("SYSTEM"))
   txt("  OS Version:   ".. iup.GetGlobal("SYSTEMVERSION"))
   txt("  Computername: "..iup.GetGlobal("COMPUTERNAME"))
   txt("  Username:     "..iup.GetGlobal("USERNAME"))
   txt("")
   txt("Screen:")
   txt("  Driver:    "..iup.GetGlobal("DRIVER"))
   txt("  Size:      "..iup.GetGlobal("SCREENSIZE"))
   txt("  Full Size: "..iup.GetGlobal("FULLSIZE"))

   txt("  Depth:     "..iup.GetGlobal("SCREENDEPTH"))
   local dlg = iup.dialog {
      title="Luayats - Version and System Info", 
      icon = images.app,
      expand = yes,
      iup.vbox {
	 gap = 6,
	 margin = "4x4",
	 iup.frame{
	    iup.label{title = info, font="Monospace 9"}
	 },
	 iup.hbox{
	    iup.fill{},
	    iup.button{
	       title="&Close", 
	       tip = "Close this dialog",
	       image = images.ok, 
	       action = function(self) iup.GetDialog(self):hide() end
	    }, 
	 },
      },
      maxbox=no, minbox=no, resize=no
   }
   dlg:popup()
end

local function pushlist(self, val)
   local n = tonumber(self.count) + 1
   local m = tonumber(self.maxcount)
   if n > m then
      n = m
   end
   for i = n, 2, -1 do
      if self[i-1] == val then return end
      self[i] = self[i-1]
   end
   self[1] = val
end

------------------------------------------------------------------------------
-- Find Dialog constructor.
-- @return Dialog handle.
------------------------------------------------------------------------------
function finder()
   local toggleword = iup.toggle{title = "Match whole word &only", name = "toggleword"}
   local togglecase = iup.toggle{title = "&Match case", name = "togglecase"}
   local togglegsub = iup.toggle{title = "Use Lua &gsub pattern", name = "togglegsub"}
   local toggleregexp = iup.toggle{title = "Regular e&xpression", name = "toggleregexp"}
   local togglewrap = iup.toggle{title = "&Wrap around", value = on, name = "togglewrap"}
   local toggleback = iup.toggle{title = "&Reverse direction", name = "toggleback"}
   local searchlist = iup.list{
      editbox = yes,
      dropdown = yes,
      expand = yes,
      maxcount = 10,
      push = pushlist,
      name = "searchlist",
      append = function(self, val)
		  for i = 1,self.count do
		     if self[i] == val then
			return
		     end
		  end
		  self[self.count + 1] = val
	       end
   }
   local butcancel = iup.button{
      title = "&Close", 
      image = images.cancel,
      tip = "Hide this dialog",
      action = function(self) 
		  iup.GetDialog(self):hide()
	       end
   }
   local butsearch = iup.button{
      title = "&Search",
      image = images.find,
      tip = "Start searching",
      name = "butsearch",
      action = function(self)
		  local buf = notebook:getbuf()
		  local pattern = searchlist.value
		  local found, path = buf:search(pattern, toggleword.value, togglecase.value, 
					   toggleback.value, togglewrap.value)
		  if found == true then
		     searchlist:push(pattern)
		  else
		     notebook:warn("find")
		  end
		  return
	       end
   }
   local dlg = iup.dialog{
      title = "Find Text",
      parentdialog = notebook:dialog(),
      size = "THIRDx",
      resize = no,
      iup.vbox{
	 gap = "4x4",
	 margin = "2x2",
	 alignment = "ALEFT",
	 searchlist,
	 toggleword,
	 togglecase,
-- maybe later
--	 togglegsub,
--	 toggleregexp,
	 togglewrap,
	 toggleback,
	 iup.label{separator=horizontal},
	 iup.hbox{
	    iup.fill{}, butcancel, butsearch
	 }
      },
      getchild = iup.GetDialogChild
   }
   return dlg
end

------------------------------------------------------------------------------
-- Replace Dialog Constructor.
-- @return Dialog handle.
------------------------------------------------------------------------------
function replacer()
   local toggleword = iup.toggle{title = "Match whole word &only"}
   local togglecase = iup.toggle{title = "&Match case"}
   local togglegsub = iup.toggle{title = "Use Lua &gsub pattern"}
   local toggleregexp = iup.toggle{title = "Regular e&xpression"}
   local togglewrap = iup.toggle{title = "&Wrap around", value = on}
   local toggleback = iup.toggle{title = "&Reverse direction"}
   local searchlist = iup.list{
      editbox = yes,
      dropdown = yes,
      expand = yes,
      maxcount = 10,
      push = pushlist,
      append = function(self, val)
		  for i = 1,self.count do
		     if self[i] == val then
			return
		     end
		  end
		  self[self.count + 1] = val
	       end
   }
   local replacelist = iup.list{
      editbox = yes,
      dropdown = yes,
      expand = yes,
      maxcount = 10,
      push = pushlist,
      append = function(self, val)
		  for i = 1,self.count do
		     if self[i] == val then
			return
		     end
		  end
		  self[self.count + 1] = val
	       end
   }
   local butcancel = iup.button{
      title = "&Close", 
      image = images.cancel,
      tip = "Hide this dialog",
      action = function(self) 
		  iup.GetDialog(self):hide()
	       end
   }
   local butreplaceall = iup.button{
      title = "Replace &all",
      tip = "Replace all instances",
      expand = vertical,
      action = function(self)
		  local buf = notebook:getbuf()
		  local pattern = searchlist.value
		  local rpattern = replacelist.value
		  local replaced = 0
		  local path = 1
		  buf.visible = no
		  self.parentdialog.cursor = busy
		  local found, pos = buf:search(pattern, toggleword.value, togglecase.value, 
						toggleback.value, togglewrap.value)
		  local lpos, first = pos, pos
		  if found == true then
		     log:debug(string.format("   replace: from %q to %q", buf.selectedtext, rpattern))
		     buf.undostack:push(buf:catch())
		     replaced = replaced + 1
		     local posbeg, posend = buf:getselection()
		     buf.selectedtext = rpattern
		     buf:setselection(posbeg, posbeg + string.len(rpattern))
		     if toggleback.value == on then
			buf.nextstart = posbeg - 1
		     else
			buf.nextstart = posend + 1
		     end
		  else
		     buf.visible = yes
		     self.parentdialog.cursor = "ARROW"
		     notebook:warn("find")
		     return
		  end
		  while found == true do
		     found, pos = buf:search(pattern, toggleword.value, togglecase.value, 
					     toggleback.value, togglewrap.value)
		     log:debug(string.format("   replace: from %q to %q", buf.selectedtext or "", rpattern))
		     if found == true then
			local posbeg, posend = buf:getselection()
			if toggleback.value == on then
			   if pos < lpos then path = 2 end
			   if path == 2 and pos > first then break end
			   buf.nextstart = posbeg - 1
			else
			   if pos < lpos then path = 2 end
			   if path == 2 and pos > first then  break end
			   buf.nextstart = posend + 1
			end
			lpos = pos
			buf.selectedtext = rpattern
			replaced = replaced + 1
			buf:setselection(posbeg, posbeg + string.len(rpattern))
		     end
		  end
		  buf.visible = yes
		  self.parentdialog.cursor = arrow
		  if replaced > 0 then
		     notebook:mark(true)
		     searchlist:push(pattern)
		     replacelist:push(rpattern)
		     iup.Message("Info", string.format("%d occurences replaced", replaced))
		  end
	       end
   }
   local butreplace = iup.button{
      title = "&Replace",
      -- Reactivate once we have a good icon for windows
      -- image = images.replace,
      tip = "Replace one instance",
      expand = vertical,
      action = function(self)
		  local buf = notebook:getbuf()
		  buf.undostack:push(buf:catch())
		  local rpattern = replacelist.value
		  local posbeg, posend = buf:getselection()
		  buf.selectedtext = rpattern
		  buf:setselection(posbeg, posbeg + string.len(rpattern))
		  if toggleback.value == on then
		     buf.nextstart = posbeg - 1
		  else
		     buf.nextstart = posend + 1
		  end
		  notebook:mark(true)
		  replacelist:push(rpattern)
	       end
   }
   local butsearch = iup.button{
      title = "&Search",
      image = images.find,
      tip = "Start searching",
      action = function(self)
		  local buf = notebook:getbuf()
		  local pattern = searchlist.value
		  local found = buf:search(pattern, toggleword.value, togglecase.value, 
					   toggleback.value, togglewrap.value)
		  if found == true then
		     searchlist:push(pattern)
		  else
		     notebook:warn("find")
		  end
	       end
   }
   local dlg = iup.dialog{
      title = "Find Text",
      parentdialog = notebook:dialog(),
      size = "THIRDx",
      resize = no,
      iup.vbox{
	 gap = "4x4",
	 margin = "2x2",
	 alignment = aleft,
	 iup.hbox{
	    iup.vbox{
	       alignment = aleft,
	       iup.label{title = " Find what:", expand = vertical},
--	       iup.fill{},
	       iup.label{title = "Replace by:", expand = vertical},
	    },
	    iup.vbox{
	       alignment = aleft,
	       searchlist,
	       replacelist,
	    },
	 },
	 toggleword,
	 togglecase,
-- maybe later
--	 togglegsub,
--	 toggleregexp,
	 togglewrap,
	 toggleback,
	 iup.label{separator=horizontal},
	 iup.hbox{
	    iup.fill{}, butcancel, butreplace, butreplaceall, butsearch
	 }
      }
   }
   return dlg
end

------------------------------------------------------------------------------
-- Incremental Finder Constructor.
-- @return Searchbox handle.
------------------------------------------------------------------------------
function incrementalfinder()
   local searchbox = iup.text{
      expand = horizontal,
      valuechanged_cb = function(self)
			   self:find()
			end,
      find = function(self)
		local buf = notebook:getbuf()
		buf.nextstart = buf.startincremental
		local found = buf:search(self.value, off, off, off, off, true)
	     end
   }
   local box = iup.hbox{
      alignment = "ACENTER",
      iup.label{title = "Find:"},
      searchbox,
      searchbox = searchbox,
   }
   return box
end

------------------------------------------------------------------------------
-- Gotoline constructor Constructor.
-- @return Status box handle.
------------------------------------------------------------------------------
function gotoline()
   local gotobox = iup.text{
      expand = horizontal,
      valuechanged_cb = function(self)
			   self:goto(tonumber(self.value))
			end,
      goto = function(self, line)
		local buf = notebook:getbuf()
		if line and line > 2 then
--		   buf:setcaret(line, 1)
		   buf:scroll(line-2, 1)
		end
	     end
   }
   local box = iup.hbox{
      alignment = "ACENTER",
      iup.label{title = "Goto:"},
      gotobox,
      gotobox = gotobox
   }
   return box
end
------------------------------------------------------------------------------
-- Statuswindow Constructor.
-- @return Status box handle.
------------------------------------------------------------------------------
function statuswindow()
   local statusbox = iup.text{
      expand = horizontal,
      valuechanged_cb = function(self)
		   self:find()
		end,
   }
   local box = iup.hbox{
      alignment = "ACENTER",
      iup.button{
--	 title="&Close",
	 image = images.ok,
	 flat = "yes",
	 action = function(self) 
		     notebook:statuswindow(false)
		  end,
	 map_cb = function(self) 
--		     self.title = "&Close" 
		     self.image = images.ok 
		  end
      },
      statusbox,
      statusbox = statusbox,
      print = function(self, value)
		 self.statusbox.value = value
	     end
   }
   return box
end

------------------------------------------------------------------------------
-- Create an objectbrowser box.
-- @param treetab Table representation of tree to show.
-- @return Reference to browser box.
------------------------------------------------------------------------------
function objectbrowser(treetab)
   local objtree = iup.tree{
      scrollbar = yes,
      addexpanded = yes,
      selection_cb = objselection_cb,
      branchopen_cb = objbranchopen_cb,
      branchclose_cb = objbranchclose_cb,
      executeleaf_cb = objexecuteleaf_cb,
      rightclick_cb = objrightclick_cb,
      size = "THIRDX"
   }
   local box = iup.sbox{
      tree = objtree,
      direction = "EAST",
      iup.vbox{
	 objtree,
	 iup.hbox{
	    iup.fill{},
	    iup.button{
	       title = "&Close browser",
	       image = images.ok,
	       flat = yes,
	       action = function() notebook:objectbrowser(false) end
	    }
	 }
      }
   }
   return box
end

------------------------------------------------------------------------------
-- The editor.
-- Opens the GUI.
-- @param title Dialog title.
-- @param icon Dialog icon to use.
-- @param ... File list.
-- @return Dialog handle.
------------------------------------------------------------------------------
function editor(title, icon, ...)
   local icon = icon or images.app
   local title = title or "Luayats"
   local toolbar = toolbar(yes)
   local statusbar = statusbar()
   local workarea = iup.vbox{
--      toolbar,
      notebook,
--      statusbar
   }
   local workspace = iup.hbox {
      workarea,
   }
   local dlg = iup.dialog{
      icon = icon,
      title = conf.gui.MainDialogTitle,
      defaultenter = menuactions.simul.run,
      defaultesc = nil,
      size = conf.gui.EditorWindowSize,
      iup.vbox{
	 toolbar,
	 workspace,
	 statusbar
      },
--      workarea,
      shrink=yes,
      expand = yes,
      menu = menu,
      -- Properties toolbar and notebook with tabs
      toolbar = toolbar,
      notebook = notebook,
      statusbar = statusbar,
      messagebar = messagebar,
      workarea = workarea,
      workspace = workspace,
      -- callbacks
      getfocus_cb = function(self) iup.Redraw(self, 1) end,
      enterwindow_cb = function(self) iup.Redraw(self, 1) end,
      k_any = function(self, c)
		 print("dlg.k_any:", c)
		 if iup.isCtrlXkey(c) or c >= iup.K_F1 then
		    print("  => process")
		    return menu:processkey(c)
		 else
		    print("  => continue")
		    return iup.CONTINUE
		 end
	      end,
      close_cb = function(self)
		    log:debug("editor.close_cb: about to leave")
		    notebook:exit()
--		    if notebook:closeall() == true then
--		       os.exit(0)
--		    else
		       return iup.IGNORE
--		    end
		 end,
      map_cb = function(self) 
		  self.mapped = true 
	       end,
      show_cb = function(self)
		   if self._timer.running == false then
		      self._timer.run = on
		      self._timer.running = true
		   end
		   if self.timersyntax.running == false then
		      self.timersyntax.run = on
		      self.timersyntax.running = true
		   end
--		   self.statusbar.progress.visible = yes
		   if false then
		      printf("dialog:    %s", self.clientsize)
		      printf("workarea:  %s", self.workarea.clientsize)
		      printf("toolbar:   %s", self.toolbar.clientsize)
		      printf("notebook:  %s", self.notebook.clientsize)
		      printf("statusbar: %s", self.statusbar.clientsize)
		      printf("==== notebook ====")
		      printf("notebook.rastersize: %s", self.notebook.rastersize)
		      printf("notebook.size:       %s", self.notebook.size)
		      printf("==== menu ====")
		      printf("menu.rastersize: %s", self.menu.rastersize)
		      printf("menu.size:       %s", self.menu.size)
		      printf("==== self ====")
		      printf("self.rastersize: %s", self.rastersize)
		      printf("self.size:       %s", self.size)
		      printf("==============")
		      printf("default font: %s", iup.GetGlobal("DEFAULTFONT"))
		      printf("default fontsize: %s", iup.GetGlobal("DEFAULTFONTSIZE"))
		      printf("==============")
		      printf("gtk version: %s", iup.GetGlobal("GTKVERSION"))
		      printf("gtk dev version: %s", iup.GetGlobal("GTKDEVVERSION"))
		      printf("==============")
		      printf("icon: %s", tostring(self.icon))
		   end
		end,
      dropfiles_cb = function(self, fname, x, y)
			notebook:open(fname)
		     end,
      -- Methods to create/remove new buffer via scripts
      append = function(self, ...) return notebook:append(...) end,
      remove = function(self, ...) return notebook:remove(...) end,
      next = function(self, ...) return notebook:next(...) end,
      prev = function(self, ...) return notebook:prev(...) end,
      closeall = function(self, ...) return notebook:closeall(...) end,
      -- Methods to add menus
      -- TODO
      -- Methods to add toolbars
      -- TODO
      -- Timer
      _timer = iup.timer{
	 time=100,
	 running = false,
	 action_cb = function(self) 
			menu:update()
		     end,
      },
      timersyntax = iup.timer{
	 time = 1000,
	 running = false,
	 action_cb = function(self)
			statusbar:updateclock()
		     end
      }
   }
   dlg:show(conf.gui.EditorWindowSize)
   if select("#", ...) == 0 then
      notebook:append(nil, nil)
   else
      notebook:append(nil, nil)
      for _, arg in ipairs{select(1,...)} do
	 local f, err = io.open(arg, "r")
	 notebook:open(arg)
	 iup.Flush()
      end
   end
   return dlg
end

return gui