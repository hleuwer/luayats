-------------------------------------------------------------------------------
-- Sends the logging information through a socket using luasocket
--
-- @author Thiago Costa Ponte (thiago@ideais.com.br)
--
-- @copyright 2004-2007 Kepler Project
--
-- @release $Id: socket.lua 222 2009-06-06 05:51:35Z leuwer $
-------------------------------------------------------------------------------

local logging = require "yats.logging"
local socket = require "socket"

function logging.socket(address, port, logPattern)

    return logging.new( function(self, level, message)
                            local s = logging.prepareLogMsg(logPattern, os.date(), level, message).."\n"

                            local socket, err = socket.connect(address, port)
                            if not socket then
                                return nil, err
                            end
                            
                            local cond, err = socket:send(s)
                            if not cond then
                                return nil, err
                            end
                            socket:close()
                            
                            return true
                        end
                      )
end

return logging