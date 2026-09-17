@echo off
pushd ..\build
set FILE=..\code\main.c
set LIBS=kernel32.lib

::set FLAG=/O2 /Ob2 /DNDEBUG /nologo /Gm- /GR- /EHa- /GS- /Gs9999999 /DRELEASE=0
set FLAG=/nologo /Gm- /GR- /EHa- /Oi /Ob1 /Zi /GS- /Gs9999999 /DRELEASE=0
set LINK=/subsystem:console /nodefaultlib /entry:__WeaMain__ /stack:0x100000,0x100000

cl %FLAG% %FILE% %LIBS% /link %LINK%
popd