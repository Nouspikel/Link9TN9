#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <malloc.h>
#include "segment.h"
extern ofstream g_Logfile;
extern int g_Log;
extern int g_Verbous;

/////////////////////////////////////////////////////////////////////////////////////
// CSegment class. Holds segment information
/////////////////////////////////////////////////////////////////////////////////////
#define SF_BLANK 0x00080000
#define SF_CSEG 0x00040000
#define SF_DSEG 0x00020000
#define SF_PSEG 0x00010000
#define SF_ROM 0x0001
#define SF_RAM 0x0002
#define SF_NVRAM 0x0004
#define SF_OVL 0x0008
//----------------------------------------------------------------------------------
// Constuctor (empty)
//----------------------------------------------------------------------------------
CSegment::CSegment()
{
	m_Name[0]=0;										// init empty name
	m_Name[6]=0;										// extra 0 at end
	m_Size=0;
	m_Address=0;
	m_Page=0;
	m_Number=0;
	m_Bottom=m_Top=0;
	m_pBuffer=NULL;
	m_pSegnbs=NULL;
	m_Flags=0;
	m_Special=0;
}

//----------------------------------------------------------------------------------
// Constuctor, with name
//----------------------------------------------------------------------------------
CSegment::CSegment(char* Label, int Size, int Tag, int Segnb)
{
	strcpy(m_Name,Label);								// copy name
	m_Name[6]=0;										// extra 0 at end
	m_Size=Size;
	m_Address=0;
	m_Page=0;
	m_Number=Segnb;
	m_Bottom=m_Top=0;
	m_pBuffer=NULL;
	m_pSegnbs=NULL;
	m_Flags=0;
	m_Special=0;
}

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CSegment::~CSegment()
{
	if(m_pBuffer!=NULL) free(m_pBuffer);				// free buffers
	if(m_pSegnbs!=NULL) free(m_pSegnbs);
}

//----------------------------------------------------------------------------------
// Append data to buffer, increment pointer
//----------------------------------------------------------------------------------
int CSegment::Push(int Value, int Segnb)
{
	if(m_Top>=m_Size)									// buffer full
	{
		if(m_Special&0x0002)							// skip already loaded ghost segments
			return m_Top;

		cout << "Segment buffer overflow (segment " << Segnb << ")\n";
		if(g_Log&0x02) g_Logfile << "Segment buffer overflow (segment " << Segnb << ")\n";
		return 0;
	}

	m_pSegnbs[m_Top/2]=(Segnb&0xFF);					// save segment number
	m_pBuffer[m_Top++]=(Value>>8)&0xFF;					// save msb
	m_pBuffer[m_Top++]=(Value&0xFF);					// save lsb

	return m_Top;										// return current size
}

//----------------------------------------------------------------------------------
// Get data from segment buffer
//----------------------------------------------------------------------------------
int CSegment::GetData(int Address, int IsOffset)
{
	if(!IsOffset)
		Address-=m_Address;								// make it segment offset

	if((Address>m_Size)||(Address<0))					// safety check
	{
		cout << "Ref chain: link points outside of segment\n";
		if(g_Log&0x02) g_Logfile << "Ref chain: link points outside of segment\n";
		return -1;										// announce failure
	}
	
	return ((m_pBuffer[Address]<<8)&0xFF00)|(m_pBuffer[Address+1]&0xFF);	// return content of this offset
}

//----------------------------------------------------------------------------------
// Get segment number from buffer
//----------------------------------------------------------------------------------
int CSegment::GetSegnumber(int Address)
{
	Address-=m_Address;									// make it segment offset

	if((Address>m_Size)||(Address<0))					// safety check
	{
		cout << "Ref chain: link points outside of segment\n";
		if(g_Log&0x02) g_Logfile << "Ref chain: link points outside of segment\n";
		return -1;										// announce failure
	}
	
	return (m_pSegnbs[Address/2]&0xFF);					// return segment number for this offset
}

