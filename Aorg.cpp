#include <iostream.h>
#include <fstream.h>
#include <malloc.h>
#include <memory.h>
#include "Aorg.h"
extern ofstream g_Logfile;
extern int g_Log;

/////////////////////////////////////////////////////////////////////////////////////
// CAorg class. Handles AORG instructions
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Default constuctor
//----------------------------------------------------------------------------------
CAorg::CAorg()
{
}

//----------------------------------------------------------------------------------
// Constuctor with one address
//----------------------------------------------------------------------------------
CAorg::CAorg(int Address)
{
	m_First=m_Last=Address;
}

//----------------------------------------------------------------------------------
// Expand range to include address
//----------------------------------------------------------------------------------
void CAorg::Include(int Address)
{
	if(Address>m_Last) m_Last=Address;
	if(Address<m_First) m_First=Address;
}

//----------------------------------------------------------------------------------
// Allocate data buffer
//----------------------------------------------------------------------------------
void CAorg::AllocateBuffers()
{
	int size = m_Last-m_First+2;											// round up to next word

	if(size<=0) return;														// no size

	m_pBuffer=(char*)malloc(size);											// size in bytes
	memset(m_pBuffer,0,size);												// zero the entire buffer
	m_pSegnbs=(char*)malloc(size/2);										// one seg number per word
	memset(m_pSegnbs,(char)0xFF,size/2);									// init as no segment
}

//----------------------------------------------------------------------------------
// Append data to buffer
//----------------------------------------------------------------------------------
int CAorg::Push(int Value, int Segnb)
{
	if(m_Top>m_Last-m_First+2)
	{
		cout << "AORG buffer overflow\n";
		if(g_Log&0x02) g_Logfile << "AORG buffer overflow\n";
		return 0;
	}

	m_pSegnbs[m_Top/2]=(Segnb&0xFF);										// save segment number
	m_pBuffer[m_Top++]=(Value>>8)&0xFF;										// save msb
	m_pBuffer[m_Top++]=(Value&0xFF);										// save lsb

	return m_Top;															// return current size
}

//----------------------------------------------------------------------------------
// Set data pointer
//----------------------------------------------------------------------------------
int CAorg::SetPtr(int Address)
{
	if((Address>m_Last)||(Address<m_First))
	{
		cout << "New address outside segment buffer\n";
		if(g_Log&0x02) g_Logfile << "New address outside segment buffer\n";
		return 1;	
	}

	m_Top=Address-m_First;													// set pointer

	return 0;																// announce success
}

//----------------------------------------------------------------------------------
// Get data from segment buffer
//----------------------------------------------------------------------------------
int CAorg::GetData(int Address)
{

	if((Address>m_Last)||(Address<m_First))									// safety check
	{
		cout << "Ref chain: link points outside of AORG stretch\n";
		if(g_Log&0x02) g_Logfile << "Ref chain: link points outside of AORG stretch\n";
		return -1;															// announce failure
	}

	Address-=m_First;														// make it segment offset
	
	return ((m_pBuffer[Address]<<8)&0xFF00)|(m_pBuffer[Address+1]&0xFF);	// return content of this offset
}

//----------------------------------------------------------------------------------
// Replace data inside segment buffer
//----------------------------------------------------------------------------------
int CAorg::SetData(int address, int value)
{

	if((address>m_Last)||(address<m_First))									// safety check
	{
		cout << "Ref chain: link points outside of AORG stretch\n";
		if(g_Log&0x02) g_Logfile << "Ref chain: link points outside of AORG stretch\n";
		return -1;															// announce failure
	}

	address-=m_First;														// make it segment offset
	
	m_pBuffer[address]=(value>>8)&0xFF;										// set new value
	m_pBuffer[address+1]=(value&0xFF);
	
	return 0;																// announce success
}

/////////////////////////////////////////////////////////////////////////////////////
// CAorgList class. Handles list of AORG objects
/////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------
// Add address to list, creating an AORG object if necessary
//----------------------------------------------------------------------------------
CAorg* CAorgList::Append(int Address)
{
	int i;
	CAorg* pAorg=0;

	for(i=0;i<m_Aorgs.GetSize();i++)										// find where Address fits in AORG list
	{
		pAorg=(CAorg*)m_Aorgs.GetAt(i);										// get one object

		if(Address>pAorg->m_Last+2) continue;								// address not reached yet

		if(Address>pAorg->m_Last)											// address is at the upper edge
		{
			pAorg->m_Last=Address;											// expand range to include it
			return pAorg;		
		}

		if(Address<pAorg->m_First-2)										// before this one
		{
			pAorg=new CAorg(Address);										// create a new one
			m_Aorgs.InsertAt((PTR)pAorg,i);									// insert it in list
			return pAorg;
		}

		if(Address<pAorg->m_First)											// address is at the lower edge
		{
			pAorg->m_First=Address;											// lower bottom to include it
			return pAorg;
		}

		break;																// adress is within a block
		return pAorg;
	}

	pAorg=new CAorg(Address);												// not found: create a new one
	m_Aorgs.Add((PTR)pAorg);												// add it to list
	return pAorg;
}


//----------------------------------------------------------------------------------
// Merge overlapping AORG blocks
//----------------------------------------------------------------------------------
void CAorgList::Merge(int MinGap)
{
	int i;
	CAorg *pAorg,*pAorg1;

	for(i=0;i<m_Aorgs.GetSize()-1;i++)										// find where Address fits in AORG list
	{
		pAorg=(CAorg*)m_Aorgs.GetAt(i);
		pAorg1=(CAorg*)m_Aorgs.GetAt(i+1);

		if(pAorg->m_Last+MinGap>=pAorg1->m_First)							// reaches next one
		{
			if(pAorg->m_Last<pAorg1->m_Last)								// englobes next one ?
				pAorg->m_Last=pAorg1->m_Last;								// no, expand range
			m_Aorgs.RemoveAt(i+1);											// remove next one
			delete pAorg1;													// delete the object
			i--;															// to stay on same
		}
	}
}

//----------------------------------------------------------------------------------
// Allocate data buffers for all segments
//----------------------------------------------------------------------------------
void CAorgList::AllocateBuffers()
{
	CAorg *pAorg;
	int i;

	for(i=0;i<m_Aorgs.GetSize();i++)										// iterate list
	{
		pAorg=(CAorg*)m_Aorgs.GetAt(i);										// get segment pointer
		pAorg->AllocateBuffers();											// allocate memory buffer
	}
}

//----------------------------------------------------------------------------------
// Sets pointer for push
//----------------------------------------------------------------------------------
CAorg* CAorgList::SetPtr(int Address)
{
	CAorg* pAorg=Find(Address);												// find proper object

	if(pAorg!=NULL)															// found
	{
		pAorg->SetPtr(Address);												// set the push ptr in this object
		return pAorg;														// return ptr to the object
	}

	cout << "Address " << Address << " not found in AORG stretches\n";
	if(g_Log&0x02) g_Logfile << "Address " << Address << " not found in AORG stretches\n";

	return NULL;
}

//----------------------------------------------------------------------------------
// Find AORG stretch containing an address
//----------------------------------------------------------------------------------
CAorg* CAorgList::Find(int Address)
{
	CAorg *pAorg;
	int i;

	for(i=0;i<m_Aorgs.GetSize();i++)										// iterate list
	{
		pAorg=(CAorg*)m_Aorgs.GetAt(i);										// get segment pointer
		if((Address>=pAorg->m_First)&&(Address<=pAorg->m_Last)) return pAorg;	// check if inside range
	}
	return NULL;															// not found
}
