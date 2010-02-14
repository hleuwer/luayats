require "iupluaimglib"
--require "yats.icons.edit_find_replace"
require "yats.icons.IUP_Lua"
module("gui", package.seeall)

images = {
   ok = "IUP_ActionOk",
   cancel = "IUP_ActionCancel",
   app = load_image_IUP_Lua(), --"IUP_Lua",
   run = "IUP_MediaPlay",
   stop = "IUP_MediaStop",
   reset = "IUP_MediaGotoBegin",
   new = "IUP_FileNew",
   open = "IUP_FileOpen",
   close = "IUP_FileClose",
   closeall = "IUP_FileCloseAll",
   save = "IUP_FileSave",
   saveall = "IUP_FileSaveAll",
   copy = "IUP_EditCopy",
   cut = "IUP_EditCut",
   paste = "IUP_EditPaste",
   find = "IUP_EditFind",
   print = "IUP_Print",
--   replace = "IUP_EditFind",
--   replace = load_image_edit_find_replace(),
   check = "IUP_ActionOk",
   undo = "IUP_EditUndo",
   redo = "IUP_EditRedo",
   tools = "IUP_ToolsSettings",
   cancel = "IUP_ActionCancel",
   help = "IUP_MessageHelp",
   info = "IUP_MessageInfo",
   next = "IUP_ArrowRight",
   prev = "IUP_ArrowLeft",
   home = "IUP_NavigateHome"
}

return gui