//----------------------------------------------------------------------------------
// Replace data inside segment buffer (pointer unchanged)
//----------------------------------------------------------------------------------
int CSegment::SetData(int address, int value, int segnb, int IsOffset)
{
	if(!IsOffset)
	{
		address-=m_Address;								// make it segment offset
		address+=m_Bottom;								// in case loaded from several files
	}

	if((address>m_Size)||(address<0))					// safety check
	{
		cout.setf(ios::hex|ios::uppercase);
		cout << "Set segment data at >" << address << " points outside of segment " << m_Name <<"\n";
		cout.unsetf(ios::hex|ios::uppercase);
		if(g_Log&0x02)
		{
			g_Logfile.setf(ios::hex|ios::uppercase);
			g_Logfile << "Set segment data at >" << address << " points outside of segment " << m_Name <<"\n";
			g_Logfile.unsetf(ios::hex|ios::uppercase);
		}
		return -1;										// announce failure
	}
	
	m_pBuffer[address]=(value>>8)&0xFF;					// set new value
	m_pBuffer[address+1]=(value&0xFF);
	if(segnb>=0)										// -1 means keep old number
		m_pSegnbs[address/2]=(segnb&0xFF);				// set new segment number

	return 0;											// announce success
}

//----------------------------------------------------------------------------------
// Set data pointer
//----------------------------------------------------------------------------------
int CSegment::SetPtr(int Value)
{
	Value+=m_Bottom;									// in case segment was split

	if(Value>m_Size)
	{
		cout.setf(ios::hex|ios::uppercase);
		cout << "New address >" << Value << " outside segment "<< m_Name << " buffer\n";
		cout.unsetf(ios::hex|ios::uppercase);
		if(g_Log&0x02)
		{
			g_Logfile.setf(ios::hex|ios::uppercase);
			g_Logfile << "New address >" << Value << " outside segment " << m_Name << " buffer\n";
			g_Logfile.unsetf(ios::hex|ios::uppercase);
		}
		return 1;	
	}

	m_Top=Value;										// set pointer

	return 0;											// announce success
}

//----------------------------------------------------------------------------------
// Allocate data buffers
//----------------------------------------------------------------------------------
void CSegment::AllocateBuffers()
{
	if(m_Size<=0) return;								// no size

	if(m_pBuffer!=NULL) free(m_pBuffer);				// free buffers if already allocated
	if(m_pSegnbs!=NULL) free(m_pSegnbs);

	m_pBuffer = (char*) malloc(m_Size);					// size in bytes
	memset(m_pBuffer,0,m_Size);							// zero the entire buffer
	m_pSegnbs = (char*) malloc(m_Size/2);				// one seg number per word
	memset(m_pSegnbs,(char)0xFF,m_Size/2);				// init as no segment
}

/////////////////////////////////////////////////////////////////////////////////////
// CSegmentList class. Manages segments
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CSegmentList::~CSegmentList()
{
	m_Segments.RemoveAll(1);							// remove from list, delete objects
}

//----------------------------------------------------------------------------------
// Append new segment to chain or update its size if it exists
//----------------------------------------------------------------------------------
CSegment* CSegmentList::Append(char *Name, int Size, int Tag, int SegNb)
{
	CSegment *pSeg;

	pSeg=Find(Name);									// see if it exists

	if(pSeg!=NULL)										// it does
	{
		if(pSeg->m_Special&0x0002)						// ghost segment 
		{
			if(pSeg->m_Size!=Size)						// check if size matches (not blocking)
			{
				if(g_Verbous>=1) 
				{
					cout << "Warning: Ghost segment size doesn't match existing segment " << pSeg->m_Name << "\n";
					if(g_Log&0x01) g_Logfile << "Warning: Ghost segment size doesn't match existing segment " << pSeg->m_Name << "\n";
				}
			}
			return pSeg;								// do not update ghost segment size
		}
		pSeg->m_Size+=Size;								// update segment size
		return pSeg;									// return this segment
	}

	pSeg=new CSegment(Name,Size,Tag,SegNb);				// create new segment object
	m_Segments.Add((PTR)pSeg);							// add it to list

	if(strcmp(Name,"$PSEG")==0) pSeg->m_Flags|=SF_PSEG;	// seg flags
	else if(strcmp(Name,"$DATA")==0) pSeg->m_Flags|=SF_DSEG;
	else if(strcmp(Name,"$BLANK")==0) pSeg->m_Flags|=(SF_CSEG|SF_BLANK);
	else pSeg->m_Flags|=SF_CSEG;

	return pSeg;										// return it
}

