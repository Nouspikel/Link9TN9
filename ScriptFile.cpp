#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <malloc.h>
#include "ScriptFile.h"
#include "Memimage.h"
#include "Patch.h"
#include <sys/types.h>
#include <sys/stat.h>

extern g_Verbous;
extern g_ErrLevel;
extern ofstream g_Logfile;
extern int g_Log;

/////////////////////////////////////////////////////////////////////////////////////
// CScriptfile class. Handles the script file
/////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------------
CScriptfile::CScriptfile()
{
	m_Equates.Append("ROM",0x1000);												// predefined flags
	m_Equates.Append("RAM",0x2000);
	m_Equates.Append("NVRAM",0x4000);
	m_Equates.Append("OVL",0x8000);
	m_Equates.Append("M_CPU",0);
	m_Equates.Append("M_GRAM",1);
	m_Equates.Append("M_VDP",2);
	m_Equates.Append("$PSEG",0x00010000);
	m_Equates.Append("$DSEG",0x00020000);
	m_Equates.Append("$CSEG",0x00040000);
	m_Equates.Append("$BLANK",0x00080000);
	m_Equates.Append("$SIZE",0);
	m_Equates.Append("$TYPE",0);
	m_Equates.Append("$ADDR",0);
	m_Equates.Append("$PAGE",0);
	m_Equates.Append("$COUNT",0);
	strcpy(m_PsegName,"$PSEG");
	m_Tables[0]=0x00;
	m_MaxSize=0x2000;
	m_MinGap=0512;
	m_Path[0]=0x00;
	m_LibPath[0]=0x00;
	m_Strategy=0;
	g_Log=0;
}

//----------------------------------------------------------------------------------
// Constructor, with file name
//----------------------------------------------------------------------------------
CScriptfile::CScriptfile(char* filename)
{
	m_File.open(filename,ios::nocreate);
	if(!m_File)
	{
		cout << "Cannot open script file:" << filename << "\n";
		if(g_Log&0x02) g_Logfile << "Cannot open script file:" << filename << "\n";
		return;
	}
	m_Equates.Append("ROM",0x0001);												// predefined flags
	m_Equates.Append("RAM",0x0002);
	m_Equates.Append("NVRAM",0x0004);
	m_Equates.Append("OVL",0x0008);
	m_Equates.Append("$PSEG",0x00010000);
	m_Equates.Append("$DSEG",0x00020000);
	m_Equates.Append("$CSEG",0x00040000);
	m_Equates.Append("$BLANK",0x00080000);
	m_Equates.Append("$SIZE",0);
	m_Equates.Append("$TYPE",0);
	m_Equates.Append("$ADDR",0);
	m_Equates.Append("$PAGE",0);
	m_Equates.Append("$COUNT",0);
	strcpy(m_PsegName,"$PSEG");
	m_Tables[0]=0x00;
	m_MaxSize=0x2000;
	m_MinGap=0512;
	m_Path[0]=0x00;
	m_LibPath[0]=0x00;
	g_Log=0;
}


//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CScriptfile::~CScriptfile()
{
	char *pChar;
	int i;

	for(i=0;i<m_Searches.GetSize();i++)
	{
		pChar=(char*)m_Searches.GetAt(i);
		free(pChar);
	}
}

