cd /D "%~dp0"
pushd ..\..
call vendor\bin\premake\premake5.exe vs2019
popd