//----------------------------------------------------------------------------------
// Find a segment by name
//----------------------------------------------------------------------------------
CSegment* CSegmentList::Find(char *Name)
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		if(strcmp(pSeg->m_Name,Name)==0) return pSeg;	// names match: return this segment
	}

	return NULL;										// not found
}

//----------------------------------------------------------------------------------
// Find a segment by name, return its index in the list
//----------------------------------------------------------------------------------
int CSegmentList::FindIndex(char* Name)
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		if(strcmp(pSeg->m_Name,Name)==0) return i;		// names match: return this segment
	}

	return -1;											// not found
}

//----------------------------------------------------------------------------------
// Find a segment by number
//----------------------------------------------------------------------------------
CSegment* CSegmentList::Find(int Number)
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		if(pSeg->m_Number==Number) return pSeg;			// names match: return this segment
	}

	return NULL;										// not found
}

//----------------------------------------------------------------------------------
// Find a segment by number, return its index in the list
//----------------------------------------------------------------------------------
int CSegmentList::FindIndex(int Number)
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		if(pSeg->m_Number==Number) return i;			// names match: return this segment
	}

	return -1;											// not found
}

//----------------------------------------------------------------------------------
// Find segment address, by number
//----------------------------------------------------------------------------------
int CSegmentList::GetAddress(int number)
{
	CSegment *pSeg=Find(number);						// find segment 

	if(pSeg!=NULL) return pSeg->m_Address;				// return its address

	return -1;											// not found
}

//----------------------------------------------------------------------------------
// Find segment address, by name
//----------------------------------------------------------------------------------
int CSegmentList::GetAddress(char* Name)
{
	CSegment *pSeg=Find(Name);							// find segment

	if(pSeg!=NULL) return pSeg->m_Address;				// return its address

	return -1;											// not found
}

//----------------------------------------------------------------------------------
// Set segment pointer, by number
//----------------------------------------------------------------------------------
int CSegmentList::SetPtr(int Number, int Value)
{
	CSegment *pSeg=Find(Number);						// find segment 

	if(pSeg!=NULL) return pSeg->SetPtr(Value);			// forward command to segment

	cout << "Segment number " << Number << " not found\n";
	if(g_Log&0x02) g_Logfile << "Segment number " << Number << " not found\n";
	return 1;
}

//----------------------------------------------------------------------------------
// Set segment pointer, by name
//----------------------------------------------------------------------------------
int CSegmentList::SetPtr(char* Name, int Value)
{
	CSegment *pSeg=Find(Name);							// find segment 

	if(pSeg!=NULL) return pSeg->SetPtr(Value);			// forward command to segment

	cout << "Segment " << Name << " not found\n";
	if(g_Log&0x02) g_Logfile << "Segment " << Name << " not found\n";
	return 1;
}
//----------------------------------------------------------------------------------
// Reset all segment numbers
//----------------------------------------------------------------------------------
void CSegmentList::ResetNumbers()
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		pSeg->m_Number=-1;								// reset segment number
		pSeg->m_Bottom=0;
	}
}

//----------------------------------------------------------------------------------
// Assign segment numbers
//----------------------------------------------------------------------------------
int CSegmentList::SetNumber(char* Name, int SegNb)
{
	CSegment *pSeg=Find(Name);							// find segment

	if(pSeg==NULL) return 1;							// not found
	
	pSeg->m_Number=SegNb;								// set number
	pSeg->m_Top=pSeg->m_Bottom;							// restart from where we stopped (separate files)

	return 0;											// announce success
}