//----------------------------------------------------------------------------------
// Opens the file (if created with default constructor)
//----------------------------------------------------------------------------------
int CScriptfile::Open(char* filename)
{
	m_File.open(filename,ios::nocreate);
	if(!m_File)
	{
		cout << "Cannot open script file:" << filename << "\n";
		if(g_Log&0x02) g_Logfile << "Cannot open script file:" << filename << "\n";
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------------
// Parse a command line
//----------------------------------------------------------------------------------
int CScriptfile::ParseLine(char* pLine)
{
	char Label[9];
	char Command[81];
	char Name[81];
	char Ftype[4];
	char* pChr;
	int Address=0,ToAddr=0,Page=0,ToPage=0,Step,Memtype=0,i,j,Type,ptr=0,Segout,on,off,Value;
	CObjectfile *pObject=NULL;
	CBlock *pBlock, *pChild;
	CSegment *pSegment;
	CMemimage *pMemimage;
	CObjectfile *pObjectfile;
	CLibrary *pLibrary;
	CPatch* pPatch;
	ifstream File2;
	CScriptfile* pScript;
	struct _stat buf;

	Label[0]=0x00;
	if(pLine[ptr]=='@')	ptr++;													// @LABEL syntax, skip the @ sign
	if(pLine[ptr]=='#') return 0;												// comment line
	if(pLine[ptr] > ' ') GetLabel(Label,8,pLine,&ptr);							// grab label

	if(pLine[ptr]=='#') return 0;												// the rest is a comment
	GetCommand(Command,10,pLine,&ptr);											// grab command
	if(Command[0]==0x00) return 0;												// empty command
	_strupr(Command);															// force uppercase
	if(g_Verbous>=4) 
	{
		cout << "Command: "<< pLine << "\n";
		if(g_Log&0x01)
		{
			g_Logfile << "Command: "<< pLine << "\n";
		}
	}

//	LOAD or GETDEF
//	--------------
	if((strcmp(Command,"LOAD")==0)||(strcmp(Command,"GETDEF")==0))			// load tagged object file
	{
		GetQuotedstring(Name,80,pLine,&ptr);
		if((Value=_stat(Name,&buf))!=0)											// check if file exists
		{																		// it does not
			for(j=i=0;(i<256)&&(j<80);i++)										// search the object file path
			{
				if((m_Path[i]==';')||(m_Path[i]==0x00))							// end of one path
				{
					if(i==0) break;												// not path defined
					if(Command[j-1]!='/') Command[j++]='/';						// add trailing / if not there
					Command[j]=0x00;											// add end-of-string mark
					strcat(Command,Name);										// append file name
					if((Value=_stat(Command,&buf))==0) break;					// check if exists
					if((m_Path[i]==0x00)||(m_Path[i+1]==0x00)) break;			// no more
					j=0;														// not found, start over with next path
					continue;
				}
				Command[j++]=m_Path[i];											// copy path to buffer
			}
		}
		if(Value!=0)															// found?
		{
			cout << "Object file " << Name << " not found\n";					// no: issue error
			if(g_Log&0x02) 
				g_Logfile << "Object file " << Name << " not found\n";
			return g_ErrLevel;
		}
		pObjectfile=new CObjectfile(Name);										// create file object
		if(strcmp(Command,"LOAD")==0)										// LOAD command
		{
			m_Objectfiles.Add((PTR)pObjectfile);									// add it to lists
			pObjectfile->FirstPass(&m_Segments,&m_Defs,&m_Refs,&m_Aorgs,m_PsegName);	// first pass on this file
		}
		else																	// GETDEF command
		{
			pObjectfile->FirstPass(&m_Segments,&m_Defs,&m_Refs,&m_Aorgs,m_PsegName,1);	// first pass on this file
			delete pObjectfile;													// not needed any more
		}
	}


//	BLOCK
//	-----
	else if(strcmp(Command,"BLOCK")==0)										// define memory block
	{
		Address=GetMath(pLine,&ptr,&m_Equates,&m_Defs);
		if(pLine[ptr]==',')														// address,size syntax
		{
			ptr++;																// skipt the comma
			ToAddr=Address+GetMath(pLine,&ptr,&m_Equates,&m_Defs)-1;			// calculate end address
		}
		else if(pLine[ptr]==':')
		{
			ptr++;																// skipt the : sign
			ToAddr=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// from:to syntax
		}
		if(pLine[ptr]==',')	
		{
			ptr++;																// skipt the comma
			Page=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							
		}
		if(pLine[ptr]==',')														// page,number syntax
		{
			ptr++;																// skipt the comma
			ToPage=Page+GetMath(pLine,&ptr,&m_Equates,&m_Defs)-1;	
		}
		else if(pLine[ptr]==':')
		{
			ptr++;																// skipt the : sign
			ToPage=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// first:last syntax
		}

		Step=0;
		if(pLine[ptr]==',')														
			Step=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// page number increment
		if(Step==0)																// not provided
		{
			if(Page<=ToPage) Step=1;											// set default
			else Step=-1;														// negative if order inverted
		}

		if(pLine[ptr]==',')														// memory type
		{
			ptr++;																// skipt the : sign
			Memtype=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// first:last syntax
		}


		if((pBlock=m_Blocks.Find(Label,BT_NONE))!=NULL)							// see if previously referred to
		{
			pBlock->Define(Address,ToAddr,Page,ToPage,Step,Memtype);			// yes: finish block definition
		}
		else if(m_Blocks.Find(Label,BT_BLOCK)!=NULL)							// see if already defined
		{
			cout << "Block " << Name << " defined twice\n";						// yes: issue error				
			if(g_Log&0x02) g_Logfile << "Block " << Name << " defined twice\n";
			return g_ErrLevel;
		}
		else
		{
			pBlock=m_Blocks.AddBlock(Label,Address,ToAddr,Page,ToPage,Step,Memtype);	// create new block
		}
		if((pLine[ptr]==',')&&(pBlock))
		{
			pBlock->m_Condition=_strdup(pLine+ptr+1);							// copy rest of line into block's condition
		}
	}


//	DEVICE
//	------
	else if(strcmp(Command,"DEVICE")==0)										// define memory device
	{
		if((pBlock=m_Blocks.Find(Label,BT_NONE))!=NULL)							// see if previously referred to
			pBlock->m_Type=BT_DEVICE;											// yes: make it a device
		else
			pBlock=m_Blocks.AddDevice(Label,BT_DEVICE);							// no: make a new device
		if(pBlock==NULL) return g_ErrLevel;												// error

		while(GetParam(Label,6,pLine,&ptr,',')>0)								// parse list of block names
		{
			pChild=m_Blocks.Find(Label,BT_BLOCK);								// check if exists
			if(pChild==NULL)
				pChild=m_Blocks.AddBlock(Label,-1,-1,-1,-1,-1);					// no: create empty block (to be filled when declared)
			if(pChild!=NULL)
				pBlock->m_Children.Add((PTR)pChild);							// add pointer to device's list of blocks
		}																		// next block
	}


//	MODEL
//	-----
	else if(strcmp(Command,"MODEL")==0)										// define memory model
	{
		pBlock=m_Blocks.AddDevice(Label,BT_MODEL);								// add it to chain
		if(pBlock==NULL) return g_ErrLevel;										// error

		while(GetParam(Label,6,pLine,&ptr,',')>0)								// parse list of block names
		{
			pChild=m_Blocks.Find(Label,BT_DEVICE);								// check if a device with this name exists
			if(pChild==NULL) pChild=m_Blocks.Find(Label,BT_BLOCK);				// or else a block

			if(pChild==NULL)													// none found
				pChild=m_Blocks.AddBlock(Label,-1,-1,-1,-1,-1);					// create empty block/device (to be filled when declared)
			if(pChild!=NULL)
				pBlock->m_Children.Add((PTR)pChild);							// add pointer to device's list of blocks
		}																		// next block
	}


//	SELECT
//	------
	else if(strcmp(Command,"SELECT")==0)										// select blocks to be used
	{
		if(pLine[ptr]=='!')														// wipe existing list
		{
			m_Selected.RemoveAll(0);											// empty it
			ptr++;																// skip ! mark
		}

		while(1)																// parse list of block names
		{
			while((pLine[ptr]==',')||(pLine[ptr]==' ')||(pLine[ptr]=='\t'))ptr++;	// skip commas and spaces
			if(pLine[ptr]=='{')													// start horizontal section
			{
				m_Selected.Add((PTR)0x0001);									// insert special code in block list
				ptr++;															// skip { sign
				continue;														// keep scanning
			}
			else if(pLine[ptr]=='}')											// end horizontal section
			{
				m_Selected.Add((PTR)0x0002);									// insert special code in block list
				ptr++;															// skip } sign
				continue;														// keep scanning
			}
			else if(pLine[ptr]=='^')											// force vertical block inside horizontal section
			{
				m_Selected.Add((PTR)0x0003);									// insert special code in block list
				ptr++;															// skip ^ sign, expect block name next
			}

			if(GetParam(Label,6,pLine,&ptr,',')<=0) break;						// get block name, exit loop if no more
			pChr=strchr(Label,'}');												// in case of NAME} syntax
			if(pChr!=NULL)						
			{
				*pChr=0x00;														// truncate name
				while(pLine[--ptr]!='}');										// point back at } sign in input line
			}
			if(HasWildcards(Label))												// see if name is a pattern with wildcards
			{
				for(i=0;i<m_Blocks.m_Blocks.GetSize();i++)						// iterate all blocks
				{
					pChild=(CBlock*)m_Blocks.m_Blocks.GetAt(i);					// get one
					if(!Match(pChild->m_Name,Label)) continue;					// name doesn't match: next
					m_Selected.Add((PTR)pChild);								// add block to list
				}
				continue;														// next name
			}
			pChild=m_Blocks.Find(Label);										// check if exists (no wildcard)
			if(pChild==NULL)													// not found: error
			{
				if(g_Verbous>0)
				{
					cout << "Block/device/model " << Label << " not found for select\n";
					if(g_Log&0x02) g_Logfile << "WBlock/device/model " << Label << " not found for select\n";
				}
				return g_ErrLevel;
			}
			else
				m_Selected.Add((PTR)pChild);									// add block pointer to list of selected blocks
		}																		// next block
	}

//	STRATEGY
//	--------
	else if(strcmp(Command,"STRATEGY")==0)									// segment iteration strategy
	{
		i=ptr;
		if(GetLabel(Label,8,pLine,&ptr)==i) return 0;							// get strategy name
		if(strcmp(Label,"FIRST")==0) m_Strategy=0;
		else if(strcmp(Label,"LAST")==0) m_Strategy=-1;
		else if(strcmp(Label,"LARGEST")==0) m_Strategy=-2;
		else if(strcmp(Label,"SMALLEST")==0) m_Strategy=-3;
		else if(strcmp(Label,"FLAGVAL")==0)		
		{
			m_Strategy=GetMath(pLine,&ptr,&m_Equates,&m_Defs);					// get flag mask 
			if((m_Strategy>=-3)&&(m_Strategy<=0))
			{
				if(g_Verbous>0)
				{
					cout << "Warning: Illagal flag mask for STRATEGY (-3 to 0 reserved)\n";
					if(g_Log&0x02) g_Logfile << "Warning: Illagal flag mask for STRATEGY (-3 to 0 reserved)\n";
				}
				if(g_ErrLevel>1) return g_ErrLevel;
			}
		}
		else ptr=i;																// begins with segment name
		
		if(pLine[ptr]=='!')														// wipe existing list
		{
			m_SegList.RemoveAll(0);												// empty it
			ptr++;																// skip ! mark
		}

		while(1)																// more
		{
			while((pLine[ptr]==',')||(pLine[ptr]==' ')||(pLine[ptr]=='\t'))ptr++;	// skip commas and spaces
			if(GetParam(Label,8,pLine,&ptr,',')<=0) break;						// get segment name, break if no more
			if(HasWildcards(Label))												// see if name is a pattern with wildcards
			{
				for(i=0;i<m_Segments.GetSize();i++)								// iterate all segments
				{
					pSegment=m_Segments.GetAt(i);								// get one
					if(!Match(pSegment->m_Name,Label)) continue;				// name doesn't match: next
					m_SegList.Add((PTR)pSegment);								// add segment to list
				}
				continue;														// next name
			}
			if((pSegment=m_Segments.Find(Label))==0)
			{
				if(g_Verbous>0)
				{
					cout << "Warning: Segment " << Label << " not found for strategy\n";
					if(g_Log&0x02) g_Logfile << "Warning: Segment " << Label << " not found for strategy\n";
				}
				if(g_ErrLevel>1) return g_ErrLevel;
			}
			else 
				m_SegList.Add((PTR)pSegment);									// add to selection list
		}
	}

//	LIBRARY
//	-------
	else if(strcmp(Command,"LIBRARY")==0)									// remember library files
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// grab name
		if((Value=_stat(Name,&buf))!=0)											// check if file exists
		{																		// it does not
			for(j=i=0;(i<256)&&(j<80);i++)										// search the library path
			{
				if((m_LibPath[i]==';')||(m_LibPath[i]==0x00))					// end of one path
				{
					if(i==0) break;												// not path defined
					if(Command[j-1]!='/') Command[j++]='/';						// add trailing / if not there
					Command[j]=0x00;											// add end-of-string mark
					strcat(Command,Name);										// append file name
					if((Value=_stat(Command,&buf))==0) break;					// check if exists
					if((m_LibPath[i]==0x00)||(m_LibPath[i+1]==0x00)) break;		// no more
					j=0;														// not found, start over with next path
					continue;
				}
				Command[j++]=m_LibPath[i];										// copy path to buffer
			}
		}
		if(Value!=0)															// found?
		{
			if(g_Verbous>0)
			{
				cout << "Warning: Library " << Name << " not found\n";
				if(g_Log&0x02) g_Logfile << "Warning: Library " << Name << " not found\n";
			}
			return g_ErrLevel;													// abort, but not necessarily with error
		}
		pLibrary=new CLibrary(Name);											// file found: create file object
		m_Libraries.Add((PTR)pLibrary);											// add it to lists
	}


//	OUTPUT
//	------
	else if(strcmp(Command,"OUTPUT")==0)										// remember output files
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// get file name
		Type=1;																	// default
		if(GetParam(Label,4,pLine,&ptr,',')>0)									// get file type
		{
			if(strcmp(Label,"EA5")) Type=1;										// parse file types
			else if(strcmp(Label,"GK")) Type=2;
			else if(strcmp(Label,"FB6")) Type=3;
			else if(strcmp(Label,"BIN")) Type=4;
			else Type=0;
		}

		pMemimage = new CMemimage(Name,Type);									// create file object
		m_Memimages.Add((PTR)pMemimage);										// add it to list

		pMemimage->m_MaxSize=m_MaxSize;											// set maximum data size
		Segout=0;																// output blocks or segments
		while(pLine[ptr]==',')													// get list of blocks to output
		{
			if(GetParam(Label,8,pLine,&ptr,',')<=0) break;						// get a name
			switch(Segout)														// type of output
			{
			case 0:																// outputing blocks or segments (don't know yet)
				if(strcmp(Label,"$AORG")==0)
				{
					pMemimage->m_pAorgs=&m_Aorgs.m_Aorgs;						// pass AORG list to file object
					Segout=3;													// accept nothing else
				}
				pBlock=m_Blocks.Find(Label);									// try block first
				if(pBlock!=NULL)												// found
				{
					pMemimage->m_Blocks.Add((PTR)pBlock);						// add block to list
					Segout=1;													// from now on, force blocks
				}
				else															// block found
				{
					if(HasWildcards(Label))										// see if name is a pattern with wildcards
					{
						for(i=0;i<m_Segments.GetSize();i++)						// iterate all segments
						{
							pSegment=m_Segments.GetAt(i);						// get one
							if(!Match(pSegment->m_Name,Label)) continue;		// name doesn't match: next
							pMemimage->m_Segments.Add((PTR)pSegment);			// add segment to list
							Segout=2;											// from now on, force segments
						}
					}
					else														// regular name provided
					{
						pSegment=m_Segments.Find(Label);						// find it
						if(pSegment==NULL)										// not found
						{
							if(g_Verbous>0)
							{
								cout << "Warning: segment " << Label << " unknown for OUTPUT " << Name << "\n";
								if(g_Log&0x02) g_Logfile << "Warning: segment " << Label << " unknown for OUTPUT " << Name << "\n";
							}
							if(g_ErrLevel>1) return g_ErrLevel;
						}
						else
						{
							pMemimage->m_Segments.Add((PTR)pSegment);			// add segment to list
							Segout=2;											// from now on, force segments
						}
					}
				}
				break;
			case 1:																// outputing blocks
				pBlock=m_Blocks.Find(Label);									// find it
				if(pBlock==NULL)												// not found
				{
					if(g_Verbous>0)
					{
						cout << "Block " << Label << " unknown for OUTPUT " << Name << "\n";
						if(g_Log&0x02) g_Logfile << "Block " << Label << " unknown for OUTPUT " << Name << "\n";
					}
					return g_ErrLevel;
				}
				else
				{
					pMemimage->m_Blocks.Add((PTR)pBlock);						// add block to list
					Segout=1;													// from now on, force blocks
				}
				break;
			case 2:																// outputing segments
				if(HasWildcards(Label))											// see if name is a pattern with wildcards
				{
					for(i=0;i<m_Segments.GetSize();i++)							// iterate all segments
					{
						pSegment=m_Segments.GetAt(i);							// get one
						if(!Match(pSegment->m_Name,Label)) continue;			// name doesn't match: next
						pMemimage->m_Segments.Add((PTR)pSegment);				// add segment to list
					}
				}
				else															// regular name provided
				{
					pSegment=m_Segments.Find(Label);							// find it
					if((pSegment==NULL)&&(Segout==2))							// not found
					{
						if(g_Verbous>0)
						{
							cout << "Warning: segment " << Label << " unknown for OUTPUT " << Name << "\n";
							if(g_Log&02) g_Logfile << "Warning: segment " << Label << " unknown for OUTPUT " << Name << "\n";
						}
						g_ErrLevel;
					}
					else
					{
						pMemimage->m_Segments.Add((PTR)pSegment);				// add segment to list
					}
				}
					break;
			} // switch
		} // while
	}

//	LOGFILE
//	-------
	else if(strcmp(Command,"LOGFILE")==0)									// load tagged object file
	{
		GetQuotedstring(Name,80,pLine,&ptr);
		g_Logfile.open(Name);
		if(g_Logfile.rdstate()&ios::failbit)
		{
			if(g_Verbous>0) cout << "Warning: Cannot open logfile " << Name << "\n";
			if(g_ErrLevel>1) return 1;
			else return 0;
		}
		else
		{
			g_Log=0x03;															// log errors (0x02) and info (0x01)
			if(pLine[ptr]==',')													// flags provided
			{
				g_Log=0;														// forget default
				GetParam(Name,8,pLine,&ptr,',');								// get flags
				if((strchr(Name,'E'))||(strchr(Name,'e'))) g_Log|=0x02;			// log errors & warnings
				if((strchr(Name,'I'))||(strchr(Name,'i'))) g_Log|=0x01;			// log infos
			}
		}
	}


//	STUB
//	----
	else if(strcmp(Command,"STUB")==0)										// declare stub segments
	{
		if(GetParam(Name,8,pLine,&ptr,',')<=0) return 0;						// get memory block
		if((pBlock=m_Blocks.Find(Name,BT_BLOCK))==NULL)
		{																		// error if not found
			if(g_Verbous>0)														// stub will not be found if it's in a library
			{
				cout << "Warning: Block " << Name << " not found for STUB command\n"; 
				if(g_Log&0x02) g_Logfile << "Warning: Block " << Name << " not found for STUB command\n";
			}
			if(g_ErrLevel>1) return 1;
			else return 0;
		}

		while(pLine[ptr]==',')													// process list of segments
		{
			if(GetParam(Label,8,pLine,&ptr,',')<=0) break;						// get segment name
			if(HasWildcards(Label))												// see if name is a pattern with wildcards
			{
				for(i=0;i<m_Segments.GetSize();i++)								// iterate all segments
				{
					pSegment=m_Segments.GetAt(i);								// get one
					if(!Match(pSegment->m_Name,Label)) continue;				// name doesn't match: next
					if(pBlock->AssignStub(pSegment)>0)	return g_ErrLevel;		// name matches: assign it to block
				}
			}
			else																// regular name provided
			{
				if((pSegment=m_Segments.Find(Label))==NULL)
				{
					if(g_Verbous>0)
					{
						cout << "Warning: Segment " << Label << " not found for STUB command\n";
						if(g_Log&0x02) g_Logfile << "Warning: Segment " << Label << " not found for STUB command\n";
					}
					if(g_ErrLevel>1) return 1;
					else return 0;
				}
				if(pBlock->AssignStub(pSegment)>0) return g_ErrLevel;
			}
		}
	}

//	MINGAP
//	------
	else if(strcmp(Command,"MINGAP")==0)										// minimum gap size
	{
		m_MinGap=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// get value 
	}

//	MAXSIZE
//	-------
	else if(strcmp(Command,"MAXSIZE")==0)									// maximum file size (without header)
	{
		m_MaxSize=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// get value 
	}

//	EQUATE
//	------
	else if(strcmp(Command,"EQUATE")==0)										// internal label definition
	{
		if(GetParam(Label,8,pLine,&ptr,',')<=0) return 0;						// get label name
		Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// get (new) value 
		m_Equates.SetValue(Label,Value,'E',0,0);								// get or create an equate label
	}

	
//	DEFINE
//	-----
	else if(strcmp(Command,"DEFINE")==0)										// DEF label (re)definition
	{
		if(GetParam(Label,8,pLine,&ptr,',')<=0) return 0;						// get label name
		Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// get (new) value 
		i=-1;
		Name[0]=(char)0xFF;
		if(pLine[ptr]==',')														// optional: get tag and segment number
		{
			GetParam(Name,1,pLine,&ptr,',');									// get type tag
			if(pLine[ptr]==',')
				i=GetMath(pLine,&ptr,&m_Equates,&m_Defs);						// optional get segment number
		}
		m_Defs.SetValue(Label,Value,Name[0],i,0);								// get or create a DEF label
	}


//	SEGTAB
//	------
	else if(strcmp(Command,"SEGTAB")==0)										// display segment table
	{
		i=GetMath(pLine,&ptr,&m_Equates);										// get pass number
		if(i==1) m_Segments.OutputTable();										// output table now
		else strcat(m_Tables,"s");												// remember for later
	}

	
//	SYMTAB
//	------
	else if(strcmp(Command,"SYMTAB")==0)										// display symbol table
	{
		if(GetParam(Name,8,pLine,&ptr,',')<=0)									// get flags
			strcpy(Name,"3456VWXYZ");											// nothing means all of them
		if(strchr(Name,'D')) strcat(Name,"56W");								// all DEF types
		if(strchr(Name,'R')) strcat(Name,"34VXYZ");								// all REF types
		if(strchr(Name,'S')) strcat(Name,"VYZ");								// all SREF types
		i=GetMath(pLine,&ptr,&m_Equates);										// get pass number
		if(i==1)
		{
			m_Defs.OutputTable(Name);											// do DEF labels
			m_Refs.OutputTable(Name);											// do REF labels
		}
		else strcat(m_Tables,Name);												// remember for later
	}

	
//	MEMTAB
//	------
	else if(strcmp(Command,"MEMTAB")==0)										// display segment table
	{
		if(GetParam(Name,8,pLine,&ptr,',')<=0)									// get flags
			strcpy(Name,"mdb");													// nothing means all of them
		_strlwr(Name);															// make it lower case
		i=GetMath(pLine,&ptr,&m_Equates);										// get pass number
		if(i==1) m_Blocks.OutputTable(Name);
		else strcat(m_Tables,Name);
	}


//	SEGFLAGS
//	--------
	else if(strcmp(Command,"SEGFLAGS")==0)									// change segment flags
	{
		i=GetParam(Name,8,pLine,&ptr,',');										// get segment name (can have wildcards)

		on=GetMath(pLine,&ptr,&m_Equates,&m_Defs);								// grab flag on value 

		if(pLine[ptr]==',')														// flag off value provided
		{
			off=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// grab flag off value 
		}
		else 
			off=0xFFFFFFFF;														// otherwise, reset all flags

		for(i=0;i<m_Segments.GetSize();i++)										// iterate all segments
		{
			pSegment=m_Segments.GetAt(i);										// get one
			if(!Match(pSegment->m_Name,Name)) continue;							// name doesn't match: next
			pSegment->m_Flags&=~off;											// reset them
			pSegment->m_Flags|=on;												// set requested flags
		}
	}


//	SEGMENT
//	-------
	else if(strcmp(Command,"SEGMENT")==0)									// use PSEG as a distinct segment
	{
		if(GetParam(m_PsegName,8,pLine,&ptr,',')<=0) return 0;					// get segment name for PSEG segment
		if(pLine[ptr]==',')														// flag value provided
		{
			on=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// grab flag value
			pSegment=m_Segments.Append(m_PsegName,0,'M',0xFD);					// must create segment if it does not exist yet
			pSegment->m_Flags=on;												// set flags
		}
	}


//	SEGEND
//	------
	else if(strcmp(Command,"SEGEND")==0)										// change segment flags
	{
		strcpy(m_PsegName,"$PSEG");												// revert to default PSEG
	}

	
//	GHOST
//	-----
	else if(strcmp(Command,"GHOST")==0)										// declare ghost segments
	{
		j=0;
		do																		// process list of segments
		{
			if(GetParam(Name,8,pLine,&ptr,',')<=0) break;						// get segment name (can have wildcards)

			for(i=0;i<m_Segments.GetSize();i++)									// iterate all segments
			{
				pSegment=m_Segments.GetAt(i);									// get one
				if(!Match(pSegment->m_Name,Name)) continue;						// name doesn't match: next
				pSegment->m_Special|=0x0002;									// set ghost flag
				j++;															// count it
				if(g_Verbous>=4)
				{
					cout << "Ghosting " << pSegment->m_Name << "\n";
					if(g_Log&0x02) g_Logfile << "Ghosting " << pSegment->m_Name << "\n";
				}
			}
		}
		while(pLine[ptr]==',');													// next entry in list
		if((j==0)&&(g_Verbous>0))												// no segment ghosted
		{
			cout << "Warning: No segment found for GHOST\n";
			if(g_Log&0x02) g_Logfile << "Warning: No segment found for GHOST\n";
			if(g_ErrLevel>1) return g_ErrLevel;
		}
	}

//	ASSIGN
//	------
	else if(strcmp(Command,"ASSIGN")==0)										// assign a segment to a block
	{
		GetParam(Label,8,pLine,&ptr,',');										// get segment name
		pSegment=m_Segments.Find(Label);										// find segment
		if(pSegment==NULL)														// not found
		{
			cout << "Segment " << Label << " not found for ASSIGN command\n";
			if(g_Log&0x02) g_Logfile << "Segment " << Label << " not found for ASSIGN command\n";
			return g_ErrLevel;
		}

		if(pLine[ptr]!=',')														// must provide a block name
		{
			if(g_Verbous>0)
			{
				cout << "ASSIGN needs a block name\n";
				if(g_Log&0x02) g_Logfile << "ASSIGN needs a block name\n";
			}
			if(g_ErrLevel>1) return g_ErrLevel;
		}
		GetParam(Name,8,pLine,&ptr,',');										// get block name
		if((pBlock=m_Blocks.Find(Name,BT_BLOCK))==NULL)
		{																		// error if not found
			if(g_Verbous>0)
			{
				cout << "Warning: Block " << Name << " not found for ASSIGN command\n";	
				if(g_Log&0x02) g_Logfile << "Warning: Block " << Name << " not found for ASSIGN command\n";
			}
			if(g_ErrLevel>1) return 1;
		}

		Page=Address=-1;
		if(pLine[ptr]==',')														// page number is optional
		{
			i=ptr;																// to detect empty field
			Page=GetMath(pLine,&ptr,&m_Equates,&m_Defs);
			if((Page==0)&&(ptr==i+1)) Page=-1;									// none provided
		}

		if(pLine[ptr]==',')														// address is optional
		{
			Address=GetMath(pLine,&ptr,&m_Equates,&m_Defs);
		}
		if((i=pBlock->Assign(pSegment,Page,Address))<0)							// assign it to block
		{
			if(i==-2)
			{
				cout << "Failed to assign segment " << Label << " to block " << Name << "\n";
				if(g_Log&0x02) g_Logfile << "Failed to assign segment " << Label << " to block " << Name << "\n";
			}
			return g_ErrLevel;
		}
	}


//	OVERLAY
//	-------
	else if(strcmp(Command,"OVERLAY")==0)									// reserve a block for overlays
	{
		GetParam(Name,8,pLine,&ptr,',');										// get block name
		if((pBlock=m_Blocks.Find(Name,BT_BLOCK))==NULL)
		{																		// error if not found

			if(g_Verbous>0) 
			{
				cout << "Warning: Block " << Name << " not found for OVERLAY command\n"; 
				if(g_Log&0x02) g_Logfile << "Warning: Block " << Name << " not found for OVERLAY command\n"; 
			}
			if(g_ErrLevel>1) return 1;
			else return 0;
		}
		if(pBlock->m_Type!=BT_BLOCK)
		{																		// error if not found
			if(g_Verbous>0)
			{
				cout << "Warning: Models/devices not allowed with OVERLAY command\n";
				if(g_Log&0x02) g_Logfile  << "Warning: Models/devices not allowed with OVERLAY command\n";
			}
			if(g_ErrLevel>1) return 1;
			else return 0;
		}

		pBlock->m_Overlay=1;													// set overlay flag
		while(pLine[ptr]==',')													// segments provided
		{
			GetParam(Label,8,pLine,&ptr,',');									// get segment name
			if(HasWildcards(Label))												// see if name is a pattern with wildcards
			{
				for(i=0;i<m_Segments.GetSize();i++)								// iterate all segments
				{
					pSegment=m_Segments.GetAt(i);								// get one
					if(!Match(pSegment->m_Name,Label)) continue;				// name doesn't match: next
					if(pBlock->Assign(pSegment)<0)								// name matches: assign it to block
					{
						cout << "Cannot assign segment " << Label << " to block " << Name << " \n";
						if(g_Log&0x02) g_Logfile << "Cannot assign segment " << Label << " to block " << Name << " \n";
						return g_ErrLevel;
					}
				}
			}
			else																// regular name provided
			{
				pSegment=m_Segments.Find(Label);								// find this segment
				if(pSegment==NULL)												// not found
				{
					cout << "Segment " << Label << " not found for OVERLAY command\n";
					if(g_Log&0x02) g_Logfile << "Segment " << Label << " not found for OVERLAY command\n";
					return g_ErrLevel;
				}
				if(pBlock->Assign(pSegment)<0)									// assign it to block
				{
					cout << "Cannot assign segment " << Label << " to block " << Name << " \n";
					if(g_Log&0x02) g_Logfile << "Cannot assign segment " << Label << " to block " << Name << " \n";
					return g_ErrLevel;
				}
			}
		}
	}


// DISPATCH
// ---------
	else if(strcmp(Command,"DISPATCH")==0)									// dispatch segments to blocks
	{
		Value=0;
		if(GetParam(Label,8,pLine,&ptr,',')>0)									// get flags											
		{
			if(strstr(Label,"W")) Value|=1;										// wipe existing assignments
			if(strstr(Label,"S")) Value|=2;										// wipe stubs too
			if(strstr(Label,"N")) Value|=4;										// do nothing else
		}

		pBlock=NULL;
		if(GetParam(Name,8,pLine,&ptr,',')>0)									// get device/model name
		{
			pBlock=m_Blocks.Find(Name,BT_DEVICE);
			if(pBlock==NULL)
			{																	// error if not found
				if(g_Verbous>0)													
				{
					cout << "Warning: Device/Model " << Name << " not found for DISPATCH command\n"; 
					if(g_Log&0x02) g_Logfile << "Warning: Device/Model " << Name << " not found for DISPATCH command\n";
				}
				if(g_ErrLevel>1) return 1;
				else return 0;
			}
		}
		Dispatch(pBlock,Value);																	
	}


//	VERIFY
//	------
	else if(strcmp(Command,"VERIFY")==0)										// verify segment data
	{
		if(GetParam(Label,8,pLine,&ptr,',')<=0) return 0;						// get segment name
		Address=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// get address in segment
		Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs);							// get expected value

		pPatch=new CPatch(1,Label,Address,Value);								// create new patch object with first value

		while(pLine[ptr]==',')													// more values can be provided
		{
			if(pLine[++ptr]=='?')												// skip this word
			{
				Value=-1;														// flag: skip this word
				ptr++;															// skip the ? mark
			}
			else
				Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);			// grab value
			pPatch->Append(Value);												// append to list of values
		}
		m_Patches.Add((PTR)pPatch);												// add to list of patches
	}


