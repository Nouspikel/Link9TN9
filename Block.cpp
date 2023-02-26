#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include "Block.h"
#include "Segment.h"
extern ofstream g_Logfile;
extern int g_Log;

/////////////////////////////////////////////////////////////////////////////////////
// Block class. Holds memory blocks, devices and models
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Constuctor (default)
//----------------------------------------------------------------------------------
CBlock::CBlock()
{
}

//----------------------------------------------------------------------------------
// Destuctor 
//----------------------------------------------------------------------------------
CBlock::~CBlock()
{
	int i;

	for(i=0;i<m_Pages.GetSize();i++)									// delete all segment lists
	{
		delete (CList*)m_Pages.GetAt(i);
	}
}

//----------------------------------------------------------------------------------
// Constuctor, with parameters for block
//----------------------------------------------------------------------------------
CBlock::CBlock(char *Name, int Address, int ToAddr, int Page=0, int ToPage=0, int Step=1)
{
	strcpy(m_Name,Name);
	m_Name[7]=0x00;
	m_Address=m_Bottom=Address;
	m_ToAddr=m_Top=ToAddr;
	m_Page=Page;
	m_ToPage=ToPage;
	m_Step=Step;
	m_Type=BT_BLOCK;
	m_MemType=0;
	m_Condition=NULL;
	m_Overlay=0;
}

//----------------------------------------------------------------------------------
// Constuctor, with parameters for device or model
//----------------------------------------------------------------------------------
CBlock::CBlock(char* Name, int Type)
{
	strcpy(m_Name,Name);
	m_Name[7]=0x00;
	m_Type=Type;
	m_Address=m_ToAddr=m_Bottom=m_Top=m_Page=m_ToPage=0;
	m_Condition=NULL;
}

//----------------------------------------------------------------------------------
// Ulterior definition of block characteristics
//----------------------------------------------------------------------------------
void CBlock::Define(int Address, int ToAddr, int Page, int ToPage, int Step)
{
	m_Address=m_Bottom=Address;
	m_ToAddr=m_Top=ToAddr;
	m_Page=Page;
	m_ToPage=ToPage;
	m_Step=Step;
	m_Type=BT_BLOCK;
	m_Overlay=0;
}

//----------------------------------------------------------------------------------
// Calculate size of a page (in numeric order, -1: stub only)
//----------------------------------------------------------------------------------
int CBlock::GetPageSize(int page)
{
	int i,size,value;
	CList *pList;
	CSegment* pSeg;

	for(i=0,size=0;i<m_Stubs.GetSize();i++)								// calculate total stub size
	{
		pSeg=(CSegment*)m_Stubs.GetAt(i);								// get one segment
		size+=pSeg->m_Size;												// count its size
	}

	if(page<0) return size;												// trick: page -1 means get stub size

	if(page>=m_Pages.GetSize()) return size;							// page does not contain code: return stub size (if any)

	pList=(CList*)m_Pages.GetAt(page);									// ge ptr to list of segments

	for(i=0;i<pList->GetSize();i++)										// iterate segments in list
	{
		pSeg=(CSegment*)pList->GetAt(i);								// get one segment
		if((value=(int)pSeg)<0x10000)									// check if it's a gap
			size+=value;												// count gap size
		else
			size+=pSeg->m_Size;											// count segment size
	}

	return size;														// size of segments + stubs
}

/////////////////////////////////////////////////////////////////////////////////////
// BlockList class. Manages a list of blocks/devices/models
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CBlockList::~CBlockList()
{
	m_Blocks.RemoveAll();
}