//----------------------------------------------------------------------------------
// Output symbol table
//----------------------------------------------------------------------------------
void CSegmentList::OutputTable()
{
	CSegment *pSeg;
	int i;

	cout << "Segment\tSize\tAddress\tPage \n----------------------------\n";	// header
	cout.setf(ios::hex|ios::uppercase);
	if(g_Log&0x01)										// to logfile, if requested
	{
		g_Logfile << "\nSegment\tSize\tAddress\tPage \n----------------------------\n";	// header
		g_Logfile.setf(ios::hex|ios::uppercase);
	}
	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		cout << pSeg->m_Name << "\t" << pSeg->m_Size << "\t" << pSeg->m_Address << "\t" << pSeg->m_Page << "\n";
		if(g_Log&0x01) g_Logfile << pSeg->m_Name << "\t" << pSeg->m_Size << "\t" << pSeg->m_Address <<" \t" << pSeg->m_Page << "\n";
	}
	cout.unsetf(ios::hex|ios::uppercase);
	if(g_Log&0x01)
	{
		g_Logfile.unsetf(ios::hex|ios::uppercase);
		g_Logfile << "\n";
	}
}

//----------------------------------------------------------------------------------
// Allocate data buffers for all segments
//----------------------------------------------------------------------------------
void CSegmentList::AllocateBuffers()
{
	CSegment *pSeg;
	int i;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		pSeg->AllocateBuffers();						// allocate memory buffer
	}
}

/*//----------------------------------------------------------------------------------
// Push data into segment buffers, by number
//----------------------------------------------------------------------------------
int CSegmentList::Push(int Number,int Value, int Tag)
{
	CSegment *pSeg=Find(Number);						// find segment 

	if(pSeg!=NULL) return pSeg->Push(Value,Tag);		// forward command to segment

	cout << "Segment number " << Number << " not found\n";
	return 0;
}
*/

//----------------------------------------------------------------------------------
// Return ptr to segment by numerical order
//----------------------------------------------------------------------------------
CSegment* CSegmentList::GetAt(int i)
{
	return (CSegment*)m_Segments.GetAt(i);
}

//----------------------------------------------------------------------------------
// Return number of segments in list
//----------------------------------------------------------------------------------
int CSegmentList::GetSize()
{
	return m_Segments.GetSize();
}

//----------------------------------------------------------------------------------
// Get data from a given segment
//----------------------------------------------------------------------------------
int CSegmentList::GetData(int Segnb, int Address, int IsOffset)
{
	CSegment *pSeg=Find(Segnb);									// find segment

	if(pSeg!=NULL) return pSeg->GetData(Address,IsOffset);		// get data from it

	cout << "Segment number " << Segnb << " not found to get data from\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment number " << Segnb << " not found to get data from\n";	// not found

	return -1;
}

//----------------------------------------------------------------------------------
// Get data from a given segment
//----------------------------------------------------------------------------------
int CSegmentList::GetData(char* Segname, int Address, int IsOffset)
{
	CSegment *pSeg=Find(Segname);								// find segment

	if(pSeg!=NULL) return pSeg->GetData(Address,IsOffset);		// get data from it

	cout << "Segment number " << Segname << " not found to get data from\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment number " << Segname << " not found to get data from\n";	// not found

	return -1;
}
//----------------------------------------------------------------------------------
// Set data inside a given segment (by number)
//----------------------------------------------------------------------------------
int CSegmentList::SetData(int Segnb, int Address, int Value, int Tag, int IsOffset)
{
	CSegment *pSeg=Find(Segnb);							// find segment

	if(pSeg!=NULL) return pSeg->SetData(Address,Value,Tag,IsOffset);		// set data in it

	cout << "Segment number " << Segnb << " not found to set data into\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment number " << Segnb << " not found to set data into\n";	// not found

	return -1;
}

