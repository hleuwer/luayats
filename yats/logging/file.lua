-------------------------------------------------------------------------------
-- Saves logging information in a file
--
-- @author Thiago Costa Ponte (thiago@ideais.com.br)
--
-- @copyright 2004-2007 Kepler Project
--
-- @release $Id: file.lua 222 2009-06-06 05:51:35Z leuwer $
-------------------------------------------------------------------------------

local logging = require "yats.logging"

function logging.file(filename, datePattern, logPattern)

    if type(filename) ~= "string" then
        filename = "lualogging.log"
    end
    filename = string.format(filename, os.date(datePattern))
    local f = io.open(filename, "a")
    if not f then
       return nil, string.format("file `%s' could not be opened for writing", filename)
    end
    f:setvbuf ("line")

    return logging.new( function(self, level, message)
                            local s = logging.prepareLogMsg(logPattern, os.date(), level, message).."\n"
                            f:write(s)
                            return true
                        end
                      )
end

return logging