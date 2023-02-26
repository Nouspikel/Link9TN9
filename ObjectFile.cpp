#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include "ObjectFile.h"

extern int g_ErrLevel;
extern int g_Verbous;
extern ofstream g_Logfile;
extern int g_Log;

/////////////////////////////////////////////////////////////////////////////////////
// CObjectfile class. Handles tagged object files
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Default constuctor
//----------------------------------------------------------------------------------
CObjectfile::CObjectfile()
{
	m_Filename=NULL;
}

//----------------------------------------------------------------------------------
// Constuctor, with file name
//----------------------------------------------------------------------------------
CObjectfile::CObjectfile(char* filename)
{
	m_File.open(filename,ios::nocreate);
	if(!m_File)
	{
		cout << "Cannot open object file " << filename << "\n";
		if(g_Log&0x02) g_Logfile << "Cannot open object file " << filename << "\n";
		return;
	}
	m_Filename=_strdup(filename);										// save filename
	strcpy(m_PsegName,"$PSEG");											// initialize default PSEG name
}

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CObjectfile::~CObjectfile()
{
	if(m_Filename!=NULL) free(m_Filename);								// free memory allocated by _strdup
}

//----------------------------------------------------------------------------------
// Rewind file to a given position
//----------------------------------------------------------------------------------
void CObjectfile::Seek(int offset)
{
	m_File.clear();														// clear failbit (just in case)
	m_File.seekg(offset,ios::beg);										// rewrind
}

//----------------------------------------------------------------------------------
// Get hexadecimal value
//----------------------------------------------------------------------------------
int CObjectfile::GetHex(int count)
{
	int i,res=0;
	char c;

	for(i=0;i<count;i++)
	{
		m_File.get(c);
		if(c<'0') return -1;
		else if(c<='9') res=(res*16)+(int)(c-'0');
		else if((c<'A')||(c>'F')) return -1;
		else res=(res*16)+(int)(c-'A')+10;
	}
	return res;
}

//----------------------------------------------------------------------------------
// Get label name
//----------------------------------------------------------------------------------
int CObjectfile::GetLabel(char* pBuf, int size)	
{
	int i;

	m_File.read(pBuf,size);

	for(i=0;(i<size)&&(pBuf[i]>' ');i++);
	pBuf[i]=0;
	return (m_File.gcount()==size);
}

