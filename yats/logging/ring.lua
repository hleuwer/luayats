-------------------------------------------------------------------------------
-- Prints logging information to console
--
-- @author Thiago Costa Ponte (thiago@ideais.com.br)
--
-- @copyright 2004-2007 Kepler Project
--
-- @release $Id: console.lua 222 2009-06-06 05:51:35Z leuwer $
-------------------------------------------------------------------------------

local logging = require "yats.logging"
local ringbuf = {}
local meta = {__tostring = function(self)
			      return table.concat(ringbuf)
			   end
	   }
setmetatable(ringbuf, meta)
local maxsize = 10000
function logging.ring(logPattern, msize)
   if msize and msize > maxsize then
      maxsize = msize
   end
   local size = 0
   return logging.new(function(self, level, message)
			 table.insert(ringbuf, logging.prepareLogMsg(logPattern, os.date(), level, message, 
									  _G.yats.SimTime, _G.yats.SimTimeReal))
			 size = size + 1
			 if size > maxsize then
			    table.remove(ringbuf, 1)
			 end
			 return true
		      end
		   )
end

function logging.getring()
   return ringbuf
end

function logging.clsring()
   ringbuf = {}
end

function logging.setringlen(size)
   maxsize = size
end

logging.ringbuf = ringbuf
return logging