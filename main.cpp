#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <malloc.h>
//#include "objectfile.h"
#include "scriptfile.h"
#include "list.h"

/*  LINKER STRATEGY

- First pass through script. Grab memory model.Grab library files.
- First pass through DF80 files. Grab segment definitions and labels.
- First pass through libraries, if needed.
- Dispatch segments to blocks/pages.
- Process memorized commands 
- Second pass through DF80 files. Load code into segment buffers, patch REF chains.
- Second pass through libraries. Load required code as above.
- Ouput memory-image files.

*/
/////////////////////////////////////////////////////////////////////////////////////
// Main program entry point
/////////////////////////////////////////////////////////////////////////////////////
int g_Verbous=2;	// 0: only error messages 1: errors + warnings 2: main comments 3: detailed info 4: debug info
int g_ErrLevel=1;	// 0: no error 1: stops only at errors 2: stops at warnings
int g_Log;			// log what: 0=nothing, 1=output, 2=errors
ofstream g_Logfile;	// log file
											
main(int argc, char *argv[])
{
	int i,j,ptr;
	CScriptfile Script;
	char Line[257];

	/*							TODO: THISPG not fixed with Bug99 script
	char s0[]="HCD.spt";
	char s1[]="-s";
	char s2[]="Page3.spt";
	char s3[]="-l";
	char s4[]="stdmem.o,stub3.o";
	char s5[]="-o";
	char s6[]="test.ea5,EA5,USBROM,USBRAM";
	//argv[1]=s1;											
	//argv[2]=s2;
	//argv[3]=s3;
	//argv[4]=s4;
	//argv[5]=s5;
	//argv[6]=s6;

	argc=2;
	argv[1]=s0;
*/
	if(argc<2)											// no argument: command mode
	{
		Line[0]=' ';									// blank label field
		cout << "Link99:";								// display prompt
		while(1)
		{
			cin.getline(Line+1,255);					// grab one line
			if((strcmp(Line+1,"abort")==0)||(strcmp(Line+1,"ABORT")==0))
				return 0;								// exit command mode without processing
			else if((strcmp(Line+1,"exit")==0)||(strcmp(Line+1,"EXIT")==0))
			{
				Script.Process();						// process files
				return 0;								// exit command mode
			}
			else
			{
				if(Line[1]=='@')						// user wants a label
					Script.ParseLine(Line+1);			// skips the initial space
				else
					Script.ParseLine(Line);				// begins with space, i.e. no label
			}
			cout << "Link99:";							// display prompt
		}
	}

	else if(argc==2)									// just one filename
	{
		if(Script.Open(argv[1])!=0) return 1;			// use it as script
		if(Script.ParseFile()==0)
			Script.Process();							// go on if no error
	}

	else												// multiple switches
	{
		Line[0]=' ';
		Line[1]=0x00;

		for(i=1;i<argc;i++)								// find script file
		{
			if(strcmp(argv[i],"-s")==0)					// found one					
			{
				if(Script.Open(argv[i+1])!=0) return 1;	// use it as script
				Script.ParseFile();						// parse script contents
				break;									// there can only be one
			}
		}
		for(i=1;i<argc;i++)								// find tagged object files 
		{
			if(strcmp(argv[i],"-l")==0)					// found one					
			{
				i++;
				for(ptr=0;;)
				{
					strcpy(Line," LOAD \"");			// generate pseudo script line
					j=Script.GetParam(Line+7,80,argv[i],&ptr,',');
					strcat(Line,"\"");
					if(j<=0) break;
					Script.ParseLine(Line);				// parse it
				}
			}
		}
		for(i=1;i<argc;i++)								// find library files 
		{
			if(strcmp(argv[i],"-L")==0)					// found one					
			{
				i++;
				for(ptr=0;;)
				{
					strcpy(Line," LIRARY \"");			// generate pseudo script line
					j=Script.GetParam(Line+9,80,argv[i],&ptr,',');
					strcat(Line,"\"");
					if(j<=0) break;
					Script.ParseLine(Line);				// parse it
				}
			}
		}
		for(i=1;i<argc;i++)								// find output files 
		{
			if(strcmp(argv[i],"-o")==0)					// found one					
			{
				i++;
				strcpy(Line," OUTPUT \"");
				for(j=0;argv[i][j]!=0x00;j++)			// must insert double quotes around file name
				{
					if(argv[i][j]!=',') continue;		// first the first comma
					argv[i][j]=0x00;					// end string here
					strcat(Line+9,argv[i]);				// append it to command
					strcat(Line+9+j,"\",");				// insert double quotes and comma
					strcat(Line+10+j,argv[i]+j+1);		// append the rest of the string
					break;
				}
				Script.ParseLine(Line);
			}
		}	
		for(i=1;i<argc;i++)								// find memory model 
		{
			if(strcmp(argv[i],"-m")==0)					// found one					
			{
				strcpy(Line," SELECT ");				// generate pseudo script line
				strcat(Line,argv[i+1]);					// append model name (must be a model)
				Script.ParseLine(Line);					// parse it
				i++;
			}
		}

		Script.Process();								// finish processing
	}
	return 0;
}


