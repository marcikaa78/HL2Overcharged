@echo off
setlocal

rem ================================
rem ==== MOD PATH CONFIGURATIONS ===

rem == Set the absolute path to your mod's game directory here ==
set GAMEDIR=C:\Steam\steamapps\sourcemods\OverCharged_EPISODIC_CSM_CONTENT\shaders

rem == Set the relative or absolute path to Source SDK Base 2013 Singleplayer\bin ==
set SDKBINDIR=C:\Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin

rem ==  Set the Path to your mod's root source code ==
rem This should already be correct, accepts relative paths only!
set SOURCEDIR=..\..

rem ==== MOD PATH CONFIGURATIONS END ===
rem ====================================


call buildsdkshaders.bat

@echo Finished building shaders
@pause