//	PATCH
//	-----
	else if(strcmp(Command,"PATCH")==0)										// patch segment data
	{
		if(GetParam(Label,8,pLine,&ptr,',')<=0) return 0;						// get segment name
		Address=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);					// get address in segment
		if(pLine[++ptr]=='?')													// skip first word
		{
			Value=-1;															// flag: skip this word
			ptr++;																// skip the ? mark
		}
		else
			Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);				// grab first value

		pPatch=new CPatch(0,Label,Address,Value);								// create new patch object with first value

		while(pLine[ptr]==',')													// more values can be provided
		{
			if(pLine[++ptr]=='?')												// skip this word
			{
				Value=-1;														// flag: skip this word
				ptr++;															// skip the ? mark
			}
			else
				Value=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);			// grab value
			pPatch->Append(Value);												// append to list of values
		}
		m_Patches.Add((PTR)pPatch);												// add to list of patches
	}
	
//	FIND
//	----
	else if(strcmp(Command,"FIND")==0)										// find something
	{
		i=ptr;																	// remember for later
		if(GetParam(Ftype,8,pLine,&ptr)==0)										// grab type of search S LD LR LU LE LA MM MD MB
		{
			if(g_Verbous>0)
			{
				cout << "No type parameter for FIND \n";
				if(g_Log&0x02) g_Logfile << "No type parameter for FIND \n";
			}
			return 2;															// end with warning
		}

		if(Ftype[0]=='2')														// to be done at second pass
		{
			pChr=_strdup(pLine+i);												// copy into new string
			m_Searches.Add((PTR)pChr);											// add it to list
			return 0;
		}
		else return Find(pLine+i);												// perform the search
	}

//	ECHO
//	----
	else if(strcmp(Command,"ECHO")==0)										// display info on screen
	{
		while(ptr<255)															// process entire line
		{
			if(pLine[ptr]==0x00) break;											// done

			if(pLine[ptr]=='"')													// "litteral text"
			{
				GetQuotedstring(Name,80,pLine,&ptr);							// grab text
				cout << Name;													// output it
				if(g_Log&0x01) g_Logfile << Name;								// to log file, if info requested
			}
			else
			{
				i=GetParam(Label,8,pLine,&ptr,',');								// get variable name
				if(i<=0) continue;												// none: ignore
				if(strcmp(Label,"\\nl")||strcmp(Label,"\\NL"))					// newline
				{
					cout << "\n";
					if(g_Log&0x01) g_Logfile << "\n";							
				}
				else if(strcmp(Label,"\\t")||strcmp(Label,"\\T"))				// tab
				{
					cout << "\t";
					if(g_Log&0x01) g_Logfile  << "\t";
				}
				else if((Value=m_Equates.GetValue(Label))>=0)					// see if name exists
				{
					cout << Value;												// output it
					if(g_Log&0x01) g_Logfile << Value;						
				}
				else if((Value=m_Defs.GetValue(Label))>=0)						// search def labels
				{
					cout << Value;												// output it
					if(g_Log&0x01) g_Logfile << Value;						
				}
				else if((Value=m_Refs.GetValue(Label))>=0)						// search ref labels
				{
					cout << Value;												// output it
					if(g_Log&0x01) g_Logfile << Value;						
				}
				else
				{
					cout << "!NOT_FOUND!";
					if(g_Log&0x01) g_Logfile  << "!NOT_FOUND!";
				}
			}
		}

		cout << "\n";															// final end of line
		if(g_Log&0x01) g_Logfile << "\n";
	}

//	STOP
//	----
	else if(strcmp(Command,"STOP")==0)										// end script before end of file
	{
		return 666;																// special code to indicate stop and no error
	}

//	IDLE
//	----
	else if(strcmp(Command,"IDLE")==0)										// end script before end of file, don't process
	{
		return 777;																// special code to indicate no processing
	}

// PROCESS
// -------
	else if(strcmp(Command,"PROCESS")==0)									// end of script processing
	{
		Process();																// fix symbols, search libraries, load code, dispatch, generate files
	}

// MATCHREFS
// ---------
	else if(strcmp(Command,"MATCHREFS")==0)									// see that all REF labels have a matching DEF
	{
		MatchRefs(&m_Refs,&m_Defs);												
	}

// NBUNDEF
// -------
	else if(strcmp(Command,"NBUNDEF")==0)									// count number of undefined labels
	{
		if(pLine[ptr]=='S')														// include SREFs
			Value=m_Refs.CountUndef(1);
		else
			Value=m_Refs.CountUndef(0);											// REF only

		cout << Value;															// report result
		if(g_Log&0x01) g_Logfile << Value;										// to logfile, if requested
	}

// SEARCHLIB
// ---------
	else if(strcmp(Command,"SEARCHLIB")==0)									// search libraries to try fixing undefined labels
	{
		SearchLib(&m_Libraries,&m_Segments,&m_Defs,&m_Refs,&m_Aorgs,m_PsegName);	
	}

// FIXTAB
// ---------
	else if(strcmp(Command,"FIXTAB")==0)										// fix symbol table (define REFs, fix relative addresses)
	{
		FixSymtab(&m_Defs,&m_Refs,&m_Segments);										
	}

// GETCODE
// ---------
	else if(strcmp(Command,"GETCODE")==0)									// load code into memory
	{
		Value=3;																// default flags
		i=GetParam(Label,8,pLine,&ptr,',');										// get flags
		if(i>0)												
		{
			Value=0;
			if(strstr(Label,"L")) Value|=2;										// do libraries
			if(strstr(Label,"O")) Value|=1;										// do object files
		}
		GetCode(&m_Objectfiles,&m_Libraries,&m_Segments,&m_Defs,&m_Refs,&m_Aorgs,Value); // load code into memory
	}

// GENERATE
// ---------
	else if(strcmp(Command,"GENERATE")==0)									// issue Memimage file(s)
	{
		Generate(&m_Selected,&m_Blocks.m_Blocks);									
	}

// ERRORS
// ------
	else if(strcmp(Command,"ERRORS")==0)										// set error level
	{
		if(pLine[ptr])															// make sure there is an argument
		{
			i=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);					// get value 
			if((i>=0)&&(i<=2))g_ErrLevel=i;										// accept if valid
		}
	}

// VERBOUS
// -------
	else if(strcmp(Command,"VERBOUS")==0)									// set verbousness level
	{
		if(pLine[ptr])															// make sure there is an argument
		{
			i=GetMath(pLine,&ptr,&m_Equates,&m_Defs,&m_Refs);					// get value 
			if((i>=0)&&(i<=4))g_Verbous=i;										// accept if valid
		}
	}

