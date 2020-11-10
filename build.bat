
@echo off

IF NOT EXIST ./dist mkdir dist
set INCLUDES=/I "W:\CppProject\CppLib\CImgui\include"
set COMPOPTION=/O2 /Zi /MD /Fo"dist/main_win32.obj" /Fd"dist/vc140.pdb"

set LIBPATH=/LIBPATH:"W:\CppProject\CppLib\CImgui\lib"
set LINKS=user32.lib shell32.lib gdi32.lib cimgui.lib
set LINKOPTION=/INCREMENTAL:NO /pdb:"dist/main.pdb" /out:"dist/main.exe" /PROFILE

cl.exe %COMPOPTION% %INCLUDES% src/main_win32.c /link %LINKOPTION% %LIBPATH% %LINKS% 

