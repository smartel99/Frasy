@echo off
set root_dir=%~dp0..\..
set lua_user_dir=%root_dir%\..\..\src\lua\user
set lua_user_hash=%root_dir%\..\..\src\lua\hash
set lua_core_dir=%root_dir%\Frasy\lua\core
set lua_core_hash=%root_dir%\Frasy\lua\core\hash

@echo on
hashdir.exe %lua_user_dir% %* > %lua_user_hash%
hashdir.exe %lua_core_dir% -f in f .+\.lua > %lua_core_hash%