// LIBPATH
// -------
	else if(strcmp(Command,"LIBPATH")==0)									// library path
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// grab file path
		if(strlen(Name)+strlen(m_LibPath)>254)
		{
			if(g_Verbous>0)
			{
				cout << "Warning: Path maximal size (255 chars) exceeded";
				if(g_Log&0x02) g_Logfile << "Warning: Path maximal size (255 chars) exceeded";
			}
			if(g_ErrLevel>1) return 1;
		}
		else
		{
			strcat(m_LibPath,Name);												// append it to existing
			strcat(m_LibPath,";");													// seperator
		}
	}

// PATH
// ----
	else if(strcmp(Command,"PATH")==0)										// file path
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// grab file path
		if(strlen(Name)+strlen(m_Path)>254)
		{
			if(g_Verbous>0) 
			{
				cout << "Warning: Path maximal size (255 chars) exceeded";
				if(g_Log&0x02) g_Logfile << "Warning: Path maximal size (255 chars) exceeded";
			}
			if(g_ErrLevel>1) return 1;
		}
		else
		{
			strcat(m_Path,Name);												// append it to existing
			strcat(m_Path,";");													// seperator
		}
	}

// SCRIPT
// ------
	else if(strcmp(Command,"SCRIPT")==0)										// nested script
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// grab file name
								
		File2.open(Name,ios::nocreate);											// open the file
		if(!File2)																// error
		{
			cout << "Cannot open nested script file:" << Name << "\n";
			if(g_Log&0x02) g_Logfile << "Cannot open nested script file:" << Name << "\n";
			return 1;
		}
		ParseFile(&File2);														// parse nested script using this one's variables
	}

// JOB
// ---
	else if(strcmp(Command,"JOB")==0)										// nested script
	{
		GetQuotedstring(Name,80,pLine,&ptr);									// grab file name
								
		pScript=new CScriptfile;												// create new script object
		if(pScript->Open(Name)!=0)												// open the file
		{
			cout << "Cannot open external script file:" << Name << "\n";
			if(g_Log&0x02) g_Logfile << "Cannot open external script file:" << Name << "\n";
			return 1;
		}

		if(pScript->ParseFile()==0)												// parse the file
			pScript->Process();													// go on if no error

		delete(pScript);														// delete the external script object
	}

// HELP
// ----
	else if((strcmp(Command,"HELP")==0)||(strcmp(Command,"?")==0))			// display help screen 
	{
		cout << "Syntax: link99.exe              Enter interactive mode. Use @ before label. End with EXIT or ABORT\n";
		cout << "  or    link99.exe filename     Execute linker script\n";
		cout << "  or    link99.exe -switches    Execute command line \n\n";
		
		cout << "LINKER SCRIPT SYNTAX (Version 1.0)\n"; 
		cout << "====================\n";
		cout << "# any line starting with # is considered a comment line\n";
		cout << "Any line not starting with a space has a label in column 1 (no @ in script)\n\n";

		cout << "Main commands\n";
		cout << "-------------\n";
		cout << "MODEL   list                            Declare a memory model (optional name=label), with a list of devices/blocks\n";
		cout << "DEVICE  list                            Declare a memory device (optional name=label), with a list of blocks\n";
		cout << "BLOCK   memtype,start-end,fistpage-lastpage,step   Declare a memory block (optional name=label)\n";
		cout << "BLOCK   memtype,start,size,fistpage,nbpages,step   Alternative syntax for ranges\n";
		cout << "SELECT  list                            Select a memory model/device/block. Can also provide a list of such items (wildcards ok). ! = erase esisting list\n\n";


		cout << "LOAD \"filename\"                         Declare a DF80 file for loading\n";
		cout << "LIBRARY \"filename\"                      Declare a library of DF80 files to be used if needed\n";
		cout << "PATH \"path\"                             Default file path, if not that of linker script\n";
		cout << "LIBPATH \"path \"                         Default path for library files (can combine several with ; separator)\n\n";

		cout << "OUTPUT \"filename\",format,label_list     Output a memory image file, format+ ea5|gk|fb6, label_list=comma-separeted list of blocks, pages or segments (incl. wildcards)\n";
		cout << "OUTPUT \"filename\",format,$AORG          Output a memory image file for AORG stretches. Formats: EA5 GK FB6 or BIN\n\n";

		cout << "SEGMENT name,flags                      Emulate segments assemblers that don't support them. Type=SCEG,DSEG,ESEG\n";
		cout << "SEGEND                                  End of segment declared with SEGMENT\n";
		cout << "SEGFLAGS segment,flagson,flagsoff       Change segment flags (even if declared within DF80 file) Segment name can contain wildcards\n\n";

		cout << "STRATEGY                                Dispatching strategy: FIRST or LARGEST or list of segments (wildcards allowed) ! = erase existing list\n";
		cout << "ASSIGN  segment,block,page,address      Manually assign a segment to a block/page/address page can be 0 for any page\n";
		cout << "STUB block,segmentlist                  Assign segments to a block's stub (segment names can contain wildcards)\n";
		cout << "OVERLAY block,segmentlist               Reserve a block for overlay and assign segments to it (segment names can contain wildcards)\n";
		cout << "GHOST segment list                      Declare one or more segment (incl. wildcards) as ghost: loaded once, shared by several object files\n\n";

		cout << "MINGAP value                            Specifie how many missing bytes will cause a file to be split in two\n";
		cout << "MAXSIZE value                           Maximum memimage file size (not counting header)\n";
		cout << "EQUATE label,value                      Define/modifie an internal label\n";
		cout << "DEFINE label,value,tag,segnb            Create/redefine a DEF label (-1 or omitted value = no change)\n";
		cout << "ERRORS value                            Error level to abort 0: never, 1: abort on errors 2: abort on warnings (default = 1)\n\n";

		cout << "Listing commands\n";
		cout << "----------------\n";
		cout << "VERBOUS value                           Verbousness level 0: only error messages 1: errors + warnings 2: main comments 3: detailed info 4: debug info\n";
		cout << "LOGFILE filename,options                Log file name. Options: I=infos E=errors&warnings\n";
		cout << "SEGTABLE pass,options                   Output segment table. Pass = 1 or 2\n";
		cout << "SYMTABLE pass,options                   Output symbol table. Options: D=def R=ref S=sref (default=all)\n";
		cout << "MEMTABLE pass,options                   Output memory table. Options: m=model d=device b=block\n\n";

		cout << "Advanced commands\n";
		cout << "-----------------\n";
		cout << "VERIFY segment,offset,data,data,...     Verify that data at offset in segment equals a list of values (? = skip)\n";
		cout << "PATCH segment,offset,data,data,...      Modify data at offset in segment with list of values (? = skip) Disabled if previous verify false\n";
		cout << "PROCESS                                 End-of-script processing: fix symbols, search libraries, dispatch, load code, generate files\n";
		cout << "MATCHREFS                               Match REF labels with a DEF in symbol table\n";
		cout << "NBUNDEF type                            Count undef REFs and/or SREFs type=S: do SREF (default = no)\n";
		cout << "SEARCHLIB                               Search libraries, decide which segment to include\n";
		cout << "DISPATCH wipe                           Dispatch remaining segments to memory block (remove existing if requested)\n";
		cout << "FIXTAB                                  Fix symbol table: REF values, relative offsets\n";
		cout << "GETCODE type                            Load code into memory, fix REF chains. Type: O=object files, L=libraries (default=both)\n";
		cout << "GENERATE                                Generate memimage files\n\n";

		cout << "SCRIPT \"scriptfile\"                     Nested script file\n";
		cout << "JOB \"scriptfile\"                        External script file\n\n";
		cout << "FIND type,name,value                    Type:(pass #)S LD LR LU LE LA MM MD MB MP, name (optional), value=occurence (page# for MP). Results placed into Equates.";

		cout << "Script-related commands\n";
		cout << "-----------------------\n";
		cout << "ECHO                                    Print something on screen \"text\" labelnames (separated with spaces) \\t and \\nl accepted (out of quotes)\n";
		cout << "IF                                      Conditional processing (&& and || accepted)\n";
		cout << "ELSE                                    Reverse the condition\n";
		cout << "ENDIF                                   End of condition\n";
		cout << "ELIF                                    Alternative condition (else if)\n";
		cout << "STOP                                    End script before end-of-file, do processing\n";
		cout << "IDLE                                    End script file without processing\n\n";

		cout << "Predefined equates\n";
		cout << "------------------\n";
		cout << "ROM,RAM,NVRAM,OVL,$PSEG,$DSEG,$CSEG,$BLANK	Segment flags\n";
		cout << "$TYPE,$SIZE,$ADDR,$PAGE,$COUNT          Results of FIND command\n\n";

		cout << "Command line switches\n";
		cout << "=====================\n";
		cout << "-s filename                             Process script\n";
		cout << "-l filename_list                        Load file or coma-separeted list of files. Better used before -script\n";
		cout << "-L filename-liste                       List library files. Better used after -script\n";
		cout << "-o filename,type,segmentlist            Output file, as inside script. Better used after script\n";
		cout << "-m blockname                            Select memory model/device/block. Better used after script\n\n";
		if(g_Log&0x01)
		{
			g_Logfile << "Syntax: link99.exe              Enter interactive mode. Use @ before label. End with EXIT or ABORT\n";
			g_Logfile << "  or    link99.exe filename     Execute linker script\n";
			g_Logfile << "  or    link99.exe -switches    Execute command line \n\n";
			
			g_Logfile << "LINKER SCRIPT SYNTAX (Version 1.0)\n"; 
			g_Logfile << "====================\n";
			g_Logfile << "# any line starting with # is considered a comment line\n";
			g_Logfile << "Any line not starting with a space has a label in column 1 (no @ in script)\n\n";

			g_Logfile << "Main commands\n";
			g_Logfile << "-------------\n";
			g_Logfile << "MODEL   list                            Declare a memory model (optional name=label), with a list of devices/blocks\n";
			g_Logfile << "DEVICE  list                            Declare a memory device (optional name=label), with a list of blocks\n";
			g_Logfile << "BLOCK   memtype,start-end,fistpage-lastpage,step   Declare a memory block (optional name=label)\n";
			g_Logfile << "BLOCK   memtype,start,size,fistpage,nbpages,step   Alternative syntax for ranges\n";
			g_Logfile << "SELECT  list                            Select a memory model/device/block. Can also provide a list of such items (wildcards ok). ! = erase esisting list\n\n";


			g_Logfile << "LOAD \"filename\"                         Declare a DF80 file for loading\n";
			g_Logfile << "LIBRARY \"filename\"                      Declare a library of DF80 files to be used if needed\n";
			g_Logfile << "PATH \"path\"                             Default file path, if not that of linker script\n";
			g_Logfile << "LIBPATH \"path \"                         Default path for library files (can combine several with ; separator)\n\n";

			g_Logfile << "OUTPUT \"filename\",format,label_list     Output a memory image file, format+ ea5|gk|fb6, label_list=comma-separeted list of blocks, pages or segments (incl. wildcards)\n";
			g_Logfile << "OUTPUT \"filename\",format,$AORG          Output a memory image file for AORG stretches. Formats: EA5 GK FB6 or BIN\n\n";

			g_Logfile << "SEGMENT name,flags                      Emulate segments assemblers that don't support them. Type=SCEG,DSEG,ESEG\n";
			g_Logfile << "SEGEND                                  End of segment declared with SEGMENT\n";
			g_Logfile << "SEGFLAGS segment,flagson,flagsoff       Change segment flags (even if declared within DF80 file) Segment name can contain wildcards\n\n";

			g_Logfile << "STRATEGY                                Dispatching strategy: FIRST or LARGEST or list of segments (wildcards allowed) ! = erase existing list\n";
			g_Logfile << "ASSIGN  segment,block,page,address      Manually assign a segment to a block/page/address page can be 0 for any page\n";
			g_Logfile << "STUB block,segmentlist                  Assign segments to a block's stub (segment names can contain wildcards)\n";
			g_Logfile << "OVERLAY block,segmentlist               Reserve a block for overlay and assign segments to it (segment names can contain wildcards)\n";
			g_Logfile << "GHOST segment list                      Declare one or more segment (incl. wildcards) as ghost: loaded once, shared by several object files\n\n";

			g_Logfile << "MINGAP value                            Specifie how many missing bytes will cause a file to be split in two\n";
			g_Logfile << "MAXSIZE value                           Maximum memimage file size (not counting header)\n";
			g_Logfile << "EQUATE label,value                      Define/modifie an internal label\n";
			g_Logfile << "DEFINE label,value,tag,segnb            Create/redefine a DEF label (-1 or omitted value = no change)\n";
			g_Logfile << "ERRORS value                            Error level to abort 0: never, 1: abort on errors 2: abort on warnings (default = 1)\n\n";

			g_Logfile << "Listing commands\n";
			g_Logfile << "----------------\n";
			g_Logfile << "VERBOUS value                           Verbousness level 0: only error messages 1: errors + warnings 2: main comments 3: detailed info 4: debug info\n";
			g_Logfile << "LOGFILE filename,options                Log file name. Options: I=infos E=errors&warnings\n";
			g_Logfile << "SEGTABLE pass,options                   Output segment table. Pass = 1 or 2\n";
			g_Logfile << "SYMTABLE pass,options                   Output symbol table. Options: D=def R=ref S=sref (default=all)\n";
			g_Logfile << "MEMTABLE pass,options                   Output memory table. Options: m=model d=device b=block\n\n";

			g_Logfile << "Advanced commands\n";
			g_Logfile << "-----------------\n";
			g_Logfile << "VERIFY segment,offset,data,data,...     Verify that data at offset in segment equals a list of values (? = skip)\n";
			g_Logfile << "PATCH segment,offset,data,data,...      Modify data at offset in segment with list of values (? = skip) Disabled if previous verify false\n";
			g_Logfile << "PROCESS                                 End-of-script processing: fix symbols, search libraries, dispatch, load code, generate files\n";
			g_Logfile << "MATCHREFS                               Match REF labels with a DEF in symbol table\n";
			g_Logfile << "NBUNDEF type                            Count undef REFs and/or SREFs type=S: do SREF (default = no)\n";
			g_Logfile << "SEARCHLIB                               Search libraries, decide which segment to include\n";
			g_Logfile << "DISPATCH wipe                           Dispatch remaining segments to memory block (remove existing if requested)\n";
			g_Logfile << "FIXTAB                                  Fix symbol table: REF values, relative offsets\n";
			g_Logfile << "GETCODE type                            Load code into memory, fix REF chains. Type: O=object files, L=libraries (default=both)\n";
			g_Logfile << "GENERATE                                Generate memimage files\n\n";

			g_Logfile << "SCRIPT \"scriptfile\"                     Nested script file\n";
			g_Logfile << "JOB \"scriptfile\"                        External script file\n\n";
			g_Logfile << "FIND type,name,value                    Type:(pass #)S LD LR LU LE LA MM MD MB MP, name (optional), value=occurence (page# for MP). Results placed into Equates.";

			g_Logfile << "Script-related commands\n";
			g_Logfile << "-----------------------\n";
			g_Logfile << "ECHO                                    Print something on screen \"text\" labelnames (separated with spaces) \\t and \\nl accepted (out of quotes)\n";
			g_Logfile << "IF                                      Conditional processing (&& and || accepted)\n";
			g_Logfile << "ELSE                                    Reverse the condition\n";
			g_Logfile << "ENDIF                                   End of condition\n";
			g_Logfile << "ELIF                                    Alternative condition (else if)\n";
			g_Logfile << "STOP                                    End script before end-of-file, do processing\n";
			g_Logfile << "IDLE                                    End script file without processing\n\n";

			g_Logfile << "Predefined equates\n";
			g_Logfile << "------------------\n";
			g_Logfile << "ROM,RAM,NVRAM,OVL,$PSEG,$DSEG,$CSEG,$BLANK	Segment flags\n";
			g_Logfile << "$TYPE,$SIZE,$ADDR,$PAGE,$COUNT          Results of FIND command\n\n";

			g_Logfile << "Command line switches\n";
			g_Logfile << "=====================\n";
			g_Logfile << "-s filename                             Process script\n";
			g_Logfile << "-l filename_list                        Load file or coma-separeted list of files. Better used before -script\n";
			g_Logfile << "-L filename-liste                       List library files. Better used after -script\n";
			g_Logfile << "-o filename,type,segmentlist            Output file, as inside script. Better used after script\n";
		g_Logfile << "-m blockname                            Select memory model/device/block. Better used after script\n\n";
		}
	}
	else 															// illegal command
	{
		if(g_Verbous>0) 
		{
			cout << "Warning: Illegal command: " << Command << "\n";
			if(g_Log&0x02) g_Logfile << "Warning: Illegal command: " << Command << "\n";
		}
		if(g_ErrLevel>1) return 1;
	}

	return 0;
}