//----------------------------------------------------------------------------------
// Set data inside a given segment (by name)
//----------------------------------------------------------------------------------
int CSegmentList::SetData(char* Segname, int Address, int Value, int Tag, int IsOffset)
{
	CSegment *pSeg=Find(Segname);							// find segment

	if(pSeg!=NULL) return pSeg->SetData(Address,Value,Tag,IsOffset);		// set data in it

	cout << "Segment " << Segname << " not found to set data into\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment " << Segname << " not found to set data into\n";	// not found

	return -1;
}


//----------------------------------------------------------------------------------
// Get segment number for segment-relative data
//----------------------------------------------------------------------------------
int CSegmentList::GetSegnumber(int Segnb, int Address)
{
	CSegment *pSeg=Find(Segnb);							// find segment

	if(pSeg!=NULL) return pSeg->GetSegnumber(Address);	// return segment number for address in segment buffer

	cout << "Segment number " << Segnb << " not found\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment number " << Segnb << " not found\n";	// not found

	return -1;
}

//----------------------------------------------------------------------------------
// Get page number for a segment
//----------------------------------------------------------------------------------
int CSegmentList::GetPage(int Segnb)
{
	CSegment *pSeg=Find(Segnb);							// find segment

	if(pSeg!=NULL) return pSeg->m_Page;					// return page number

	cout << "Segment number " << Segnb << " not found to get its page\n";	// not found
	if(g_Log&0x02) g_Logfile << "Segment number " << Segnb << " not found to get its page\n";	// not found

	return -1;
}

//----------------------------------------------------------------------------------
// Find segments by occurence
//----------------------------------------------------------------------------------
CSegment* CSegmentList::Find(char type,int *pCount)		// type not used as impossible to tell from DF80 file
{
	CSegment *pSeg,*pSeg2=NULL;
	int i,tot;

	for(i=0;i<m_Segments.GetSize();i++)					// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		tot++;											// count occurences
		if(i==*pCount) pSeg2=pSeg;						// rememeber this one
	}
	*pCount=tot;

	return pSeg2;
}


//----------------------------------------------------------------------------------
// Check that all segments are assigned
//----------------------------------------------------------------------------------
int CSegmentList::CheckList()
{
	CSegment *pSeg;
	int i,tot;

	for(tot=i=0;i<m_Segments.GetSize();i++)				// iterate list
	{
		pSeg=(CSegment*)m_Segments.GetAt(i);			// get segment pointer
		if((int)pSeg<0x10000) continue;					// gap mark
		if((pSeg->m_Size<=0)&&(g_Verbous>=4))			// check if empty
		{
			cout << "Warning: Segment " << pSeg->m_Name << " has size 0\n";
			if(g_Log&0x01) g_Logfile << "Warning: Segment " << pSeg->m_Name << " has size 0\n";
		}
		if((pSeg->m_Address==0)&&(pSeg->m_Size>0))
		{
			tot++;										// count unassigned
			if(g_Verbous>=4) 
			{
				cout << "Segment " << pSeg->m_Name << " unassigned\n";
				if(g_Log&0x01) g_Logfile << "Segment " << pSeg->m_Name << " unassigned\n";
			}

		}
	}
	return tot;
}


//----------------------------------------------------------------------------------
// Fix REF to THISPG for stub segments (changes at every page)
//----------------------------------------------------------------------------------
int CSegment::FixThispg(int Page)
{
	int address,link,i;

	if(g_Verbous>=4) 
	{
		cout << "Fixing THISPG in page " << Page << "\n";
		if(g_Log&0x01) g_Logfile << "Fixing THISPG in page " << Page << "\n";
	}

	for(i=0;i<m_Thispg.GetSize();i++)
	{
		address=((int)m_Thispg.GetAt(i));			// get one link
		address+=m_Address;					;		// make offset an address
		while(address)								// walk the ref chain until link=0
		{
			link=GetData(address);					// get next link
			if(link<0) return 1;					// error
			SetData(address,Page,0xFF);				// replace link with current page number
			if(GetSegnumber(address)!=0xFF) break;	// link is in a different segment: stop
			address=link;							// take the link
		}
	}
	return 0;
}