//----------------------------------------------------------------------------------
// Add a new block to list
//----------------------------------------------------------------------------------
CBlock* CBlockList::AddBlock(char* Name, int Address, int ToAddr, int Page, int ToPage, int Step)
{
	CBlock* pBlock;

	if(Find(Name,BT_BLOCK)!=NULL)											// check if already exists
	{
		cout << "Block " << Name << " defined twice";						// yes: announce error
		if(g_Log&0x02) g_Logfile << "Block " << Name << " defined twice";						// yes: announce error
		return NULL;
	}

	pBlock = new CBlock(Name,Address,ToAddr,Page,ToPage,Step);					// no: create new 

	m_Blocks.Add((PTR)pBlock);												// add to list

	return pBlock;															// return it
}

//----------------------------------------------------------------------------------
// Find a block by name
//----------------------------------------------------------------------------------
CBlock* CBlockList::Find(char* Name, int Type)
{
	int i;
	CBlock* pBlock;

	for(i=0;i<m_Blocks.GetSize();i++)										// iterate whole list
	{
		pBlock=(CBlock*)m_Blocks.GetAt(i);									// get one block
		if(strstr(pBlock->m_Name,Name)==NULL) continue;						// different name
		if((Type>0)&&(pBlock->m_Type!=Type)) continue;						// different type

		return pBlock;														// found, return it
	}

	return NULL;															// not found
}

//----------------------------------------------------------------------------------
// Find a block by name
//----------------------------------------------------------------------------------
void CBlockList::Reset(int what)
{
	int i;
	CBlock* pBlock;

	for(i=0;i<m_Blocks.GetSize();i++)										// iterate whole list
	{
		pBlock=(CBlock*)m_Blocks.GetAt(i);									// get one block
		if((int)pBlock<0x1000) continue;									// special marks
		if(pBlock->m_Type!=BT_BLOCK) continue;								// device or model
		if(what&0x01) pBlock->m_CurPage=pBlock->m_Page;						// reset current page
	}
}

//----------------------------------------------------------------------------------
// Adds a new device/model to list
//----------------------------------------------------------------------------------
CBlock* CBlockList::AddDevice(char* Name, int Type)
{
	CBlock* pBlock;

	if(Find(Name)!=NULL)													// check if already exists
	{
		cout << "Block/device " << Name << " defined twice";				// yes: announce error
		if(g_Log&0x02) g_Logfile << "Block/device " << Name << " defined twice";				// yes: announce error
		return NULL;
	}

	pBlock = new CBlock(Name,Type);											// no: create new 

	m_Blocks.Add((PTR)pBlock);												// add to list

	return pBlock;															// return it
}

