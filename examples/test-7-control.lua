-- This a control file
--  The control file consists of actions with format:
--  {slot, string}
--  The string is valid Lua Code.
--  The control file is processed by a 'luadummy' object.
require "yats.stdlib"

-- An example function executed by actions.
function foo(a, b, c)
  printf("This is foo at %d: %d\n", yats.SimTime, a+b+c)
end
local nn = 0
function cyclic_foo(a)
  printf("\nCyclic event %d at %d with arg %s\n", nn, yats.SimTime, a)
  nn = nn + 1
end
-- The table of actions.
actions = {
  -- A table of early actions.
  early = {
    -- Calls interactive input.
    {1234, yats.cli}, 
    -- Executes the given string in global environment.
    {10000, "print('early: sending to late');earlystring = 'early occured'"},
    -- Executes the given function with parameters given in table.
    {20000, foo, arg = {1, 2, 3}},
    -- Exectuts a series of actions.
    {21000, {
	{0, "print('action 1')"}, 
	{0, "print('action 2')"},
	{0, print, arg={"whatever"}},
	{0, yats.cli}
      }
    }
  },
  -- A tabel of late actions
  late = {
    -- late slot time actions
    {1234, "print(s)"},  -- print what has been given during early timeslot
    -- Here a message is printed coming from early timeslot
    {10000, "print('late: '..earlystring)"},
    -- Executes the given lua script.
    {22000, dofile, arg={"examples/test-7-control-include.lua"}},
    {250000, cyclic_foo, arg={"hello"}, cycle=111111, ncycle = 10}
  }
}
