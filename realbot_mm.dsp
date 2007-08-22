# Microsoft Developer Studio Project File - Name="realbot_mm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=realbot_mm - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "realbot_mm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "realbot_mm.mak" CFG="realbot_mm - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "realbot_mm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/SDKSrc/Public/dlls", NVGBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MT /W3 /WX /GX /Zi /O2 /I "..\metamod" /I "..\..\devtools\sdk\Single-Player Source\common" /I "..\..\devtools\sdk\Single-Player Source\dlls" /I "..\..\devtools\sdk\Single-Player Source\engine" /I "..\..\devtools\sdk\Single-Player Source\pm_shared" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\realbot_mm.def"
# SUBTRACT LINK32 /profile /incremental:yes /map
# Begin Custom Build
TargetPath=.\Release\realbot_mm.dll
TargetName=realbot_mm
InputPath=.\Release\realbot_mm.dll
SOURCE="$(InputPath)"

"$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) c:\progra~1\steam\steamapps\s.hendriks2@chello.nl\counter-strike\realbot\dll\realbot_mm.dll 
	copy $(TargetPath) c:\progra~1\steam\steamapps\s.hendriks2@chello.nl\dedica~1\realbot\dll\realbot_mm.dll 
	copy $(TargetPath) c:\progra~1\half-life\realbot\dll\realbot_mm.dll 
	copy $(TargetPath) d:\games\half-life\realbot\dll\realbot_mm.dll 
	
# End Custom Build
# Begin Target

# Name "realbot_mm - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "BAL"

# PROP Default_Filter ""
# Begin Group "Halflife"

# PROP Default_Filter ""
# Begin Group "Metamod"

# PROP Default_Filter ""
# End Group
# End Group
# End Group
# Begin Source File

SOURCE=.\bot.cpp
# End Source File
# Begin Source File

SOURCE=.\bot_buycode.cpp
# End Source File
# Begin Source File

SOURCE=.\bot_client.cpp
# End Source File
# Begin Source File

SOURCE=.\bot_func.cpp
# End Source File
# Begin Source File

SOURCE=.\bot_navigate.cpp
# End Source File
# Begin Source File

SOURCE=.\build.cpp
# End Source File
# Begin Source File

SOURCE=.\ChatEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\dll.cpp
# End Source File
# Begin Source File

SOURCE=.\engine.cpp
# End Source File
# Begin Source File

SOURCE=.\game.cpp
# End Source File
# Begin Source File

SOURCE=.\IniParser.cpp
# End Source File
# Begin Source File

SOURCE=.\NodeMachine.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\BAL\BAL_Bot.h
# End Source File
# Begin Source File

SOURCE=.\bot.h
# End Source File
# Begin Source File

SOURCE=.\bot_client.h
# End Source File
# Begin Source File

SOURCE=.\bot_func.h
# End Source File
# Begin Source File

SOURCE=.\bot_weapons.h
# End Source File
# Begin Source File

SOURCE=.\ChatEngine.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\IniParser.h
# End Source File
# Begin Source File

SOURCE=.\NodeMachine.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\todo.txt
# End Source File
# End Target
# End Project