//----------------------------------------------------------------------------------
// Outputs block table
//----------------------------------------------------------------------------------
CBlock* CBlockList::OutputTable(char* Types, int *pCount)
{
	int i,j,tot=0;
	CBlock* pBlock,*pMem=NULL;

	if(pCount==NULL)
	{
		cout << "Block\tType\tContents\n----------------------\n";				// header
		if(g_Log&0x01) g_Logfile << "\nBlock\tType\tContents\n----------------------\n";				// header
	}

	if((Types==NULL)||(strchr(Types,'m'))||(strchr(Types,'M')))
	{
		for(i=0;i<m_Blocks.GetSize();i++)									// list memory models
		{
			pBlock=(CBlock*)m_Blocks.GetAt(i);								// get one
			if(pBlock->m_Type!=BT_MODEL) continue;							// wrong type

			if(pCount!=NULL)												// save info for FIND
			{
				tot++;														// found one
				if(tot==*pCount) pMem=pBlock;								// it's the one we wanted: remember it
			}
			else															// output info
			{
				cout << pBlock->m_Name << "\t MODEL \t";
				if(g_Log&0x01) g_Logfile << pBlock->m_Name << "\t MODEL \t";
				for(j=0;j<pBlock->m_Children.GetSize();j++)
				{
					if(j>0)
					{
						cout << ",";
						if(g_Log&0x01) g_Logfile << ",";
					}
					cout << ((CBlock*)pBlock->m_Children.GetAt(j))->m_Name;
					if(g_Log&0x01) g_Logfile << ((CBlock*)pBlock->m_Children.GetAt(j))->m_Name;
				}
				cout <<"\n";
				if(g_Log&0x01) g_Logfile <<"\n";
			}
		}
	}

	if((Types==NULL)||(strchr(Types,'d'))||(strchr(Types,'D')))
	{
		for(i=0;i<m_Blocks.GetSize();i++)									// list devices
		{
			pBlock=(CBlock*)m_Blocks.GetAt(i);								// get one
			if(pBlock->m_Type!=BT_DEVICE) continue;							// wrong type

			if(pCount!=NULL)												// save info for FIND
			{
				tot++;														// found one
				if(tot==*pCount) pMem=pBlock;								// it's the one we wanted: remember it
			}
			else															// output info
			{
				cout << pBlock->m_Name << "\t DEVICE \t";
				if(g_Log&0x01) g_Logfile << pBlock->m_Name << "\t DEVICE \t";
				for(j=0;j<pBlock->m_Children.GetSize();j++)
				{
					if(j>0)
					{
						cout << ",";
						if(g_Log&0x01) g_Logfile << ",";
					}
					cout << ((CBlock*)pBlock->m_Children.GetAt(j))->m_Name;
					if(g_Log&0x01) g_Logfile << ((CBlock*)pBlock->m_Children.GetAt(j))->m_Name;
				}
				cout <<"\n";
				if(g_Log&0x01) g_Logfile <<"\n";
			}
		}
	}

	if((Types==NULL)||(strchr(Types,'b'))||(strchr(Types,'B')))
	{
		for(i=0;i<m_Blocks.GetSize();i++)									// list blocks
		{
			pBlock=(CBlock*)m_Blocks.GetAt(i);								// get one
			if(pBlock->m_Type!=BT_BLOCK) continue;							// wrong type

			if(pCount!=NULL)												// save info for FIND
			{
				tot++;														// found one
				if(tot==*pCount) pMem=pBlock;								// it's the one we wanted: remember it
			}
			else															// output info
			{
				cout.setf(ios::hex|ios::uppercase);
				cout << pBlock->m_Name << "\t BLOCK \t" << pBlock->m_Address << "," <<pBlock->m_ToAddr << ",";
				cout.unsetf(ios::hex);
				cout <<pBlock->m_Page <<"," << pBlock->m_ToPage << "\n";
				if(g_Log&0x01)												// list to logfile, if requested
				{
					g_Logfile.setf(ios::hex|ios::uppercase);
					g_Logfile << pBlock->m_Name << "\t BLOCK \t" << pBlock->m_Address << "," <<pBlock->m_ToAddr << ",";
					g_Logfile.unsetf(ios::hex);
					g_Logfile <<pBlock->m_Page <<"," << pBlock->m_ToPage << "\n";
				}
			}
		}
	}
	if(pCount!=NULL)	*pCount=tot;										// report total number of occurences
	else if(g_Log&0x01) g_Logfile <<"\n";									// end listing with blank line

	return pMem;
}

