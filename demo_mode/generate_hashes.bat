@echo off
set lua_user_dir=%~dp0src\lua\user
set lua_user_hash=%~dp0src\lua\hash
set lua_core_dir=%~dp0..\Frasy\lua\core
set lua_core_hash=%~dp0..\Frasy\lua\core\hash

@echo on
%~dp0..\scripts\Windows\hashdir.exe %lua_user_dir% %* > %lua_user_hash%
%~dp0..\scripts\Windows\hashdir.exe %lua_core_dir% -f in f .+\.lua > %lua_core_hash%