//----------------------------------------------------------------------------------
// Parse the entire file (first pass)
//----------------------------------------------------------------------------------
int CScriptfile::ParseFile(ifstream* pFile)
{
	int level=0;
	int truth=0;
	int err;

	if(pFile==NULL) pFile=&m_File;												// this file
	if(g_Verbous>1)
	{
		cout << "Parsing script...\n";
		if(g_Log&0x01) g_Logfile << "Parsing script...\n";
	}

	while(!pFile->eof())														// parse the whole file
	{
		pFile->getline(m_Buffer,255);											// grab one line

		if(CheckCondition(m_Buffer,&level,&truth)>0) return 1;					// check for control statements, abort on errors

		if(truth>=level)														// executing
		{
			err=ParseLine(m_Buffer);											// parse the line
			if(err==666) break;													// special code for STOP command
			if(err>g_ErrLevel) return 1;										// abort on errors
		}
	}

	return 0;																	// report success
}

//----------------------------------------------------------------------------------
// Check for conditional statements (IF, ELSE, ENDIF)
//----------------------------------------------------------------------------------
int CScriptfile::CheckCondition(char* pLine, int* pLevel, int* pTruth)
{
	char Label[9];
	char Command[11];
	int ptr=0;

	Label[0]=0x00;
	if(pLine[ptr]=='#') return 0;												// comment line
	if(pLine[ptr] > ' ') GetLabel(Label,8,pLine,&ptr);							// grab label
	if(pLine[ptr]=='#') return 0;												// the rest is a comment
	GetCommand(Command,10,pLine,&ptr);											// grab command

	if(strcmp(Command,"IF")==0)													// IF statement
	{
		if(*pTruth>=*pLevel)													// currently true
		{
			if(GetCondition(pLine,&ptr))										// is condition true ?
				*pTruth=(*pTruth)+1;											// yes: increase # of true statements
		}																		// otherwise ignore condition

		*pLevel=*pLevel+1;														// in any case, increase the # of if statements
	}

	else if(strcmp(Command,"ELSE")==0)											// ELSE statement
	{
		if(*pTruth>=*pLevel)													// currently true
			*pTruth=*pTruth-1;													// make it false
		else if(*pTruth==*pLevel-1)												// currently false by 1 statement
			*pTruth=*pTruth+1;													// make it true
	}

	else if(strcmp(Command,"ELIF")==0)											// ELSE statement
	{
		if(*pTruth>=*pLevel)													// currently true
			*pTruth=*pTruth-1;													// make it false
		else if(*pTruth==*pLevel-1)												// currently false by 1 statement
		{
			if(GetCondition(pLine,&ptr))										// is condition true ?
				*pTruth=(*pTruth)+1;											// yes: increase # of true statements
		}																	
	}																			// same level of if statements

	else if(strcmp(Command,"ENDIF")==0)											// ENDIF statement
	{
		if(*pTruth>=*pLevel)													// currently true
			*pTruth=*pTruth-1;													// decrease # of true statements

		*pLevel=*pLevel-1;														// decrease the level
	}

	return 0;																	// report success
}