/*  LINKER SCRIPT SYNTAX 
========================
# any line starting with # is considered a comment line
Any line not starting with a space has a label in column 1 (no @ in script)

Main commands
-------------
MODEL   list                            Declare a memory model (optional name=label), with a list of devices/blocks
DEVICE  list                            Declare a memory device (optional name=label), with a list of blocks
BLOCK   memtype,start-end,fistpage-lastpage,step   Declare a memory block (optional name=label)
BLOCK   memtype,start,size,fistpage,nbpages,step   Alternative syntax for ranges
SELECT  list                            Select a memory model/device/block. Can also provide a list of such items (wildcards ok). ! = erase esisting list

LOAD filename                           Declare a DF80 file for loading
LIBRARY filename                        Declare a library of DF80 files to be used if needed
PATH path                               Default file path, if not that of linker script
LIBPATH path                            Default path for library files (can combine several with ; separator)

OUTPUT "filename",format,label_list     Output a memory image file, format+ ea5|gk|fb6, label_list=comma-separeted list of blocks, pages or segments (incl. wildcards)
OUTPUT "filename",format,$AORG          Output a memory image file for AORG stretches. Formats: EA5 GK FB6 or BIN

SEGMENT name,flags                      Emulate segments assemblers that don't support them. Type=SCEG,DSEG,ESEG
SEGEND                                  End of segment declared with SEGMENT
SEGFLAG segment,flagson,flagsoff        Change segment flags (even if declared within DF80 file) Segment name can contain wildcards

STRATEGY                                Dispatching strategy: FIRST or LARGEST or list of segments (wildcards allowed) ! = erase existing list
ASSIGN  segment,block,page,address      Manually assign a segment to a block/page/address page can be 0 for any page
STUB block,segmentlist                  Assign segments to a block's stub (segment names can contain wildcards)
OVERLAY block,segmentlist               Reserve a block for overlay and assign segments to it (segment names can contain wildcards)

MINGAP value                            Specifie how many missing bytes will cause a file to be split in two
MAXSIZE value                           Maximum memimage file size (not counting header)
EQUATE label,value                      Define/modifie an internal label
DEFINE label,value,tag,segnb            Create/redefine a DEF label (-1 or omitted value = no change)
ERRORS value                            Error level to abort 0: never, 1: abort on errors 2: abort on warnings (default = 1)

Listing commands
----------------
VERBOUS value                           Verbousness level 0: only error messages 1: errors + warnings 2: main comments 3: detailed info 4: debug info
LOGFILE filename,options                Log file name, options: I = infos E = errors&warnings
SEGTABLE pass,options                   Output segment table. Pass: 1 or 2.
SYMTABLE pass,options                   Output symbol table. Options: D=def R=ref S=sref (default=all)
MEMTABLE pass,options                   Output memory table. Options: m=model d=device b=block

Advanced commands
-----------------
VERIFY segment,offset,data,data,...	    Verify that data at offset in segment equals a list of values (? = skip)
PATCH segment,offset,data,data,...      Modify data at offset in segment with list of values (? = skip) Disabled if previous verify false
PROCESS                                 End-of-script processing: fix symbols, search libraries, dispatch, load code, generate files
MATCHREFS                               Match REF labels with a DEF in symbol table 
NBUNDEF	type                            Count undef REFs and/or SREFs type=S: do SREF (default = no)
SEARCHLIB                               Search libraries, decide which segment to include
DISPATCH wipe                           Dispatch remaining segments to memory block (remove existing if requested)
FIXTAB                                  Fix symbol table: REF values, relative offsets
GETCODE	type                            Load code into memory, fix REF chains. Type: O=object files, L=libraries (default=both)
GENERATE                                Generate memimage files

SCRIPT "scriptfile"                     Nested script file
JOB	"scriptfile"                        External script file

FIND type,name,value					Type:S LD LR LU LE LA MM MD MB MP, name (optional), value=occurence (page# for MP). Results placed in Equates

Script-related commands
-----------------------
ECHO                                    Print something on screen "text" labelnames (separated with spaces) \t and \nl accepted (out of quotes)
IF                                      Conditional processing (&& and || accepted)
ELSE                                    Reverse the condition
ENDIF                                   End of condition
ELIF                                    Alternative condition (else if)
STOP                                    End script before end-of-file, do processing
IDLE                                    End script file without processing

Future commands (maybe)
------------------------
GETKEY
GETVALUE
GOTO
COMMAND
ENTRY address							Speficy the entry point of the program
INCREMENTAL filename					Specify how to modify filenames when splitting a file
GETDEF filename							Get label DEFs from a DF80 file, but don't load it
NEWBLOCK								Causesthe linker to skip to the next memory block
REBUILD [all]							Override incremental linking (i.e. don't check timestamps on files)
PG4SEG									Verify the PG4SEG mechanism

Predefined equates
------------------
ROM,RAM,NVRAM,OVL,$PSEG,$DSEG,$CSEG,$BLANK	Segment flags
$TYPE,$SIZE,$ADDR,$PAGE,$COUNT			Results of FIND command


Command line switches
=====================
-s filename                             Process script
-l filename_list                        Load file or coma-separeted list of files. Better used before -script
-L filename-liste                       List library files. Better used after -script
-o filename,type,segmentlist			Output file, as inside script. Better used after script
-m blockname							Select memory model/device/block. Better used after script


*/


