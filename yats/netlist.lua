---------------------------------------------------------------
-- @title LuaYats - Netlists.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @date 02/02/2005.

-- Yats programming in Lua.

-- This module defines netlist functions in Luayats.

module("yats", yats.seeall)

local function createDot()
  return {
    nodes={},
    edges={}
  }
end

local function addNode(t, objname)
  print(string.format("Adding node %s", objname))
  table.insert(t.nodes, objname)
end

local function addEdge(t, src, spin, dest, dpin)
  print(string.format("%s.%s => %s.%s", src, spin, dest, dpin))
  table.insert(t.edges, {src=src, spin=spin, dest=dest, dpin=dpin})
end

function makeDot(fname)
  local t = createDot()
  local hash = {}
  for name, obj in pairs(sim.objectlist) do
    addNode(t, name)
    for i, v in obj:successors() do
      addEdge(t, name, "out"..i, v.suc.name, v.suc.inputs[v.shand+1])
    end
  end
  local f = io.open(fname..".dot","w+")
  print(f,fname)
  f:write(string.format("digraph G {\n"))
  f:write(string.format("rankdir LR;\n"))
  for _,v in ipairs(t.nodes) do
    f:write(string.format("  %s [shape=box];\n", string.gsub(v,"-","_")))
  end
  for _, v in ipairs(t.edges) do
    f:write(string.format('  %s -> %s [label="%s => %s"];\n', 
			  string.gsub(v.src,"-","_"), string.gsub(v.dest,"-","_"), v.spin, v.dpin))
  end
  f:write(string.format("}\n"))
  f:close()
end
return yats