//----------------------------------------------------------------------------------
// Assign a segment to a block
//----------------------------------------------------------------------------------
int CBlock::Assign(void* pSegment, int ForcePage, int ForceAddress)
{
	int page,size,p,step;
	CSegment* pSeg=(CSegment*)pSegment;
	CList* pPageSegs=NULL;

	if(m_Type!=BT_BLOCK)												// sanity check
	{
		cout << "Warning: cannot assign segment to model/device\n";
		if(g_Log&0x02) g_Logfile << "Warning: cannot assign segment to model/device\n";
		return -1;
	}
	if(pSeg->m_Address>0)
	{
		cout << "Segment " << pSeg->m_Name << " is already assigned\n";
		if(g_Log&0x02) g_Logfile << "Segment " << pSeg->m_Name << " is already assigned\n";
		return -1;
	}
	if(pSeg->m_Size==0)
	{
		cout << "Segment " << pSeg->m_Name << " is empty\n";
		if(g_Log&0x02) g_Logfile << "Segment " << pSeg->m_Name << " is empty\n";
		return -1;
	}

	if(m_Page<m_ToPage)													// to increment or decrement page number
	{
		step=m_Step;
		if((ForcePage>=0)&&((ForcePage<m_Page)||(ForcePage>m_ToPage)))
		{
			cout << "Page " << ForcePage << " is not part of block " << m_Name << " for ASSIGN\n";
			if(g_Log&0x02) g_Logfile << "Page " << ForcePage << " is not part of block " << m_Name << " for ASSIGN\n";
			return -1;
		}
	}
	else 
	{
		step=-m_Step;
		if((ForcePage>=0)&&((ForcePage<m_ToPage)||(ForcePage>m_Page)))
		{
			cout << "Page " << ForcePage << " is not part of block " << m_Name << " for ASSIGN\n";
			if(g_Log&0x02) g_Logfile << "Page " << ForcePage << " is not part of block " << m_Name << " for ASSIGN\n";
			return -1;
		}
	}

	for(p=0,page=m_Page;page!=m_ToPage+step;p++,page+=step)				// page loop
	{
		if((ForcePage>=0)&&(page!=ForcePage))							// force a particular page
		{
			if(p>=m_Pages.GetSize())									// must have an entry for each page
			{
				pPageSegs=new CList();									// create a new segment list
				m_Pages.Add((PTR)pPageSegs);							// add it to page list
			}
			continue;
		}

		size=m_ToAddr-m_Address+1;										// block size
		if(size<0) size=-size;											// absolute value
		size-=GetPageSize(p);											// minus stub and already loaded code
		pPageSegs=(CList*)m_Pages.GetAt(p);								// in case page already exists (NULL otherwise)

		if(pSeg->m_Size>size)											// too big for this page
		{
			if(ForcePage<0) continue;
			cout << "Page " << ForcePage << " has no room to ASSIGN segment " << pSeg->m_Name << " to it\n";
			if(g_Log&0x02) g_Logfile << "Page " << ForcePage << " has no room to ASSIGN segment " << pSeg->m_Name << " to it\n";
			return -1;
		}

		if(pPageSegs==NULL)												// no pointer yet
		{
			pPageSegs=new CList();										// create a new segment list
			m_Pages.Add((PTR)pPageSegs);								// add it to page list
		}
		
		if(m_Address<m_ToAddr)											// load from bottom
		{
			if(ForceAddress>=0)
			{
				if((ForceAddress<m_Bottom)||(ForceAddress+pSeg->m_Size>m_ToAddr))
				{
					cout << "Address " << ForceAddress << " is not available to ASSIGN " << pSeg->m_Name << " to it\n";
					if(g_Log&0x02) g_Logfile << "Address " << ForceAddress << " is not available to ASSIGN " << pSeg->m_Name << " to it\n";
					return -1;
				}
				size=ForceAddress-m_Bottom;								// gap before segment begins
				pPageSegs->Add((PTR)size);								// special gap signal
				m_Bottom=ForceAddress;
			}
			pSeg->m_Address=m_Bottom;									// assign segment to bottom of block
			if(!m_Overlay) m_Bottom+=pSeg->m_Size;						// update ptr, except for overlays
			pPageSegs->Add((PTR)pSeg);									// add segment to list for that page
		}
		else															// load at the top
		{
			if(ForceAddress>=0)
			{
				if((ForceAddress>m_Bottom)||(ForceAddress-pSeg->m_Size<m_ToAddr))
				{
					cout << "Address " << ForceAddress << " is not available for ASSIGN " << pSeg->m_Name << " to it\n";
					if(g_Log&0x02) g_Logfile << "Address " << ForceAddress << " is not available for ASSIGN " << pSeg->m_Name << " to it\n";
					return -1;
				}
				size=m_Bottom-ForceAddress-pSeg->m_Size;				// gap after segment ends
				pPageSegs->InsertAt((PTR)size,0);								// special gap signal
				m_Bottom=ForceAddress+pSeg->m_Size-1;
			}
			if(!m_Overlay) m_Bottom-=pSeg->m_Size;						// update ptr, except for overlays
			pSeg->m_Address=m_Bottom+1;									// assign segment to top of block
			pPageSegs->InsertAt((PTR)pSeg,0);							// add segment at top of list
		}
		pSeg->m_Page=page;												// assign page

		return pSeg->m_Address;											// return address where segment is assigned
	} // page loop

	return -2;
}

