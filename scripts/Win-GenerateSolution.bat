@echo off

pushd %~dp0\..\
call vendor\bin\premake5.exe vs2019 --use-vld
popd

pause
