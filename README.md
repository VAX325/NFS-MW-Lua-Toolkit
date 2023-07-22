# NFS MW Lua toolkit
 Lua compiler and decompiler for Need for Speed Most Wanted.\
 Based on Hisham Muhammad decompiler with some fixes for NFS MW Bytecode.\
 Use Win32 version, because NFS MW is 32bit app.
 
# Some info
 NFS MW (Black box at least) uses modifed Lua 5.0.1. Modifed because the struct lua_Number in NFS MW weight 4 bytes (it's float), but in normal Lua 5.0.1 it's double (8 bytes).