//----------------------------------------------------------------------------------
// Assign a segment to a block as a stub
//----------------------------------------------------------------------------------
int CBlock::AssignStub(void* pSeg)
{
	CSegment* pSegment=(CSegment*)pSeg;

	if(pSegment->m_Address<=0)											// segment not assigned yet
	{
		if(m_Address<m_ToAddr)											// place stubs at bottom
		{
			if(m_Bottom+pSegment->m_Size>m_ToAddr)
			{
				cout << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				return 1;
			}
			pSegment->m_Address=m_Bottom;								// stubs are at beginning of block
			m_Bottom+=pSegment->m_Size;									// update ptr
			m_Stubs.Add((PTR)pSegment);									// record stub in block's list
			pSegment->m_Special|=1;										// set stub flag
		}
		else
		{
			if(m_Bottom-pSegment->m_Size<m_ToAddr)
			{
				cout << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				return 1;
			}
			m_Bottom-=pSegment->m_Size;									// update ptr
			pSegment->m_Address=m_Bottom+1;								// stubs are at end of block
			m_Stubs.InsertAt((PTR)pSegment,0);							// record stub at top of block's list
			pSegment->m_Special|=1;										// set stub flag
		}
	}

	else																// segment already assigned, possibly to another block
	{
		if(m_Address<m_ToAddr)											// place stubs at bottom
		{
			if((pSegment->m_Address<m_Address)||(pSegment->m_Address>m_ToAddr)||(pSegment->m_Address+pSegment->m_Size<m_Address)||(pSegment->m_Address+pSegment->m_Size>m_ToAddr)) // assigned outside of this block
			{
				cout << "Stub segment " << pSegment->m_Name << " already assigned outside of block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " already assigned outside of block " << m_Name << "\n";
				return 1;												// report error
			}
			if(pSegment->m_Address+pSegment->m_Size>m_ToAddr)			// make sure it fits
			{
				cout << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				return 1;												// report error
			}
			m_Bottom=pSegment->m_Address+pSegment->m_Size;				// adjust ptr forward
			m_Stubs.Add((PTR)pSegment);									// add stub to list
			pSegment->m_Special|=1;										// set stub flag
		}
		else															// place stubs at top
		{
			if((pSegment->m_Address<m_ToAddr)||(pSegment->m_Address>m_Address)||(pSegment->m_Address+pSegment->m_Size<m_ToAddr)||(pSegment->m_Address+pSegment->m_Size>m_Address)) // assigned outside of this block
			{
				cout << "Stub segment " << pSegment->m_Name << " already assigned outside of block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " already assigned outside of block " << m_Name << "\n";
				return 1;												// report error
			}
			if(pSegment->m_Address-pSegment->m_Size<m_ToAddr)			// make sure it fits
			{
				cout << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				if(g_Log&0x02) g_Logfile << "Stub segment " << pSegment->m_Name << " too big for block " << m_Name << "\n";
				return 1;												// report error
			}
			m_Bottom=pSegment->m_Address-pSegment->m_Size;				// adjust ptr backwards
			m_Stubs.InsertAt((PTR)pSegment,0);							// record stub at top of block's list
			pSegment->m_Special|=1;										// set stub flag
		}
	}

	return 0;															// report success
}