//----------------------------------------------------------------------------------
// First pass, grab segments and labels
//----------------------------------------------------------------------------------
int CObjectfile::FirstPass(CSegmentList *pSegments, CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs, char *PsegName)
{
	char tag;
	char ModuleName[9];
	char Junk[9];
	char ProgramID[9];
	char Name[7];
	int size,number,value,current;
	CLabel *pLab;
	CAorg *pAorg=NULL;

	m_File.get(tag);
	if(tag=='>') return 2;							// text libraries: link to object file				
	else if(tag!='0')
	{
		cout << "Wrong object file format (no tag 0)\n";
		if(g_Log&0x02) g_Logfile << "Wrong object file format (no tag 0)\n";
		return 1;
	}
	size=GetHex();
	if(size<0)
	{
		cout << "Wrong object file format (no PSEG size)\n";
		if(g_Log&0x02) g_Logfile << "Wrong object file format (no PSEG size)\n";
		return 1;
	}
	if(!GetLabel(ModuleName,8))
	{
		cout << "Wrong object file format (no module name)\n";
		if(g_Log&0x02) g_Logfile << "Wrong object file format (no module name)\n";
		return 1;
	}

	pSegments->Append(PsegName,size);				// create default program segment
	strcpy(m_PsegName,PsegName);					// remember for 2nd pass, in case if waas renamed with SEGMENT command

	while(!m_File.eof())
	{
		m_File.get(tag);
		number=0;									// trick for multiple entries
		if(g_Verbous>4) 
		{
			cout << "tag" << tag << "\n";
			if(g_Log&0x01) g_Logfile << "tag" << tag << "\n";
		}

		switch(tag)
		{
		case 'M':									// segment declaration
			size=GetHex();							// segment size
			if(size<0)
			{
				cout<<"Wrong segment size (tag M)\n";
				if(g_Log&0x02) g_Logfile<<"Wrong segment size (tag M)\n";
				return 1;
			}
			if(!GetLabel(Name))						// segment name
			{
				cout<<"Wrong segment name (tag M)\n";
				if(g_Log&0x02) g_Logfile<<"Wrong segment name (tag M)\n";
				return 1;
			}
			number=GetHex();						// local number
			if(number<0)
			{
				cout<<"Wrong segment number (tag M)\n";
				if(g_Log&0x02) g_Logfile<<"Wrong segment number (tag M)\n";
				return 1;
			}
			pSegments->Append(Name,size,'M',number);	// create or update segment
			if(g_Verbous>=4) 
			{
				cout << "New segment " << Name << "\n";
				if(g_Log&0x01) g_Logfile << "New segment " << Name << "\n";
			}
			break;
		case '3':									// REF in PSEG
		case 'V':									// SREF in PSEG
			number--;								// segment number will be -2
		case '4':									// REF in AORG
		case 'Y':									// SREF in AORG
			number--;								// segment number will be -1
		case 'X':									// REF in CSEG or DSEG
		case 'Z':									// SREF in CSEG or DSEG
			value=GetHex();
			if(value<0)
			{
				cout<<"Illegal label chain (tag "<< tag << ")\n";
				if(g_Log&0x02) g_Logfile<<"Illegal label chain (tag "<< tag << ")\n";
				return 1;
			}
			if(!GetLabel(Name))						// segment name
			{
				cout<<"Wrong label name (tag "<< tag << ")\n";
				if(g_Log&0x02) g_Logfile<<"Wrong label name (tag "<< tag << ")\n";
				return 1;
			}
			if((tag=='X')||(tag=='Z'))				// only for CSEG or DSEG
			{
				number=GetHex();					// segment number
				if(number<0)
				{
					cout << "Illegal segment number (tag "<< tag << ")\n";
					if(g_Log&0x02) g_Logfile << "Illegal segment number (tag "<< tag << ")\n";
					return 1;
				}
			}
			if((strcmp(Name,"PG4SEG")==0)||(strcmp(Name,"THISPG")==0))  // ignore special linker labels
			{
				if(g_Verbous>=4) 
				{
					cout << "Skipping REF to " << Name << "\n";
					if(g_Log&0x01) g_Logfile << "Skipping REF to " << Name << "\n";
				}
				break;
			}
			if((tag=='3')||(tag=='3'))				// label in PSEG
				pLab=pRefs->Append(Name,-1,tag,pSegments->FindIndex(PsegName));	// create REF label in PSEG or redirected segment
			else
				pLab=pRefs->Append(Name,-1,tag,pSegments->FindIndex(number));	// create REF label, set tag and segment index
			if(g_Verbous>=4) 
			{
				cout << "New REF " << Name << "\n";
				if(g_Log&0x01) g_Logfile << "New REF " << Name << "\n";
			}
			break;
		case '5':									// DEF in PSEG
			number--;								// segment number will be -2
		case '6':									// DEF in AORG
			number--;								// segment number will be -1
		case 'W':									// DEF in CSEG or DSEG 
			value=GetHex();
			if(value<0)
			{
				cout<<"Illegal label value (tag "<< tag << ")\n";
				if(g_Log&0x02) g_Logfile<<"Illegal label value (tag "<< tag << ")\n";
				return 1;
			}
			if(!GetLabel(Name))						// segment name
			{
				cout<<"Wrong label name (tag "<< tag << ")\n";
				if(g_Log&0x02) g_Logfile<<"Wrong label name (tag "<< tag << ")\n";
				return 1;
			}
			if(tag=='W')							// only for CSEG or DSEG
			{
				number=GetHex();					// segment number
				if(number<0)
				{
					cout<<"Illegal segment number (tag "<< tag << ")\n";
					if(g_Log&0x02) g_Logfile<<"Illegal segment number (tag "<< tag << ")\n";
					return 1;
				}
			}
			if(tag=='5')							// PSEG
				pLab=pDefs->Append(Name,value,tag,pSegments->FindIndex(PsegName));	// find or create DEF label in PSEG or redirected segment
			else
				pLab=pDefs->Append(Name,value,tag,pSegments->FindIndex(number));	// find or create DEF label, set value, tag and segment index
			if(g_Verbous>=4) 
			{
				cout << "New DEF " << Name << "\n";
				if(g_Log&0x01) g_Logfile << "New DEF " << Name << "\n";
			}
			break;
		case '9':									// new AORG pointer
			current=GetHex();						// get it
			pAorg=pAorgs->Append(current);			// increase slot
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "AORG >" << current << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile <<"AORG >" << current << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;
		case 'B':			// data word
			value=GetHex();
			if(pAorg!=NULL)
			{
				pAorg->Include(current);
				current+=2;
			}
			break;
		case '1':			// autostart in AORG
		case '2':			// autostart in PSEG
		case '7':			// checksum
		case '8':			// ignore checksum
		case 'A':			// new PSEG pointer
		case 'C':			// reloc data word
		case 'D':			// replace offset with data
		case 'S':			// new DSEG ptr
		case 'T':			// DSEG data
			GetHex();		
			break;
		case 'F':			// end of record
			break;	
		case 'I':			// program ID
			GetLabel(ProgramID,8);
			break;
		case 'N':			// CSEG data
		case 'P':			// new CSEG pointer
		case 'Q':			// COBOL seg ref
		case 'R':			// repeat count
			GetHex();
			GetHex();
			break;
		case ':':			// end of file
			return 0;
		case 0x0D:			// end of line (PC text format)
		case 0x0A:
			break;
		case 'J':			// sybol table dump
			GetHex();
			GetLabel(Junk);
			GetHex();
			break;
		case 'G':			// reloc symbol table dump
		case 'H':			// abolute symbol table dump
		case 'K':			// external macro ref
		case 'U':			// LOAD instruction
			GetHex();
			GetLabel(Junk);
			break;
		default:
			cout << "Unknown tag: " << tag << " at offset " << m_File.tellg() << "\n";
			if(g_Log&0x02) g_Logfile << "Unknown tag: " << tag << " at offset " << m_File.tellg() << "\n";
			return 1;
		} // switch
	} // while

	return 0;
}