//----------------------------------------------------------------------------------
// Get boolean condition
//----------------------------------------------------------------------------------
int CScriptfile::GetCondition(char* pLine, int* pPtr)
{
	int i=*pPtr,op=0,not=0,res=0;
	int val1,val2=0;

	for(;((pLine[i]==' ')||(pLine[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 

	if(pLine[i]=='!')															// logical not
	{
		i++;
		not=1;
	}

	for(;((pLine[i]==' ')||(pLine[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 

	if(pLine[i]=='(')															// sub-condition
	{
		i++;																	// skip the ( parenthesis
		res=GetCondition(pLine,&i);												// get subcondition
		if(not) res = (!res);													// invert if needed
		if(pLine[i]==')')														// matching ) parenthesis
		{
			for(i++;((pLine[i]==' ')||(pLine[i]=='\t'))&&(i<255);i++);			// skip ) and any spaces and tabs
		}
	}
	else
	{																			// process condition
		val1=GetMath(pLine,&i,&m_Equates,&m_Defs,&m_Refs);						// first value

		if((pLine[i]=='=')&&(pLine[i+1]=='='))									// parse logical operator
		{
			op=1;
			i+=2;
		}
		else if((pLine[i]=='!')&&(pLine[i+1]=='='))
		{
			op=2;
			i+=2;
		}
		else if((pLine[i]=='>')&&(pLine[i+1]=='='))
		{
			op=3;
			i+=2;
		}
		else if((pLine[i]=='<')&&(pLine[i+1]=='='))
		{
			op=4;
			i+=2;
		}
		else if(pLine[i]=='>')
		{
			op=5;
			i++;
		}
		else if(pLine[i]=='<')
		{
			op=6;
			i++;
		}
		else if(pLine[i]==')')													// subcondition is one value
		{
			*pPtr=i;
			return (val1!=0);													// impicit comparison to 0
		}

		val2=GetMath(pLine,&i,&m_Equates,&m_Defs,&m_Refs);						// second value

		switch(op)																// apply operator
		{
		case 1:
			res=(val1==val2);
			break;
		case 2:
			res=(val1!=val2);
			break;
		case 3:
			res=(val1>=val2);
			break;
		case 4:
			res=(val1<=val2);
			break;
		case 5:
			res=(val1>val2);
			break;
		case 6:
			res=(val1<val2);
			break;
		}
	}

	for(;((pLine[i]==' ')||(pLine[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 

	if((pLine[i]=='&')&&(pLine[i+1]=='&'))										// logical and						
	{
		i+=2;
		res=(res && GetCondition(pLine,&i));
	}
	else if((pLine[i]=='|')&&(pLine[i+1]=='|'))									// logical or
	{
		i+=2;
		res=(res || GetCondition(pLine,&i));
	}
	
	for(;((pLine[i]==' ')||(pLine[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 

	*pPtr=i;																	// update ptr
	return res;																	// return result
}


//----------------------------------------------------------------------------------
// Try to match every REF label with a DEF
//----------------------------------------------------------------------------------
void CScriptfile::MatchRefs(CLabelList* pRefs, CLabelList *pDefs)
{
	int i;
	CLabel *pLabel;

	for(i=0;i<pRefs->GetSize();i++)												// iterate list of REF labels
	{
		pLabel=(CLabel*)pRefs->m_Labels.GetAt(i);								// get one
	
		if(pDefs->GetValue(pLabel->m_Name)>=0)									// look for corresponding DEF label
		{
			pLabel->m_Undefined=0;												// make it defined now
		}
	}
}


//----------------------------------------------------------------------------------
// Search libraries for unsolved REFs
//----------------------------------------------------------------------------------
void CScriptfile::SearchLib(CList *pLibList, CSegmentList* pSegments, CLabelList* pDefs, CLabelList* pRefs, CAorgList* pAorgs, char* PsegName)
{
	int i;
	CLibrary *pLib;

	for(i=0;i<m_Libraries.GetSize();i++)										// scan libraries 
	{
		pLib=(CLibrary*)m_Libraries.GetAt(i);									// get one file
		pLib->Search(pSegments,pDefs,pRefs,pAorgs,PsegName);					// search it for undef labels
	}
}

//----------------------------------------------------------------------------------
// Fix symbol table
//----------------------------------------------------------------------------------
void CScriptfile::FixSymtab(CLabelList* pDefs, CLabelList* pRefs, CSegmentList* pSegments)
{
	int i,value,psegaddr;
	CLabel *pLabel;
	CSegment* pSeg;


	// Adjust address of segment-relative DEF labels

	psegaddr=pSegments->GetAddress("$PSEG");									// do PSEG once and for all

	for(i=0;i<pDefs->GetSize();i++)												// iterate list of DEF labels
	{
		pLabel=(CLabel*)pDefs->m_Labels.GetAt(i);								// get one label
		
		if(pLabel->m_Type!='6')	continue;										// don't touch AORG labels
		if(pLabel->m_Type!='5')													// labels in PSEG
		{
			if(psegaddr<0)
			{
				cout << "Cannot find $PSEG for label  " << pLabel->m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Cannot find $PSEG for label  " << pLabel->m_Name << "\n";
				if(g_ErrLevel>0) return;
			}
			pLabel->m_Value+=psegaddr;											// fix address to make is absolute
		}
		else																	// label is in a CSEG or DSEG
		{
			pSeg=(CSegment*)pSegments->m_Segments.GetAt(pLabel->m_SegIdx);		// get parent segment for label
			if(pSeg==NULL)														// not found (weird)
			{
				cout << "Cannot find segment for label  " << pLabel->m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Cannot find segment for label  " << pLabel->m_Name << "\n";
				if(g_ErrLevel>0) return;
			}
			pLabel->m_Value+=pSeg->m_Address;									// fix address to make is absolute
		}
		if(g_Verbous>=4) 
		{
			cout.setf(ios::hex|ios::uppercase);
			cout << "Fixing DEF "<< pLabel->m_Name << " >" << pLabel->m_Value << "\n";
			cout.unsetf(ios::hex|ios::uppercase);
			if(g_Log&0x01)
			{
				g_Logfile.setf(ios::hex|ios::uppercase);
				g_Logfile << "Fixing DEF "<< pLabel->m_Name << " >" << pLabel->m_Value << "\n";
				g_Logfile.unsetf(ios::hex|ios::uppercase);
			}
		}
	}

	// Find correct address for REF labels
	
	for(i=0;i<m_Refs.GetSize();i++)												// iterate list of REF labels
	{
		pLabel=(CLabel*)pRefs->m_Labels.GetAt(i);								// get one
	
		if((value=pDefs->GetValue(pLabel->m_Name))>=0)							// look for corresponding DEF label
		{
			pLabel->m_Value=value;												// insert DEF value into REF label
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "Solving REF "<< pLabel->m_Name << " >" << value << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile << "Solving REF "<< pLabel->m_Name << " >" << value << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------
// Load code into memory
//----------------------------------------------------------------------------------
void CScriptfile::GetCode(CList *pFiles, CList* pLibraries, CSegmentList* pSegments, CLabelList* pDefs, CLabelList* pRefs, CAorgList* pAorgs, int flags)
{
	int i;
	CObjectfile *pObj;
	CLibrary *pLib;

	pSegments->AllocateBuffers();												// allocate data buffer for all segments (free existing ones)
	pAorgs->Merge(m_MinGap);													// merge AORG stretches
	pAorgs->AllocateBuffers();													// allocate buffers for remaining AORG stretches

	if(flags&&0x01)																// from object files
	{
		for(i=0;i<pFiles->GetSize();i++)										// iterate list of object files
		{
			pObj=(CObjectfile*)pFiles->GetAt(i);								// get one file
			if(g_Verbous>=4)
			{
				cout << "Loading code from object file '" << pObj->m_Filename << "'\n";
				if(g_Log&0x01) g_Logfile << "Loading code from object file '" << pObj->m_Filename << "'\n";
			}
			pObj->Seek(0);														// rewind it
			pObj->SecondPass(pSegments,pDefs,pRefs,pAorgs);						// second pass through object file
		}
	}

	if(flags&0x02)																// from libraries
	{
		for(i=0;i<pLibraries->GetSize();i++)									// iterate list of libraries
		{
			if(g_Verbous>=4)
			{
				cout << "Loading code from library '" << pObj->m_Filename << "'\n";
				if(g_Log&0x01) g_Logfile << "Loading code from library '" << pObj->m_Filename << "'\n";
			}
			pLib=(CLibrary*)pLibraries->GetAt(i);								// get one
			pLib->SecondPass(pSegments,pDefs,pRefs,pAorgs);						// second pass for loaded libraries
		}
	}

}

//----------------------------------------------------------------------------------
// Generate memory image files
//----------------------------------------------------------------------------------
void CScriptfile::Generate(CList *pSelected, CList* pBlocks)
{
	int i;
	CMemimage *pMemimage;

	for(i=0;i<m_Memimages.GetSize();i++)										// iterate list of memory image files
	{
		pMemimage=(CMemimage*)m_Memimages.GetAt(i);								// get one
		pMemimage->OutputCode(pSelected,pBlocks);								// issue its code
	}
}

//----------------------------------------------------------------------------------
// Process parsed data
//----------------------------------------------------------------------------------
int CScriptfile::Process()
{
	int value,i,j,address,verified,value1;
	CPatch* pPatch;
	char *pChr;

	MatchRefs(&m_Refs,&m_Defs);													// see that all REF labels have a matching DEF

	// Search libraries if needed
	if((i=m_Refs.CountUndef(1))>0)												// are there undefined REF or SREF?
	{
		if(m_Libraries.GetSize()>0)												// are threre libraries ?
		{
			if(g_Verbous>1)
			{
				cout << "There are " << i << " undefined REF or SREF labels\nSearching libraries...\n";
				if(g_Log&0x01) g_Logfile << "There are " << i << " undefined REF or SREF labels\nSearching libraries...\n";
			}
			SearchLib(&m_Libraries,&m_Segments,&m_Defs,&m_Refs,&m_Aorgs,m_PsegName);	// search libraries to try fixing undefined labels
			MatchRefs(&m_Refs,&m_Defs);											// check again for matching DEFs
		}
		if((i=m_Refs.CountUndef(0))>0)											// are there undefined REF left?
		{
			if(g_Verbous>0)
			{
				cout << "Warning: There are still " << i << " undefined REF labels\n";
				if(g_Log&0x01) g_Logfile << "Warning: There are still " << i << " undefined REF labels\n";
			}
			if(g_ErrLevel>1) return 1;
		}
	}

	if(g_Verbous>1)
	{
		cout << "Dispatching segments...\n";
		if(g_Log&0x01) g_Logfile << "Dispatching segments...\n";
	}
	Dispatch();																	// dispatch segments to blocks
	if(m_Segments.CheckList()>0)
	{
		cout << "Not all segments could be assigned\n";
		if(g_Log&0x01) g_Logfile << "Not all segments could be assigned\n";
		if(g_ErrLevel>1) return 1;
	}

	if(g_Verbous>1)
	{
		cout << "Fixing symbol table...\n";
		if(g_Log&0x01) g_Logfile << "Fixing symbol table...\n";
	}
	FixSymtab(&m_Defs,&m_Refs,&m_Segments);										// fix symbol table (define REFs, fix relative addresses)

	if(g_Verbous>1)
	{
		cout << "Loading code...\n";
		if(g_Log&0x01) g_Logfile << "Loading code...\n";
	}
	GetCode(&m_Objectfiles,&m_Libraries,&m_Segments,&m_Defs,&m_Refs,&m_Aorgs);	// load code into memory

	// Output tables as requested
	if(strchr(m_Tables,'s')) m_Segments.OutputTable();							// segment table

	if(strcspn(m_Tables,"56W")<strlen(m_Tables))				
		m_Defs.OutputTable(m_Tables);											// DEF labels
	if(strcspn(m_Tables,"34VXYZVYZ")<strlen(m_Tables))
		m_Refs.OutputTable(m_Tables);											// REF labels

	if(strcspn(m_Tables,"mdb")<strlen(m_Tables))								// memory table
		m_Blocks.OutputTable(m_Tables);
	
	// Process list of searches
	for(i=0;i<m_Searches.GetSize();i++)											// iterate list
	{
		pChr=(char*)m_Searches.GetAt(i);										// get text line
		Find(pChr);																// parse and perform search
	}

	// Process list of patches
	verified=1;																	// assume ok to start
	if((g_Verbous>1)&&(m_Patches.GetSize()>0))
	{
		cout << "Applying patches...\n";
		if(g_Log&0x01) g_Logfile << "Applying patches...\n";
	}
	for(i=0;i<m_Patches.GetSize();i++)											// process all patch objects
	{
		pPatch=(CPatch*)m_Patches.GetAt(i);										// get one
		if((pPatch->m_Type==0)&&(verified==1))									// it's a patch
		{
			for(j=0,address=pPatch->m_Address;j<pPatch->m_Values.GetSize();j++,address+=2)	// process list of values
			{
				if((value=(int)pPatch->m_Values.GetAt(j))==-1) continue;		// skip this one
				if(m_Segments.SetData(pPatch->m_SegName,address,value,-1,1)<0)	// modify segment data (address is offset)
				{
					if(g_Verbous>0) 
					{
						cout << "Warning: Could not patch segment " << pPatch->m_SegName << "/n";
						if(g_Log&0x02) g_Logfile << "Warning: Could not patch segment " << pPatch->m_SegName << "/n";
					}
					if(g_ErrLevel>1) return 1;
				}
				if(g_Verbous>=4) 
				{
					cout.setf(ios::hex|ios::uppercase);
					cout << "Patched " << pPatch->m_SegName << " >" << pPatch->m_Address << "\n";
					cout.unsetf(ios::hex|ios::uppercase);
					if(g_Log&0x01)
					{
						g_Logfile.setf(ios::hex|ios::uppercase);
						g_Logfile <<"Patched " << pPatch->m_SegName << " >" << pPatch->m_Address << "\n";
						g_Logfile.unsetf(ios::hex|ios::uppercase);
					}
				}
			}
		}
		else																	// it's a verify
		{
			for(j=0,address=pPatch->m_Address;j<pPatch->m_Values.GetSize();j++,address+=2)	// process list of values
			{
				if((value=(int)pPatch->m_Values.GetAt(j))==-1) continue;		// skip this one
				if((value1=m_Segments.GetData(pPatch->m_SegName,address,1))<0)	// get data from segment, by offset
				{
					if(g_Verbous>0)
					{
						cout << "Warning: Could not verify data in segment " << pPatch->m_SegName << "/n";
						if(g_Log&0x02) g_Logfile << "Warning: Could not verify data in segment " << pPatch->m_SegName << "/n";
					}
					if(g_ErrLevel>1) return 1;
				}
				if(value!=value1)												// check if matches
				{
					verified=0;													// mismatch: stops here
					if(g_Verbous>2)
					{
						cout << "VERIFY failed/n";
						if(g_Log&0x02) g_Logfile << "VERIFY failed/n";
					}
					break;
				}
			}
			if(j>=pPatch->m_Values.GetSize())									// all values done ?
			{
				verified=1;														// yes: verified
				if(g_Verbous>2) 
				{
					cout << "VERIFY succeeded/n";
					if(g_Log&0x02) g_Logfile << "VERIFY failed/n";
				}
			}
		}
	}																			// next patch object

	if(g_Verbous>1)
	{
		cout << "Outputing memory image files...\n";
		if(g_Log&0x01) g_Logfile << "Outputing memory image files...\n";
	}
	Generate(&m_Selected,&m_Blocks.m_Blocks);									// issue Memimage file(s)

	return 0;
}

//----------------------------------------------------------------------------------
// Read a word from buffer, blank separated
//----------------------------------------------------------------------------------
int CScriptfile::GetCommand(char* Word, int max, char* pBuf, int* ptr)
{
	int j,i=*ptr;

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip leading spaces

	for(j=0;(i<255)&&(j<max)&&(pBuf[i]>' ');i++,j++)
		Word[j]=pBuf[i];														// copy word
	
	Word[j]=0x00;																// add end mark

	for(;(pBuf[i]>' ')&&(i<255);i++);											// skip rest of word
	
	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip trailing spaces

	*ptr=i;																		// decount characters

	return j;																	// return name size
}

//----------------------------------------------------------------------------------
// Read a word from buffer, specifying separator
//----------------------------------------------------------------------------------
int CScriptfile::GetParam(char* Word, int max, char* pBuf, int* ptr, char sep)
{
	int j,i=*ptr;

	if(pBuf[i]==sep) i++;														// skip initial separator

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip leading spaces

	for(j=0;(i<255)&&(j<max)&&(pBuf[i]!=sep)&&(pBuf[i]>' ');i++,j++)
		Word[j]=pBuf[i];														// copy word
	
	Word[j]=0x00;																// add end mark

	for(;(pBuf[i]!=sep)&&(pBuf[i]>' ')&&(i<255);i++);							// skip rest of word
	
	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip trailing spaces

	*ptr=i;																		// decount characters

	return j;																	// return name size
}

//----------------------------------------------------------------------------------
// Read a word from buffer, specifying separator
//----------------------------------------------------------------------------------
int CScriptfile::GetLabel(char* Word, int max, char* pBuf, int* ptr)
{
	int j,i=*ptr;

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip leading spaces

	for(j=0;(i<255)&&(j<max)&&(((pBuf[i]>='A')&&(pBuf[i]<='Z'))||((pBuf[i]>='0')&&(pBuf[i]<='9'))||(pBuf[i]=='$')||(pBuf[i]=='_'));i++,j++)
		Word[j]=pBuf[i];														// copy word
	
	Word[j]=0x00;																// add end mark

	for(;(i<255)&&(((pBuf[i]<='A')&&(pBuf[i]>='Z'))||((pBuf[i]<='0')&&(pBuf[i]>='9'))||(pBuf[i]=='$')||(pBuf[i]=='_'));i++);	// skip rest of word
	
	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);		// skip trailing spaces

	*ptr=i;																		// decount characters

	return i;																	// return current position
}

//----------------------------------------------------------------------------------
// Get quoted string
//----------------------------------------------------------------------------------
int CScriptfile::GetQuotedstring(char* Word, int max, char* pBuf, int* ptr)
{
	int j,i=*ptr;
	char c;

	c=pBuf[i];
	if((c!='"')&&(c!='\'')) return 0;


	for(i++,j=0;(pBuf[i]!=c)&&(i<255)&&(j<max);i++,j++)							// find matching quote
		Word[j]=pBuf[i];														// copy string
	Word[j]=0x00;																// endmark
	
	for(i++;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255)&&(pBuf[i]!=0x00);i++);	// skip trailing spaces

	*ptr=i;																		// decount characters

	return j;																	// return string size
}

//----------------------------------------------------------------------------------
// Get value from a math expression (default: 0)
//----------------------------------------------------------------------------------
int CScriptfile::GetMath(char* pBuf, int* ptr, CLabelList* pEquates, CLabelList* pDefs, CLabelList* pRefs)
{
	int i;
	char unary='+';
	int val=0,val1;
	char name[33];

	for(i=*ptr;((pBuf[i]==' ')||(pBuf[i]=='\t')||(pBuf[i]==','))&&(i<255);i++);	// skip leading spaces and comas 

	if((pBuf[i]=='+')||(pBuf[i]=='-')||(pBuf[i]=='~')||(pBuf[i]=='@'))			// unary operator
		unary=pBuf[i++];														// remember it and skip to next char

	if(pBuf[i]=='(')															// sub-expression
	{
		i++;																	// skip the parenthesis
		val=GetMath(pBuf,&i);
		if(pBuf[i]==')')														// matching parenthesis
		{
			for(i++;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255);i++);			// skip ) and any spaces and tabs
		}
	}
	else if(pBuf[i]=='>')														// hexadecimal mark
	{
		i++;																	// skip it
		val=GetHex(4,pBuf,&i);													// get hex value
	}
	else if((pBuf[i]>='0')&&(pBuf[i]<='9'))										// regular number
		val=GetNumber(4,pBuf,&i);												// get decimal value
	else																		
	{																			// search labels, segments, etc.
		val1=-1;
		GetLabel(name,32,pBuf,&i);												// grab name
		if(pEquates!=NULL)														// search equates
		{
			if((val1=pEquates->GetValue(name))>=0)								// see if name exists
				val=val1;														// yes
		}
		if((val1<0)&&(pDefs!=NULL))												// search def labels
		{
			if((val1=pDefs->GetValue(name))>=0)
				val=val1;
		}
		if((val1<0)&&(pRefs!=NULL))												// search ref labels
		{
			if((val1=pRefs->GetValue(name))>=0)
				val=val1;
		}
	}

	switch(unary)																// apply unary operator
	{	
	case '-':																	// negation
		val=-val;
		break;
	case '~':																	// bitwise inversion
		val=val^0xFFFFFFFF;
		break;
	case '@':																	// absolute value
		if(val<0) val=-val;
		break;
	}

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 
	
	switch(pBuf[i++])															// handle binary op (if any)
	{
	case '+':																	// addition
		val=val+GetMath(pBuf,&i);
		break;
	case '-':																	// substraction
		val=val-GetMath(pBuf,&i);
		break;
	case '*':																	// multiplication
		val=val*GetMath(pBuf,&i);
		break;
	case '/':																	// division
		val=val/GetMath(pBuf,&i);
		break;
	case '%':																	// modulo
		val=val%GetMath(pBuf,&i);
		break;
	case '&':																	// bitwise and
		val=val&GetMath(pBuf,&i);
		break;
	case '|':																	// bitwise or
		val=val+GetMath(pBuf,&i);
		break;
	case '^':																	// bitwise xor
		val=val^GetMath(pBuf,&i);
		break;
	case ')':																	// end of sub-expression
		i--;																	// keep it
		break;																	
	default:
		i--;																	// other char: do not use it
		break;
	}

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t'))&&(i<255);i++);						// skip spaces and tabs 

	*ptr=i;																		// adjust ptr
	return val;																	// return result
}

//----------------------------------------------------------------------------------
// Get hexadecimal value
//----------------------------------------------------------------------------------
int CScriptfile::GetNumber(int count, char* pBuf, int* ptr)
{
	int i=*ptr,j,res=0;
	char c;

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t')||(pBuf[i]==','))&&(i<255);i++);		// skip leading spaces and comas 
	*ptr=i;

	if(pBuf[i]=='>') return GetHex(count,pBuf,ptr);								// found > hex mark: parse hexadecimal number

	for(j=0;(j<count)&&(i<255);i++,j++)											// parse decimal number
	{
		c=pBuf[i];																// get one char
		if((c<'0')||(c>'9')) break;												// not in 0-9: we are done
		res=(res*10)+(int)(c-'0');												// use digit
	}

	*ptr=i;																		// decount characters

	return res;																	// return result
}

//----------------------------------------------------------------------------------
// Get hexadecimal value
//----------------------------------------------------------------------------------
int CScriptfile::GetHex(int count, char* pBuf, int* ptr)
{
	int i=*ptr,j,res=0;
	char c;

	for(;((pBuf[i]==' ')||(pBuf[i]=='\t')||(pBuf[i]=='>')||(pBuf[i]==',')||(pBuf[i]=='-'))&&(i<255);i++);	// skip leading spaces comas, and > mark


	for(j=0;(j<count)&&(i<255);i++,j++)											// parse hex number
	{
		c=pBuf[i];																// get one char
		if(c<'0') break;														// illegal
		else if(c<='9') res=(res*16)+(int)(c-'0');								// 0-9: use digit
		else if((c<'A')||(c>'F')) break;										// illegal
		else res=(res*16)+(int)(c-'A')+10;										// A-F: use hex digit
	}

	*ptr=i;																		// decount characters

	return res;																	// done
}

//----------------------------------------------------------------------------------
// Dispatch segments into blocks
//----------------------------------------------------------------------------------
int CScriptfile::Dispatch(CBlock* pBlock, int wipe)
{
	int i,j,p,size,page,more,morepages,ptr,horiz=0,maxval,toobig;
	CList *pList,*pPageSegs=NULL,*pSegList;
	CSegment *pSeg,*pBest;
	CList Stack;
	CBlock *pBlock2;

	if(pBlock==NULL)															// root
	{
		if(m_Selected.GetSize()>0)												// is there a selection ?
			pList=&m_Selected;													// yes: use it
		else if(m_Blocks.m_Blocks.GetSize()>0)									// are there blocks ?
			pList=&m_Blocks.m_Blocks;											// yes: use them in order of appearance
		else
		{																		// no: use default (memory expansion card)
			m_Blocks.AddBlock("HIMEM",0xA000,0xFFFF);
			m_Blocks.AddBlock("LOMEM",0x2000,0x3FFF);
			pList=&m_Blocks.m_Blocks;
		}
	}
	else																		// called from a device or model
		pList=&pBlock->m_Children;												// use device/model's list of children

	if(m_SegList.GetSize()>0)													// get segment list
		pSegList=&m_SegList;													// user-specified
	else
		pSegList=&m_Segments.m_Segments;										// all

	for(i=0;i<pList->GetSize();i++)												// wipe prior assignments
	{
		pBlock2=(CBlock*)pList->GetAt(i);										// get one block
		if((int)pBlock2<0x1000) continue;										// special marks
		if(pBlock2->m_Type!=BT_BLOCK) continue;									// device or model
		pBlock2->m_CurPage=0;													// reset current page
		pBlock2->m_Top=pBlock2->m_Bottom;										// reset load ptr
		if(wipe==0) continue;													// don't wipe anything

		if(g_Verbous>=4) 
		{
			cout << "Wiping existing assigments\n ";
			if(g_Log&0x01)
			{
				g_Logfile << "Wiping existing assigments\n ";
			}
		}
		if(wipe&0x01)															// wipe segment assigments
		{
			for(p=0;p<pBlock2->m_Pages.GetSize();p++)							// wipe previous assignments: iterate list of pages
			{
				pPageSegs=(CList*)pBlock2->m_Pages.GetAt(p);					// get segment list for this page
				for(j=0;j<pPageSegs->GetSize();j++)								// iterate segment list
				{
					pSeg=(CSegment*)pSegList->GetAt(j);							// get one segment
					pSeg->m_Address=0;											// reset address
				}
				pPageSegs->RemoveAll(0);										// remove segment ptrs (don't delete segments)
			}
		}
		if(wipe&0x02)															// wipe stubs
		{
			for(j=0;j<pBlock2->m_Stubs.GetSize();j++)
			{
				pSeg=(CSegment*)pBlock2->m_Stubs.GetAt(j);						// get one segment
				pSeg->m_Address=0;												// reset address
			}
			pBlock2->m_Stubs.RemoveAll(0);										// remove segment ptrs (don't delete segments)		
		}
	}

	for(more=i=0;i<pSegList->GetSize();i++)										// count segments to be dispatched
	{
		pSeg=(CSegment*)pSegList->GetAt(i);										// get one
		if(pSeg->m_Address>0) continue;											// already assigned
		if(pSeg->m_Size==0) continue;											// empty segment
		more++;																	// to be assigned
	}
	if(more==0) return 0;														// no segments left

	for(i=0;(i<pList->GetSize())&&(more>0);i++)									// block assignment loop
	{
		pBlock=(CBlock*)pList->GetAt(i);										// get one block
		if(((int)pBlock)==0x0001)												// detect special codes
		{																		// start horizontal section
			Stack.Add((PTR)i);													// save current block position on stack
			horiz=1;															// flag: horizontal from now on
			morepages=0;														// reset page flag
			continue;															// next block
		}
		else if(((int)pBlock)==0x0002)											// end horizontal section
		{
			if(Stack.GetSize()<=0) continue;									// ignore, if noting on stack
			if(morepages)														// there are more pages to be scanned inside horizontal section
			{
				i=(int)Stack.GetAt(Stack.GetSize()-1);							// get position of last { from stack
				morepages=0;													// reset page flag
				continue;														// restart from block following { mark
			}
			Stack.RemoveAt(Stack.GetSize()-1);									// remove this section from the stack
			if(Stack.GetSize()==0) horiz=0;										// no more horizontal sections
			continue;															// continue with next block after } mark	
		}
		else if(((int)pBlock)==0x0003)											// force vertical for next block
		{
			if(horiz>0) horiz=2;												// set flag (only valid in horizontal section)
			continue;															// get next block
		}
		if(pBlock->m_Type!=BT_BLOCK)											// devices and models
		{
			more=Dispatch(pBlock,wipe);											// recursive call (ignores horizontal flag)
			continue;
		}
		else if(((int)pBlock)<0x1000) continue;									// skip other non-pointer values
		if(wipe&0x04) continue;													// do nothing but wiping

		p=pBlock->m_CurPage;													// current page number
		if((p<0)&(horiz>0))														// forced vertical block, already done
		{	
			horiz=1;															// back to horizontal mode
			continue;															// skip this block														
		}

		page=pBlock->m_Page;													// first page
		if(pBlock->m_Page<pBlock->m_ToPage)										// increment or decrement page number to current page
		{
			if(pBlock->m_Step==0) pBlock->m_Step=1;								// safety check
			page+=(pBlock->m_Step*p);
		}
		else 
		{
			if(pBlock->m_Step==0) pBlock->m_Step=-1;							// safety check
			page-=(pBlock->m_Step*p);
		}

		while(more)																// page loop
		{
			pBlock->m_Top=pBlock->m_Bottom;										// reset loading pointer
			size=pBlock->m_ToAddr-pBlock->m_Address+1;							// block size
			if(size<0) size=-size;												// absolute value
			size-=pBlock->GetPageSize(p);										// minus stub and already loaded code
			pPageSegs=(CList*)pBlock->m_Pages.GetAt(p);							// in case page already exists (NULL otherwise)

			for(j=toobig=0,pBest=NULL,maxval=0;;j++)							// segment loop
			{
				if(j<pSegList->GetSize())										// inside the list
				{
					if(m_Strategy==-1)											// strategy = last
						pSeg=(CSegment*)pSegList->GetAt(pSegList->GetSize()-j-1);	// get one segment, counting from end
					else
						pSeg=(CSegment*)pSegList->GetAt(j);						// get one segment
					if(pSeg->m_Address>0) continue;								// already assigned
					if(pSeg->m_Size==0) continue;								// empty segment
					if(pBlock->m_Condition!=NULL)								// parse block assignment rules
					{
						ptr=0;
						if(!Term(pSeg,pBlock->m_Condition,&ptr)) continue;		// condition not verified
					}
					toobig=1;
					if(pSeg->m_Size>size) continue;								// too big for this page

					if(m_Strategy==-2)											// strategy = largest
					{
						if((pBest==NULL)||(pSeg->m_Size>pBest->m_Size))			// first or larger than current
							pBest=pSeg;											// make it current biggest
						if(j<pSegList->GetSize()-1)	continue;					// end of list not reached
						j=-1;													// to restart from 0 next iteration
						pSeg=pBest;												// largest of all
						pBest=NULL;												// reset ptr for next time
					}
					else if(m_Strategy==-3)										// strategy = smallest
					{
						if((pBest==NULL)||(pSeg->m_Size<pBest->m_Size))			// first or smaller than current
							pBest=pSeg;											// make it current smallest
						if(j<pSegList->GetSize()-1)	continue;					// end of list not reached
						j=-1;													// to restart from 0 next iteration
						pSeg=pBest;												// smallest of all
						pBest=NULL;												// reset ptr for next time
					}
					else if(m_Strategy>0)										// strategy = flagval
					{
						if((pBest==NULL)||(pSeg->m_Flags&m_Strategy>maxval))	// first or better than current
						{
							pBest=pSeg;											// make it current best
							maxval=pSeg->m_Flags&m_Strategy;					// remember top value so far
						}
						if(j<pSegList->GetSize()-1)	continue;					// end of list not reached
						j=-1;													// to restart from 0 next iteration
						pSeg=pBest;												// largest of all
						pBest=NULL;												// reset ptr for next time
					}
				}
				else if(pBest!=NULL)											// list done (last segment rejected)
				{
					pSeg=pBest;													// proceed with largest
					pBest=NULL;													// don't do it again
				}
				else break;														// exit segment loop

				// assign segment to this page
				if(pPageSegs==NULL)												// no pointer yet
				{
					pPageSegs=new CList();										// create a new segment list
					pBlock->m_Pages.Add((PTR)pPageSegs);						// add it to block list
				}
				
				if(pBlock->m_Address<pBlock->m_ToAddr)							// load from bottom
				{
					pSeg->m_Address=pBlock->m_Top;								// assign segment to bottom of block
					pBlock->m_Top+=pSeg->m_Size;								// update ptr
					pPageSegs->Add((PTR)pSeg);									// add segment to list in block
				}
				else															// load at the top
				{
					pBlock->m_Top-=pSeg->m_Size;								// update ptr
					pSeg->m_Address=pBlock->m_Top+1;							// assign segment to top of block
					pPageSegs->InsertAt((PTR)pSeg,0);							// add segment at top of list
				}
				size-=pSeg->m_Size;												// remaining size in this page
				if(g_Verbous>=4) 
				{
					cout.setf(ios::hex|ios::uppercase);
					cout << "Dispatching " << pSeg->m_Name << " to block " << pBlock->m_Name << "[" << page << "] at >" << pSeg->m_Address << "\n";
					cout.unsetf(ios::hex|ios::uppercase);
					if(g_Log&0x01)
					{
						g_Logfile.setf(ios::hex|ios::uppercase);
						g_Logfile << "Dispatching " << pSeg->m_Name << " to block " << pBlock->m_Name << "[" << page << "] at >" << pSeg->m_Address << "\n";
						g_Logfile.unsetf(ios::hex|ios::uppercase);
					}
				}

				pSeg->m_Page=page;												// assign page
				more--;															// uncount this segment (to stop when all assigned)
			} // segment loop

			if((page==pBlock->m_ToPage)||(toobig==0))							// no more pages or no appropriate segment
			{
				pBlock->m_CurPage=-1;											// flag: don't scan this block any more
				break;															// exit page loop
			}

			p++;																// next page in list
			morepages=1;														// set flag for horizontal mode
			pPageSegs=NULL;														// to create new segment list
			if(pBlock->m_Page<pBlock->m_ToPage)									// increment or decrement page number
				page+=pBlock->m_Step;			
			else 
				page-=pBlock->m_Step;

			if(horiz==1)														// horizontal mode
			{
				pBlock->m_CurPage=p;											// remember current page number
				break;															// next block: exit page loop
			}
			else if(horiz==2)
				pBlock->m_CurPage=-1;											// flag not to repeat vertical blocks inside horizontal section
		} // page loop
		if(horiz==2) horiz=1;													// previous block was forced vertical inside horizontal section
	} // block loop

	return more;																// in case of recursive call
}

//----------------------------------------------------------------------------------
// Parse logical term
//----------------------------------------------------------------------------------
int CScriptfile::Term(CSegment* pSeg, char* pBuf, int* ptr)
{
	char c,Label[9],unary;
	int n=*ptr,value,res=0;

	while(1)
	{
		for(;(pBuf[n]<=' ')||pBuf[n]==')';n++)									// skip spaces
		{
			if((pBuf[n]==')')||(pBuf[n]==0x00)||(pBuf[n]==0x0A)||(pBuf[n]==0x0D))	// end of clause/string/line
			{
				*ptr=n+1;														// decount characters used
				return res;														// return result
			}
		}

		unary=pBuf[n];															// get next char
		if((unary=='~')||(unary=='-')||(unary=='+')) n++;						// valid unary operator

		for(;pBuf[n]<=' ';n++)													// skip spaces
		{
			if((pBuf[n]==0x00)||(pBuf[n]==0x0A)||(pBuf[n]==0x0D))				// end of string/line
			{
				*ptr=n+1;														// decount characters used
				return res;														// return result
			}
		}

		if(pBuf[n]=='(')														// start of clause
		{
			n++;																// skip the ( sign
			res=Term(pSeg,pBuf,&n);												// appraise clause										
		}
		else
		{
			if(pBuf[n]=='>')													// hexadecimal number
			{
				n++;															// skip > mark
				value=GetHex(4,pBuf,&n);										// grab number
			}
			else if((pBuf[n]>='0')&&(pBuf[n]<='9'))								// decimal number
			{
				value=GetNumber(5,pBuf,&n);										// grab number
			}
			else
			{
				GetLabel(Label,8,pBuf,&n);										// grab flag name								
				value=m_Equates.GetValue(Label);								// get its value from linker's labels
				if(value<0)														// not found
				{
					value=m_Defs.GetValue(Label);								// try DEF labeld
					if(value<0)													// still not found
					{
						cout << "Label " << Label << " not found in BLOCK condition\n";
						if(g_Log&0x02) g_Logfile << "Label " << Label << " not found in BLOCK condition\n";
						value=0;												// error												
					}
				}
			}
			if(unary=='-') value=-value;										// in case it's negated
			res=((pSeg->m_Flags&value)!=0);										// calculate result
		}
		if(unary=='~') res=!res;												// in case it's inverted

		for(;pBuf[n]<=' ';n++)													// skip spaces
		{
			if((pBuf[n]==0x00)||(pBuf[n]==0x0A)||(pBuf[n]==0x0D))				// end of string/line
			{
				*ptr=(*ptr)+n+1;												// decount characters used
				return res;														// return result
			}
		}

		c=pBuf[n++];															// get next char
		switch(c)
		{
		case '&':																// AND operator
			res &=Term(pSeg,pBuf,&n);
			break;
		case '|':																// OR operator
			res |=Term(pSeg,pBuf,&n);
			break;
		case '^':																// XOR operator
			res ^=Term(pSeg,pBuf,&n);
			break;
		case ')':																// end of clause
			*ptr=n+1;															// skip the ) sign
			return res;
		default:																// illegal char
			cout << "Illegal character in logic expression: " << c <<"\n";
			if(g_Log&0x02) g_Logfile << "Illegal character in logic expression: " << c <<"\n";
			return 0;
		}
	} // while

	return res;																	// return result
}

//----------------------------------------------------------------------------------
// Match strings with wildcards
//----------------------------------------------------------------------------------
int CScriptfile::Match(char* s, char* Pattern)
{
	int i,j,next='.';

	for(i=0,j=0;s[i]>0x00;i++,j++)
	{
		switch(Pattern[j])														// wildcards
		{
		case 0x00:																// end of pattern string
			if(s[i]==0x00) return 1;											// announce success
			break;																// incomplete: announce mismatch
		case '$':																// $ any label letter (or $ itself)
			if((s[i]>='A')&&(s[i]<='Z')) continue;								// match, next char, next pattern
			if(s[i]=='$') continue;
			if(s[i]=='_') continue;
			break;																// mismatch: exit switch
		case '#':																// # any decimal digit
			if((s[i]>='0')&&(s[i]<='9')) continue;
			break;
		case '>':																// > any hex digit
			if((s[i]>='0')&&(s[i]<='9')) continue;
			if((s[i]>='A')&&(s[i]<='F')) continue;
			break;
		case '?':																// ? any label character
			if((s[i]>='0')&&(s[i]<='9')) continue;
			if((s[i]>='A')&&(s[i]<='Z')) continue;
			if(s[i]=='$') continue;
			if(s[i]=='_') continue;
			break;
		case '+':																// + 0 or more of the previous pattern
			if(Match(s+i,Pattern+j+1)) return 1;								// match current char with next pattern (0 occurences)
			next=Pattern[j+1];													// in case pattern is finished
			if(j>0)																// on first char + is same as *
			{
				i--;															// match this char again
				j-=2;															// with previous pattern char
				continue;
			}																	// fall through
		case '*':																// * 0 or more of any char
			if(Match(s+i,Pattern+j+1)) return 1;								// match current char with next pattern (0 occurences)
			next=Pattern[j+1];													// in case pattern is finished
			j--;																// next char, stay on *
			continue;															// always match
			break;
		case '\\':																// escape character 
			j=j+1;																// use next patter char as regular text
		default:																// regular charater in pattern
			if(s[i]==Pattern[j]) continue;										// match
			break;																// mismatch
		}
		return 0;																// mismatch
	}

	if(Pattern[j]==0x00) return 1;												// string is finished, pattern too													
	if((Pattern[j]=='*')&&(next==0x00)) return 1;								// pattern ends with *
	if((Pattern[j]=='+')&&(next==0x00)) return 1;								// pattern ends with +

	return 0;																	// more pattern to come: mismatch
}

//----------------------------------------------------------------------------------
// Check if a string contains wildcards
//----------------------------------------------------------------------------------
int CScriptfile::HasWildcards(char* s)
{
	int i;

	for(i=0;s[i]>0x00;i++)														// iterate the string
	{
		if((s[i]=='$')||(s[i]=='#')||(s[i]=='>')||(s[i]=='?')||(s[i]=='+')||(s[i]=='*')) return 1; // found a wildcard
	}

	return 0;																	// none found
}

//----------------------------------------------------------------------------------
// Implements the Find function
//----------------------------------------------------------------------------------
int CScriptfile::Find(char* pLine)
{
	int ptr=0,Value,i,j;
	char Ftype[4],Name[7];
	char c1,c2;
	CBlock* pBlock;
	CSegment* pSegment;
	CLabel *pLabel, *pLabel2;

	GetParam(Ftype,8,pLine,&ptr);											// grab type of search S LD LR LU LE LA MM MD MB MP

	c1=Ftype[0];
	if((c1=='1')||(c1=='2')) 												// skip pass number, if any
	{
		c1=Ftype[1];
		c2=Ftype[2];
	}
	else
	{
		c1=Ftype[0];
		c2=Ftype[1];
	}
	
	GetParam(Name,7,pLine,&ptr);											// grab name to search for (optional)

	Value=1;																// by default: first found
	if(pLine[ptr]==',')														// optional param
	{
		Value=GetNumber(5,pLine,&ptr);										// grab occurence/page to find
	}


	m_Equates.SetValue("$COUNT",0);											// in case not found
	m_Equates.SetValue("$SIZE",0);					
	m_Equates.SetValue("$ADDR",0);
	m_Equates.SetValue("$PAGE",0);
	m_Equates.SetValue("$TYPE",0);
	switch(c1)
	{
	case 'S':																// find segment
		if(Name[0]!=0x00)													// name provided
		{
			pSegment=m_Segments.Find(Name);									// find segment by name
			if(pSegment==NULL)												// name not found
			{
				if(g_Verbous>0)
				{
					cout << "Segment " << Name << " not found \n";						
					if(g_Log&0x02) g_Logfile << "Segment " << Name << " not found \n";;
				}
				return 2;													// end with warning, set count as 0
				m_Equates.SetValue("$COUNT",1);								// set total count as 1
			}
		}
		else																// find segment by occurence
		{
			pSegment=m_Segments.Find(Name[1],&Value);						// find a given occurence
			m_Equates.SetValue("$COUNT",Value);								// set total count
			if(pSegment==NULL)												// occurence not found
			{
				if(g_Verbous>0)
				{
					cout << "Occurence not found \n";						
					if(g_Log&0x02) g_Logfile << "Occurence not found \n";;
				}
				return 0;													// this is not an error 
			}
		}
		if((pSegment!=NULL)&&(g_Verbous>1))									// segment found
		{
			m_Equates.SetValue("$SIZE",pSegment->m_Size);					// pass info into variables (size, address, page)
			m_Equates.SetValue("$ADDR",pSegment->m_Address);
			m_Equates.SetValue("$PAGE",pSegment->m_Page);
			cout.setf(ios::hex|ios::uppercase);								// report on screen
			cout << "Found segment " << pSegment->m_Name << "\n";
			cout << "Page: >"<< pSegment->m_Page << "\n";
			cout << "Address: >"<< pSegment->m_Address << "\n";
			cout << "Size: >"<< pSegment->m_Size << "\n";
			cout.unsetf(ios::hex|ios::uppercase);
			if(g_Log&0x02)													// report to log file
			{
				g_Logfile.setf(ios::hex|ios::uppercase);
				g_Logfile << "Found segment " << pSegment->m_Name << "\n";
				g_Logfile << "Page: >"<< pSegment->m_Page << "\n";
				g_Logfile << "Address: >"<< pSegment->m_Address << "\n";
				g_Logfile << "Size: >"<< pSegment->m_Size << "\n";
				g_Logfile.unsetf(ios::hex|ios::uppercase);
			}
		}
		break;

	case 'L':																// find label
		pLabel2=NULL;
		if(Name[0]!=0x00)													// name provided
		{
			if((c2=='E')||(c2=='A'))										// equates or any
				pLabel2=m_Equates.Find(Name);								// find label by name
			if((pLabel2==NULL)&&((c2=='D')||(c2=='A')))						// DEFs or any
				pLabel2=m_Defs.Find(Name);								
			if((pLabel2==NULL)&&((c2=='R')||(c2=='U')||(c2=='A')))			// REFs, undef or any
				pLabel2=m_Refs.Find(Name,c2=='U');									
			if(pLabel2==NULL)
			{
				if(g_Verbous>0)
				{
					cout << "Label " << Name << " not found \n";						
					if(g_Log&0x02) g_Logfile << "Label " << Name << " not found \n";;
				}
				return 0;													// this is not an error 
			}
			m_Equates.SetValue("$COUNT",1);									// found set count as 1
		}
		else																// no name, find by occurence
		{
			j=0;
			if((c2=='E')||(c2=='A'))										// equates or any
			{
				pLabel=m_Equates.OutputTable("E",1);						// find label by type 
				if(pLabel)													// found
				{
					j++;													// count it
					if(j==Value) pLabel2=pLabel;							// remember this one //  get total from OutputTable
				}
			}
			if((c2=='D')||(c2=='A'))										// DEFs or any
			{
				pLabel=m_Defs.OutputTable("56W",1);							// find label by type 
				if(pLabel)													// found
				{
					j++;													// count it
					if(j==Value) pLabel2=pLabel;							// remember this one
				}
			}
			if((c2=='R')||(c2=='A'))										// REFs or any
			{
				pLabel=m_Defs.OutputTable("34VXYZ",1);						// find label by type 
				if(pLabel)													// found
				{
					j++;													// count it
					if(j==Value) pLabel2=pLabel;							// remember this one
				}
			}
			m_Equates.SetValue("$COUNT",j);									// total label searched					
			if(pLabel2==NULL)												// occurence not found
			{
				if(g_Verbous>0)
				{
					cout << "Occurence not found \n";						
					if(g_Log&0x02) g_Logfile << "Occurence not found \n";
					return 0;
				}
			}
		}			
		if((pLabel2!=NULL)&&(g_Verbous>1))									// label found
		{
			m_Equates.SetValue("$TYPE",pLabel2->m_Type);					// pass label info (value, tag)into equates					
			cout.setf(ios::hex|ios::uppercase);								// report result on screen
			cout << "Found label " << pLabel2->m_Name << "\n";
			cout << "Tag: "<< pLabel2->m_Type << "\n";
			if(pLabel2->m_Undefined)
			{
				m_Equates.SetValue("$ADDR",0x7FFFFFFF);						// code for undef
				cout << "Currently undefined\n";
			}
			else
			{
				m_Equates.SetValue("$ADDR",pLabel2->m_Value);
				cout << "Value: >"<< pLabel2->m_Value << "\n";
			}
			cout.unsetf(ios::hex|ios::uppercase);
			if(g_Log&0x02)													// report to log file
			{
				g_Logfile.setf(ios::hex|ios::uppercase);
				g_Logfile << "Found label " << pLabel2->m_Name << "\n";
				g_Logfile << "Tag: "<< pLabel2->m_Type << "\n";
				if(pLabel2->m_Undefined)
					g_Logfile << "Currently undefined\n";
				else
					g_Logfile << "Value: >"<< pLabel2->m_Value << "\n";
				g_Logfile.unsetf(ios::hex|ios::uppercase);
			}
		}
		break;

	case 'M':																// find memory 
		if(Name[0]!=0x00)													// name provided
		{
			switch(c2)
			{
			case 'A':														// all
				i=0;
				break;
			case 'B':														// block
			case 'P':														// page
				i=BT_BLOCK;
				break;
			case 'D':														// device
				i=BT_DEVICE;
				if(Value==0) Value=1;	
				break;
			case 'M':														// model
				i=BT_MODEL;
				break;
			default:														// illegal
				if(g_Verbous>0)
				{
					cout << "Illegal memory item for FIND \n";						
					if(g_Log&0x02) g_Logfile << "Illegal memory item for FIND \n";
				}
				return 1;
			}
			pBlock=m_Blocks.Find(Name,i);									// find by name and type									
			if(pBlock==NULL)
			{
				if(g_Verbous>0)
				{
					cout << "Memory item " << Name << " not found \n";						
					if(g_Log&0x02) g_Logfile << "Memory item " << Name << " not found \n";;
				}
				return 0;													// this is not an error 
			}
			m_Equates.SetValue("$COUNT",1);
		}
		else																// no name, find by occurence and type
		{
			pBlock=m_Blocks.OutputTable(Ftype,&Value);						// find by type and occurence
			m_Equates.SetValue("$COUNT",Value);								// set total count
			if(pBlock==NULL)												// not found
			{
				if(g_Verbous>0)
				{
					cout << "Occurence not found \n";						
					if(g_Log&0x02) g_Logfile << "Occurence not found \n";;
				}
				return 0;													// this is not an error 
			}
		}
		if((pBlock!=NULL)&&(g_Verbous>1))									// block found					
		{
			cout << "Found memory item " << pBlock->m_Name << "\n";			// report to screen
			cout << "Pages used: "<< pBlock->m_Pages.GetSize() << "\n";
			if(pBlock->GetPageSize(-1))
				cout << "Stub size: " << pBlock->GetPageSize(-1) << "\n";
			if(c2=='B')														// block
			{
				m_Equates.SetValue("$PAGE",pBlock->m_Pages.GetSize());		// pass # of pages in $PAGE
				m_Equates.SetValue("$SIZE",pBlock->GetPageSize(-1));		// pass stub size in $SIZE
				for(i=0;i<pBlock->m_Pages.GetSize();i++)
					cout << "Page " << i << " contents: " << pBlock->GetPageSize(i) << " bytes\n";
			}
			else															// page
			{
				    m_Equates.SetValue("$SIZE",pBlock->GetPageSize(Value));
					m_Equates.SetValue("$PAGE",Value);						// pass page # in $PAGE
					cout << "Page " << Value << " contents: " << pBlock->GetPageSize(Value) << " bytes\n";
			}
			if(g_Log&0x02)													// report to log file
			{
				g_Logfile << "Found memory item " << pBlock->m_Name << "\n";
				g_Logfile << "Pages used: "<< pBlock->m_Pages.GetSize() << "\n";
				if(pBlock->GetPageSize(-1))
					g_Logfile << "Stub size: " << pBlock->GetPageSize(-1) << "\n";
				if(c2=='B')													// block
				{
					for(i=0;i<pBlock->m_Pages.GetSize();i++)
						g_Logfile << "Page " << i << " contents: " << pBlock->GetPageSize(i) << " bytes\n";
				}
				else														// page
					g_Logfile << "Page " << Value <<" contents: " << pBlock->GetPageSize(Value) << " bytes\n";
			}
		}
		break;

	default:																// illegal search
		if(g_Verbous>0)
		{
			cout << "Illegal item type for FIND \n";						// may be optional
			if(g_Log&0x02) g_Logfile << "Illegal item type for FIND \n";
		}
		return 2;
	}

	return 0;
}

