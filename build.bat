
@echo off

IF NOT EXIST ./dist mkdir dist
set INCLUDES=
set COMPOPTION=/O2 /Zi /MD /Fo"dist/main_win32.obj" /Fd"dist/vc140.pdb" /source-charset:utf-8

set LIBPATH=
set LINKS=user32.lib shell32.lib gdi32.lib
set LINKOPTION=/INCREMENTAL:NO /pdb:"dist/main.pdb" /out:"dist/main.exe" /PROFILE

cl.exe %COMPOPTION% %INCLUDES% src/main_win32.cpp /link %LINKOPTION% %LIBPATH% %LINKS% 