//----------------------------------------------------------------------------------
// Second pass, generate code and solve REF chains
//----------------------------------------------------------------------------------
int CObjectfile::SecondPass(CSegmentList *pSegments, CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs)
{
	char tag;
	int Psize;
	int Start=0;
	char ModuleName[9];
	char Junk[9];
	char ProgramID[9];
	char Name[7];
	int size,number,value,address,link,i,pg4seg,pg4segnb,idx;
	CSegment *pSeg;
	CAorg *pAorg;
	CLabel * pDef;
	
	pSegments->ResetNumbers();							// reset all segment numbers and bottom pointer
	pg4segnb=-1;										// flag: no PG4SEG yet

	m_File.get(tag);									// skip first record
	Psize=GetHex();
	GetLabel(ModuleName,8);
	pSeg=pSegments->Find(m_PsegName);					// always exists (created on first pass)
	pSeg->m_Number=0xFD;								// arbitrary number

	while(!m_File.eof())
	{
		m_File.get(tag);
		number=0;										// trick for multiple entries
		value=0;
		switch(tag)
		{
		case 'M':										// segment declaration
			size=GetHex();								// segment size
			GetLabel(Name);								// segment name
			number=GetHex();							// local number
			pSegments->SetNumber(Name,number);			// assign local number, update bottom pointer
			if(g_Verbous>=4) 
			{
				cout << "Loading segment " << Name << "\n";
				if(g_Log&0x01) g_Logfile << "Loading segment " << Name << "\n";
			}
			break;
		case '2':										// autostart in PSEG
			Start+=pSegments->GetAddress(m_PsegName);	// add PSEG address
		case '1':										// autostart in AORG
			Start+=GetHex();							// save start point							
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "Start at >" << Start << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile <<"Start at >" << Start << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;
		case '9':										// new AORG pointer
			pSeg=NULL;									// no more segment loading
			value=GetHex();								// offset
			if(pAorg==NULL)
			{
				cout<<"Illegal pointer to AORG (tag 9)\n";
				if(g_Log&0x02) g_Logfile<<"Illegal pointer to AORG (tag 9)\n";
				return 1;
			}
			pAorg=pAorgs->SetPtr(value);
			if (pAorg==NULL) return 1;					// adjust its pointer
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "AORG >" << value << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile <<"AORG >" << value << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;										
		case 'A':										// new PSEG pointer
			value=GetHex();								// offset
			pSeg=pSegments->Find(m_PsegName);			// find segment corresponding to PSEG (might have been renamed)
			if(pSeg==NULL)
			{
				cout<<"Illegal pointer to PSEG (tag A)\n";
				if(g_Log&0x02) g_Logfile<<"Illegal pointer to PSEG (tag A)\n";
				return 1;
			}
			if(pSeg->SetPtr(value)>0) return 1;			// adjust its pointer
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "PSEG >" << value << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile <<"PSEG >" << value << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;
		case 'S':										// new DSEG ptr
			value=GetHex();								// grab value
			pSeg=pSegments->Find(0);					// new current segment
			if(pSeg==NULL)
			{
				cout<<"Illegal segment with tag S\n";
				if(g_Log&0x02) g_Logfile<<"Illegal segment with tag S\n";
				return 1;
			}
			if(pSeg->SetPtr(value)>0) return 1;			// adjust its pointer
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "DSEG >" << value << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile <<"DSEG >" << value << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;
		case 'P':										// new CSEG pointer
			value=GetHex();								// grab value
			number=GetHex();							// grab segment number
			pSeg=pSegments->Find(number);				// new current segment
			if(pSeg==NULL)
			{
				cout<<"Illegal segment number with tag P: "<< number << "\n";
				if(g_Log&0x02) g_Logfile<<"Illegal segment number with tag P: "<< number << "\n";
				return 1;
			}
			if(pSeg->SetPtr(value)>0) return 1;			// adjust its pointer
			if(g_Verbous>=4) 
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << "CSEG " << number << " >" << value << "\n";
				cout.unsetf(ios::hex|ios::uppercase);
				if(g_Log&0x01)
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile << "CSEG " << number << " >" << value << "\n";
					g_Logfile.unsetf(ios::hex|ios::uppercase);
				}
			}
			break;
		case 'B':										// data word
			value=GetHex();
			if(pSeg!=NULL)
			{
				if(pSeg->Push(value,0xFF)==0) return 1;	// append to buffer, no segment number
			}
			else
			{
				if(pAorg->Push(value,0xFF)==0) return 1;	// ditto for AORG segments
			}
			break;		
		case 'C':										// PSEG-relative data
			value=GetHex();
			value+=pSegments->GetAddress(m_PsegName);	// add PSEG address
			if(pSeg!=NULL)
			{
				if(pSeg->Push(value,0xFD)==0) return 1;	// append to buffer, no segment number
			}
			else
			{
				if(pAorg->Push(value,0xFD)==0) return 1; // ditto for AORG segments
			}
			break;		
		case 'T':										// DSEG-relative data
			value=GetHex();
			value+=pSegments->GetAddress(0);			// add segment address. DSEG number is 0
			if(pSeg!=NULL)
			{
				if(pSeg->Push(value,0)==0) return 1;	// append to buffer, no segment number
			}
			else
			{
				if(pAorg->Push(value,0)==0) return 1;	// ditto for AORG segments
			}
			break;		
		case 'N':										// CSEG-relative data
			value=GetHex();								// value
			number=GetHex();							// segment number
			value+=pSegments->GetAddress(number);		// add segment address
			if(pSeg!=NULL)
			{
				if(pSeg->Push(value,number)==0) return 1; // append to buffer, no segment number
			}
			else
			{
				if(pAorg->Push(value,number)==0) return 1; // ditto for AORG segments
			}
			break;		
		case '3':										// REF in PSEG
		case 'V':										// SREF in PSEG
			number--;									// segment number will be 0xFE
		case '4':										// REF in AORG
		case 'Y':										// SREF in AORG
			number--;									// segment number will be 0xFF
		case 'X':										// REF in CSEG or DSEG
		case 'Z':										// SREF in CSEG or DSEG
			address=GetHex();							// initial link (offset)
			GetLabel(Name);								// label name#
			if((tag=='X')||(tag=='Z'))
			{
				number=GetHex();						// segment number
				if(number<0)
				{
					cout<<"Illegal segment number (tag "<< tag << ")\n";
					if(g_Log&0x02) g_Logfile<<"Illegal segment number (tag "<< tag << ")\n";
					return 1;
				}
			}
			if((address==0)&&(tag=='4'))				// Rossel bug: label is REF but not used
			{
				if(g_Verbous>=4) 
				{
					cout << "Label " << Name << " REFered but not used\n";
					if(g_Log&0x01) g_Logfile << "Label " << Name << " REFered but not used\n";
				}
				break;									// skip this label
			}
			if(strcmp(Name,"PG4SEG")==0)
			{
				pg4seg=address+pSegments->GetAddress(number);	// remember for later
				pg4segnb=(number&0xFF);
				break;									// skip fot now
			}
			else if(strcmp(Name,"THISPG")==0)
			{
				pSeg=pSegments->Find(number);			// find the segment
				if(pSeg!=NULL)
				{
					if(pSeg->m_Special&0x0001)			// segment is a stub
						pSeg->m_Thispg.Add((PTR)address);	// remember for later
					else							
						value=pSeg->m_Page;				// linker trick: susbstitutes page of current segment
				}
				idx=pSegments->FindIndex(number);		// get segment index
			}
			else
			{
				pDef=pDefs->Find(Name);					// find DEF label by name
				if(pDef==NULL)
				{
					cout << "Label " << Name << " undefined (tag " << tag << ")\n";
					if(g_Log&0x02) g_Logfile << "Label " << Name << " undefined (tag " << tag << ")\n";
					break;
				}
				value=pDef->m_Value;					// get value
				idx=pDef->m_SegIdx;						// get index of parent segment
			}
			if(g_Verbous>=4) 
			{
				cout << "Fixing REF chain " << Name << "\n";
				if(g_Log&0x01) g_Logfile << "Fixing REF chain " << Name << "\n";
			}
			address+=pSegments->GetAddress(number);		// make offset an address
			while(address)								// walk the ref chain until link=0
			{
				link=pSegments->GetData(number,address); // get next link
				if(link<0) return 1;					// error
				i=pSegments->GetSegnumber(number,address);	// get segment number for segment-relative link
				pSegments->SetData(number,address,value,idx);	// replace link with correct value and segment index for the DEF
				if(i!=0xFF)								// link is in a different segment
				{
					number=i;							// continue in new segment
					if(strcmp(Name,"THISPG")==0)		// special case for THISPG and stubs
					{
						pSeg=pSegments->Find(number);
						if(pSeg!=NULL)
						{
							if(pSeg->m_Special&0x0001)	// segment is a stub
								pSeg->m_Thispg.Add((PTR)address);
							else							
								value=pSeg->m_Page;		// change page number
						}
					}
				}
				address=link;							// take the link
			}
			break;
		case '5':										// DEF in PSEG
		case '6':										// DEF in AORG
			GetHex();									// skip these
			GetLabel(Name);
			break;
		case 'W':										// DEF in CSEG or DSEG 
			GetHex();									// skip these
			GetLabel(Name);
			GetHex();								
			break;
		case '7':										// checksum
		case '8':										// ignore checksum
			GetHex();									// skip anyway							
			break;
		case 'D':										// replace offset with data
			GetHex();		
			break;
		case 'F':										// end of record
			break;	
		case 'I':										// program ID
			GetLabel(ProgramID,8);
			break;
		case 'Q':										// COBOL seg ref
		case 'R':										// repeat count
			GetHex();
			GetHex();
			break;
		case ':':										// end of file
			break;
		case 0x0D:										// end of line (PC text format)
		case 0x0A:
			break;
		case 'J':										// sybol table dump
			GetHex();
			GetLabel(Junk);
			GetHex();
			break;
		case 'G':										// reloc symbol table dump
		case 'H':										// abolute symbol table dump
		case 'K':										// external macro ref
		case 'U':										// LOAD instruction
			GetHex();
			GetLabel(Junk);
			break;
		default:										// should never happen (detected at first pass)
			cout << "Unknown tag: " << tag << " at offset " << m_File.tellg() << "\n";
			if(g_Log&0x02) g_Logfile << "Unknown tag: " << tag << " at offset " << m_File.tellg() << "\n";
			return 1;
		} // switch
		if(tag==':') break;								// end of file
	} // while

	if(pg4segnb>=0)										// fix PG4SEG chain: find page corresponding to label in previous word
	{			
		if(g_Verbous>=4) 
		{
			cout << "Fixing PG4SEG chain\n";
			if(g_Log&0x01) g_Logfile << "Fixing PG4SEG chain\n";
		}
		while(pg4seg)									// walk the ref chain until link=0
		{
			link=pSegments->GetData(pg4segnb,pg4seg);	// get next link
			if(link<0) return 1;						// error
			i=pSegments->GetSegnumber(pg4segnb,pg4seg-2);	// get segment index for previous word
			pSeg=(CSegment*)pSegments->GetAt(i);			// get segment at this index
			if(pSeg==NULL) return 1;					// error
			pSegments->SetData(pg4segnb,pg4seg,pSeg->m_Page);	// replace link with page number
			i=pSegments->GetSegnumber(pg4segnb,pg4seg);	// get segment number for segment-relative link
			if(i!=0xFF)									// link is in a different segment
				pg4segnb=i;								// continue in new segment
			pg4seg=link;								// take the link
		}
	}

	return 0;
};

