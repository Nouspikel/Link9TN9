# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Link99 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Link99 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Link99 - Win32 Release" && "$(CFG)" != "Link99 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Link99.mak" CFG="Link99 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Link99 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Link99 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "Link99 - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "Link99 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\Link99.exe"

CLEAN : 
	-@erase ".\Release\Link99.exe"
	-@erase ".\Release\Aorg.obj"
	-@erase ".\Release\ScriptFile.obj"
	-@erase ".\Release\Patch.obj"
	-@erase ".\Release\Block.obj"
	-@erase ".\Release\Segment.obj"
	-@erase ".\Release\Memimage.obj"
	-@erase ".\Release\List.obj"
	-@erase ".\Release\Label.obj"
	-@erase ".\Release\main.obj"
	-@erase ".\Release\ObjectFile.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/Link99.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Link99.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/Link99.pdb" /machine:I386 /out:"$(OUTDIR)/Link99.exe" 
LINK32_OBJS= \
	"$(INTDIR)/Aorg.obj" \
	"$(INTDIR)/ScriptFile.obj" \
	"$(INTDIR)/Patch.obj" \
	"$(INTDIR)/Block.obj" \
	"$(INTDIR)/Segment.obj" \
	"$(INTDIR)/Memimage.obj" \
	"$(INTDIR)/List.obj" \
	"$(INTDIR)/Label.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/ObjectFile.obj"

"$(OUTDIR)\Link99.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Link99 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\Link99.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\Link99.exe"
	-@erase ".\Debug\main.obj"
	-@erase ".\Debug\Label.obj"
	-@erase ".\Debug\Segment.obj"
	-@erase ".\Debug\ObjectFile.obj"
	-@erase ".\Debug\Patch.obj"
	-@erase ".\Debug\Memimage.obj"
	-@erase ".\Debug\Block.obj"
	-@erase ".\Debug\ScriptFile.obj"
	-@erase ".\Debug\List.obj"
	-@erase ".\Debug\Aorg.obj"
	-@erase ".\Debug\Link99.ilk"
	-@erase ".\Debug\Link99.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/Link99.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Link99.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/Link99.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Link99.exe" 
LINK32_OBJS= \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/Label.obj" \
	"$(INTDIR)/Segment.obj" \
	"$(INTDIR)/ObjectFile.obj" \
	"$(INTDIR)/Patch.obj" \
	"$(INTDIR)/Memimage.obj" \
	"$(INTDIR)/Block.obj" \
	"$(INTDIR)/ScriptFile.obj" \
	"$(INTDIR)/List.obj" \
	"$(INTDIR)/Aorg.obj"

"$(OUTDIR)\Link99.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "Link99 - Win32 Release"
# Name "Link99 - Win32 Debug"

!IF  "$(CFG)" == "Link99 - Win32 Release"

!ELSEIF  "$(CFG)" == "Link99 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\main.cpp
DEP_CPP_MAIN_=\
	".\scriptfile.h"\
	".\List.h"\
	".\ObjectFile.h"\
	".\Block.h"\
	".\segment.h"\
	".\Label.h"\
	".\Aorg.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Label.cpp
DEP_CPP_LABEL=\
	".\Label.h"\
	".\List.h"\
	

"$(INTDIR)\Label.obj" : $(SOURCE) $(DEP_CPP_LABEL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Segment.cpp
DEP_CPP_SEGME=\
	".\segment.h"\
	".\List.h"\
	

"$(INTDIR)\Segment.obj" : $(SOURCE) $(DEP_CPP_SEGME) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ObjectFile.cpp
DEP_CPP_OBJEC=\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\ObjectFile.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	".\segment.h"\
	".\Label.h"\
	".\Aorg.h"\
	".\List.h"\
	

"$(INTDIR)\ObjectFile.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ScriptFile.cpp
DEP_CPP_SCRIP=\
	".\scriptfile.h"\
	".\Memimage.h"\
	".\Patch.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	{$(INCLUDE)}"\sys\STAT.H"\
	".\ObjectFile.h"\
	".\List.h"\
	".\Block.h"\
	".\segment.h"\
	".\Label.h"\
	".\Aorg.h"\
	

"$(INTDIR)\ScriptFile.obj" : $(SOURCE) $(DEP_CPP_SCRIP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\List.cpp
DEP_CPP_LIST_=\
	".\List.h"\
	

"$(INTDIR)\List.obj" : $(SOURCE) $(DEP_CPP_LIST_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Memimage.cpp
DEP_CPP_MEMIM=\
	".\Memimage.h"\
	".\Block.h"\
	".\segment.h"\
	".\Aorg.h"\
	".\List.h"\
	

"$(INTDIR)\Memimage.obj" : $(SOURCE) $(DEP_CPP_MEMIM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Block.cpp
DEP_CPP_BLOCK=\
	".\Block.h"\
	".\segment.h"\
	".\List.h"\
	

"$(INTDIR)\Block.obj" : $(SOURCE) $(DEP_CPP_BLOCK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Aorg.cpp
DEP_CPP_AORG_=\
	".\Aorg.h"\
	".\List.h"\
	

"$(INTDIR)\Aorg.obj" : $(SOURCE) $(DEP_CPP_AORG_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Patch.cpp
DEP_CPP_PATCH=\
	".\Patch.h"\
	".\List.h"\
	

"$(INTDIR)\Patch.obj" : $(SOURCE) $(DEP_CPP_PATCH) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
