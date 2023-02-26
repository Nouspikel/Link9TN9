# Bug99 --------------- Project info -------------------------------
#: Project name: 
#: Version: 1.0
#: Build: 2
#: Programmer: 
#: (c): 
#: Started: 11/12/17
#: Last built: 11/19/2017 16:28:13
#: Hands-on time: 36 hrs, 13 min
#: Source files: 16
#: Code lines: 1816
#: Options: File/Seg=2, EquGroup=1, UsedPages=1, ScanSourced=1, AssignSeg=1, AutoMemory=0, External make=0 Save models=1

# Bug99 --------------- End of project info ------------------------
# Bug99 --------------- Building instructions ----------------------
.SUFFIXES: .ea5 .fb6 .o .obj .o99  .s .asm .a99 .spt .lst .cpt

# Macro declarations
APPDIR=C:\MSDEV\Projects\Bug99\
WRKDIR=C:\TI994a\USBhost\
Link99=$(APPDIR)\link99.exe
As99=$(APPDIR)\as99.exe

# Build the entire project
ALL :  Memory

# Bug99 file =+ memory target
Memory :  HCD.ea5 Page0.ea5

# Bug99 file =+ memory image 9900; Capture = HCD.cpt
HCD.ea5 :  HCD.spt HC.o Stdmem.o Stub.o OHCI.o Root.o Usbd.o
	 $(Link99) -s HCD.spt

# Bug99 file =+ script 9900
HCD.spt : 

# Bug99 file =+ object 9900
HC.o :  HC.s
	 $(As99) HC.s -ti -o HC.o

# Bug99 file =+ source 9900
HC.s :  Equates.s

# Bug99 file =+ source 9900
Equates.s : 

# Bug99 file =+ object 9900
Stdmem.o :  stdmem.s
	 $(As99) stdmem.s -ti -o Stdmem.o

# Bug99 file =+ source 9900
stdmem.s : 

# Bug99 file =+ object 9900
Stub.o :  Stub.s
	 $(As99) Stub.s -ti -o Stub.o

# Bug99 file =+ source 9900
Stub.s : 

# Bug99 file =+ object 9900; Capture = OHCI.cpt
OHCI.o :  ohci.s
	 $(As99) ohci.s -ti -o OHCI.o

# Bug99 file =+ source 9900
ohci.s :  HCdata.s Equates.s

# Bug99 file =+ source 9900
HCdata.s : 

# Bug99 file =+ source 9900
Equates.s : 

# Bug99 file =+ object 9900; Capture = Root.cpt
Root.o :  root.s
	 $(As99) root.s -ti -o Root.o

# Bug99 file =+ source 9900
root.s :  Equates.s HCdata.s

# Bug99 file =+ source 9900
Equates.s : 

# Bug99 file =+ source 9900
HCdata.s : 

# Bug99 file =+ object 9900
Usbd.o :  usbd.s
	 $(As99) usbd.s -ti -o Usbd.o

# Bug99 file =+ source 9900
usbd.s :  Equates.s HCdata.s

# Bug99 file =+ source 9900
Equates.s : 

# Bug99 file =+ source 9900
HCdata.s : 

# Bug99 file =+ memory image 9900
Page0.ea5 :  Stub0.o Page0.o
	 $(Link99) -l Stub0.o,Page0.o -o Page0.ea5

# Bug99 file =+ object 9900
Stub0.o :  Stub0.s
	 $(As99) Stub0.s -ti -o Stub0.o

# Bug99 file =+ source 9900
Stub0.s : 

# Bug99 file =+ object 9900
Page0.o :  Page0.s
	 $(As99) Page0.s -ti -o Page0.o

# Bug99 file =+ source 9900
Page0.s : 

# Bug99 --------------- End of building instructions ---------------
# Bug99 --------------- Memory models ------------------------------
# Model MODEL USBSM

# USBSM DEVICE EPROM

# EPROM BLOCK 4000,4FFF,0000,0010

# Bug99 --------------- End of memory models -----------------------