/////////////////////////////////////////////////////////////////////////////////////
// CLibrary class. Handles library files
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Constuctor, with file name
//----------------------------------------------------------------------------------
CLibrary::CLibrary(char* filename)
{
	m_File.open(filename,ios::nocreate);
	if(!m_File)
	{
		cout << "Cannot open library file " << filename << "\n";
		if(g_Log&0x02) g_Logfile << "Cannot open library file " << filename << "\n";
		return;
	}
}

//----------------------------------------------------------------------------------
// Search library for undefined labels
//----------------------------------------------------------------------------------
int CLibrary::Search(CSegmentList *pSegments, CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs, char *PsegName)
{
	char Name[7],Line[81],Filename[256];
	char *pBuf;
	int pos,res;
	struct _stat buf;
	CObjectfile *pObjectfile;

	Line[80]=Name[6]=Filename[255]=0;

	while(!m_File.eof())
	{
		m_File.getline(Line,80);						// read one line
		if(Line[0]!='.') continue;						// find target list record (begins with dot)
		for(pBuf=Line+1;pBuf<Line+74;pBuf+=6)			// grab labels in record
		{
			strncpy(Name,pBuf,6);
			if(pRefs->Find(Name,1)!=NULL)				// found an undefined label with this name
			{
				while(!m_File.eof())					// skip rest of target list
				{
					pos=m_File.tellg();					// remember position
					m_File.getline(Line,80);			// read next line
					if(Line[0]!='.')					// if not a target line
					{
						m_File.seekg(pos,ios::beg);		// go back to this line
						m_Modules.Add((PTR)pos);		// memorize for second pass
						if(g_Verbous>=4)
						{
							cout << "Trigger found: " << Name << "\n";
							if(g_Log&0x01) g_Logfile << "Trigger found: " << Name << "\n";
						}
						break;
					}
				} 
				if((res=FirstPass(pSegments,pDefs,pRefs,pAorgs,PsegName))==0) break;	//  do first pass as with regular object file
				m_Modules.RemoveAt(-1);
				if(res!=2) return 1;					// error (error code 2 means link found: text library)

				GetLabel(Filename,255);					// grab file path
				if(_stat(Filename,&buf)!=0)				// check if file exists
				{										// it does not: issue error
					cout << "Object file " << Filename << " not found\n";	
					if(g_Log&0x02) 
						g_Logfile << "Object file " << Filename << " not found\n";
					return g_ErrLevel;
				}
				pObjectfile=new CObjectfile(Filename);	// create file object
				m_Objectfiles.Add((PTR)pObjectfile);	// memorize for second pass
				pObjectfile->FirstPass(pSegments,pDefs,pRefs,pAorgs,PsegName);	// first pass on this file
				break;									// exit the for loop
			}
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------
// Second pass for libraries
//----------------------------------------------------------------------------------
int CLibrary::SecondPass(CSegmentList *pSegments, CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs)
{
	int i,pos,res;
	CObjectfile *pObjectfile;

	for(i=0;i<m_Modules.GetSize();i++)					// iterate modules loaded on first pass	
	{
		pos=(int)m_Modules.GetAt(i);					// get offset
		Seek(pos);										// go to it  
		res=CObjectfile::SecondPass(pSegments,pDefs,pRefs,pAorgs);	// do second pass as with regular object file
		if(res!=0) return res;							// report error
	}

	for(i=0;i<m_Objectfiles.GetSize();i++)				// iterate object files scanned on first pass (text library)	
	{
		pObjectfile=(CObjectfile*)m_Objectfiles.GetAt(i);	// get object file
		pObjectfile->Seek(0);
		res=pObjectfile->SecondPass(pSegments,pDefs,pRefs,pAorgs);	// do second pass as with regular object file
		if(res!=0) return res;							// report error
	}

	return 0;											// report success
}

