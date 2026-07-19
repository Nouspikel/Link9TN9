/////////////////////////////////////////////////////////////////////////////////////
// CObjectfile class. Handles tagged object files
/////////////////////////////////////////////////////////////////////////////////////
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <malloc.h>
#include "Memimage.h"
#include "Block.h"
#include "Segment.h"
#include "Aorg.h"

extern int g_Verbous;
extern int g_ErrLevel;
extern ofstream g_Logfile;
extern int g_Log;

//----------------------------------------------------------------------------------
// Default constuctor
//----------------------------------------------------------------------------------
CMemimage::CMemimage()
{
	m_Format=0;
	m_MemType=0;
	m_Page=-1;
	m_Filename=NULL;
	m_MaxSize=0x2000;
	m_pAorgs=NULL;
	m_Incremented=0;
}

//----------------------------------------------------------------------------------
// Constuctor, with file name
//----------------------------------------------------------------------------------
CMemimage::CMemimage(char* filename, int format)
{
	m_Format=format;														// save format
	m_MemType=0;
	m_Page=-1;
	m_Filename=_strdup(filename);											// and filename
	m_MaxSize=0x2000;
	m_pAorgs=NULL;
	m_Incremented=0;
}

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CMemimage::~CMemimage()
{
	if(m_Filename!=NULL) free(m_Filename);
}

//----------------------------------------------------------------------------------
// Rewind file to a given position
//----------------------------------------------------------------------------------
void CMemimage::Seek(int offset)
{
	m_File.seekp(offset,ios::beg);
}

//----------------------------------------------------------------------------------
// Output block contents in the adequate format
//----------------------------------------------------------------------------------
int CMemimage::OutputCode(CList* pSelected, CList* pBlocks)
{
	int j,b,p,page,seg,address,size,stubsize,cont=0,lastchar=0,firstfile=1,Segout=0,more=0,done,bottom,gap;
	CBlock *pBlock;
	CSegment *pSegment;
	CAorg *pAorg;
	char *pName, *pBuf;
	char Aorg[]="$AORG";
	CList *pList,*pPageSegs;

	if(m_pAorgs!=NULL)
	{
		pList=m_pAorgs;
		Segout=2;
	}
	else if(m_Segments.GetSize()>0)												// outputing segments
	{
		pList=&m_Segments;
		Segout=1;
	}
	else if(m_Blocks.GetSize()>0)											// blocks specified in OUTPUT statement
		pList=&m_Blocks;
	else if(pSelected->GetSize()>0)											// blocks specified in SELECT statement
		pList=pSelected;
	else																	// do all blocks (default added by dispatch() fct)
		pList=pBlocks;

// Segments

	if(Segout>0)															// outputing segments
	{
		for(seg=0;seg<pList->GetSize();seg++)								// iterate list of segments
		{
			if(Segout==1)
			{																// relocatable segments
				pSegment=(CSegment*)pList->GetAt(seg);
				address=pSegment->m_Address;
				size=pSegment->m_Size;
				page=pSegment->m_Page;
				pName=pSegment->m_Name;
				pBuf=pSegment->m_pBuffer;
				if(pSegment->m_Special&0x0001)								// segment is a stub
					pSegment->FixThispg(pSegment->m_Page);					// fix THISPG chain in stub segments
			}
			else															// AORG stretches
			{
				pAorg=(CAorg*)pList->GetAt(seg);
				address=pAorg->m_First;
				size=pAorg->m_Last-pAorg->m_First+2;
				pName=Aorg;
				pBuf=pAorg->m_pBuffer;
			}
			if(size<=0) continue;											// nothing in segment
			if(address<0)													// segment was not dispatched 
			{
				if(g_Verbous>0)
				{
					cout << "Warning: Segment " << pName << " not assigned an address\n";
					if(g_Log&0x02) g_Logfile << "Warning: Segment " << pName << " not assigned an address\n";
				}
				if(g_ErrLevel>1) return 1;
				continue;
			}
			if(size>m_MaxSize)
			{
				more=size-m_MaxSize;										// excedent
				size=m_MaxSize;												// maximum file size (not counting header)
			}
			else more=0;
			NewFile(size,address,more,pName,page);							// open file

			while(size>0)													// issue code
			{
				m_File.write(pBuf,size);									// append segment code to file
				if(more)													// too big for one file?
				{
					address+=size;											// adjust address
					pBuf+=size;												// adjust ptr
					size=more;
					if(size>m_MaxSize)
					{
						more=size-m_MaxSize;								// excedent
						size=m_MaxSize;										// maximum file size
					}
					else more=0;
					NextFile(size,address,more);							// more to come on next file
				}
				else														// segment done
				{
					m_File.close();											// close the file
					break;													// exit the while loop
				}
			} // write loop                                                 // next segment
		} // segments
		return 0;															// all segments done
	} // if segout

// Blocks

	for(b=0,size=0;b<pList->GetSize();b++)									// iterate list of blocks
	{
		pBlock=(CBlock*)pList->GetAt(b);									// get one block
		if(pBlock->m_Pages.GetSize()==0) continue;							// no code in this block
		page=pBlock->m_Page-pBlock->m_Step;									// first page number
		pName=pBlock->m_Name;												// block name

		for(p=0;p<pBlock->m_Pages.GetSize();p++)							// iterate list of pages in block
		{
			pPageSegs=(CList*)pBlock->m_Pages.GetAt(p);						// get ptr to list of segments
			page+=pBlock->m_Step;											// new page number
			stubsize=pBlock->GetPageSize(-1);								// get stub size
			if(stubsize>m_MaxSize)											// make sure it fits in a single file
			{
				if(g_Verbous>0)
				{
					cout << "Stub too big for file, in block " << pName << "\n";	// report error
					if(g_Log&0x02) g_Logfile << "Stub too big for file, in block " << pName << "\n";	// report error
				}
				if(g_ErrLevel>1) return 1;
				break;														// exit page loop, do next block
			}

			size=pBlock->GetPageSize(p);									// get size of this page (including stub)
			if(size<=stubsize) continue;									// empty page
			bottom=pBlock->m_Address-size+1;								// remember, in case we load from the top

			if(size>m_MaxSize)												// see if whole page fits in file
			{
				more=size-m_MaxSize;										// excedent
				size=m_MaxSize;												// maximum file size
			}
			else more=0;													// it fits

			if(pBlock->m_Address<pBlock->m_ToAddr)							// load from bottom to top
			{
				address=pBlock->m_Address;									// start at bottom of block
				NewFile(size,address,more,pName,page,pBlock->m_MemType);	// open file with new name

				for(j=0;j<pBlock->m_Stubs.GetSize();j++)					// add stubs at bottom of page (if any)
				{
					pSegment=(CSegment*)pBlock->m_Stubs.GetAt(j);			// get one
					pSegment->FixThispg(page);								// fix REFs to THISPG (if any)
					m_File.write(pSegment->m_pBuffer,pSegment->m_Size);		// append stub code to file (assume it always fits)
					if(g_Verbous>=4) 
					{
						cout.setf(ios::hex|ios::uppercase);
						cout << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size << " bytes\n";
						cout.unsetf(ios::hex|ios::uppercase);
						if(g_Log&0x01)
						{
							g_Logfile.setf(ios::hex|ios::uppercase);
							g_Logfile << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size << " bytes\n";
							g_Logfile.unsetf(ios::hex|ios::uppercase);
						}
					}
				}
			}
			else															// load from top to bottom
			{
				address=bottom;												// start at lowest loaded address
				NewFile(size,address,more,pName,page,pBlock->m_MemType);	// open file with new name
			}																// stubs will come at the end

			for(seg=0;seg<pPageSegs->GetSize();seg++)						// iterate segments in page
			{
				pSegment=(CSegment*)pPageSegs->GetAt(seg);					// get one segment
				if((gap=(int)pSegment)<0x10000)								// gap mark
				{
					for(j=0;j<gap;j++)										// assume it fits in current file									
					{
						m_File.put((char)0x00);								// fill the gap with zeros
					}
					continue;
				}
				done=0;
				while(1)													// write segment contents
				{
					if(pSegment->m_Size-done<=size)							// see if segment fits in this file
					{														// it fits
						m_File.write(pSegment->m_pBuffer+done,pSegment->m_Size-done);	// append segment code to file
						if(g_Verbous>=4) 
						{
							cout.setf(ios::hex|ios::uppercase);
							cout << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size-done << " bytes\n";
							cout.unsetf(ios::hex|ios::uppercase);
							if(g_Log&0x01)
							{
								g_Logfile.setf(ios::hex|ios::uppercase);
								g_Logfile << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size-done << " bytes\n";
								g_Logfile.unsetf(ios::hex|ios::uppercase);
							}
						}
						size-=(pSegment->m_Size-done);						// adjust remaining size in page
						break;												// exit while loop: next segment
					}

					m_File.write(pSegment->m_pBuffer,size);					// write what fits into the file
					if(g_Verbous>=4) 
					{
						cout.setf(ios::hex|ios::uppercase);
						cout << "Outputing " << pSegment->m_Name << " >" << size << " bytes\n";
						cout.unsetf(ios::hex|ios::uppercase);
						if(g_Log&0x01)
						{
							g_Logfile.setf(ios::hex|ios::uppercase);
							g_Logfile << "Outputing " << pSegment->m_Name << " >" << size << " bytes\n";
							g_Logfile.unsetf(ios::hex|ios::uppercase);
						}
					}

					done=size;												// what was written
					size=more;												// what's left to write
					if(size>m_MaxSize)										// see if it fits
					{
						more=size-m_MaxSize;								// excedent
						size=m_MaxSize;										// maximum file size
					}
					else more=0;											// it fits
					address+=m_MaxSize;										// adjust address
					NextFile(size,address,more);							// open next file for same block
				}	// write loop
			} // next segment

			if(pBlock->m_Address>pBlock->m_ToAddr)							// load from top to bottom
			{																// add stubs at end of page
				for(j=0;j<pBlock->m_Stubs.GetSize();j++)					// iterate stubs in block (if any)
				{
					pSegment=(CSegment*)pBlock->m_Stubs.GetAt(j);			// get one stub
					pSegment->FixThispg(page);								// fix REFs to THISPG (if any)
					m_File.write(pSegment->m_pBuffer,pSegment->m_Size);		// append stub code to file (assume it always fits)
					if(g_Verbous>=4) 
					{
						cout.setf(ios::hex|ios::uppercase);
						cout << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size << " bytes\n";
						cout.unsetf(ios::hex|ios::uppercase);
						if(g_Log&0x01)
						{
							g_Logfile.setf(ios::hex|ios::uppercase);
							g_Logfile << "Outputing " << pSegment->m_Name << " >" << pSegment->m_Size << " bytes\n";
							g_Logfile.unsetf(ios::hex|ios::uppercase);
						}
					}
				}
			} 

		} // next page
	} // next block

	if(m_File.is_open())
		m_File.close();														// close last file object, if any

	return 0;
}

//----------------------------------------------------------------------------------
// Generate hexadecimal number
//----------------------------------------------------------------------------------
int CMemimage::MakeHex(char*s, int n)
{
	int i=0;

	if(n>0x0FFF)																// nibble 1
	{
		s[i]=((n>>12)&0x000F)+'0';												// issue it
		if(s[i]>'9') s[i]+='@'-'9';												// correct A-F
		i++;
	}
	if(n>0x00FF)																// nibble 2
	{
		s[i]=((n>>8)&0x000F)+'0';
		if(s[i]>'9') s[i]+='@'-'9';
		i++;
	}
	if(n>0x000F)																// nibble 3
	{
		s[i]=((n>>4)&0x000F)+'0';
		if(s[i]>'9') s[i]+='@'-'9';
		i++;
	}
	s[i]=(n&0x000F)+'0';														// nibble 4, always here
	if(s[i]>'9') s[i]+='@'-'9';
	i++;

	return i;																	// return number of characters
}

//----------------------------------------------------------------------------------
// Generate decimal number
//----------------------------------------------------------------------------------
int CMemimage::MakeNumber(char* s, int n)
{
	int i=0;

	if(n>10000)																	// tens of thousands
	{
		s[i]=(n/10000)+'0';
		i++;
	}
	if(n>1000)																	// thousands
	{
		s[i]=((n%10000)/1000)+'0';
		i++;
	}
	if(n>100)																	// hundreds
	{
		s[i]=((n%1000)/100)+'0';
		i++;
	}
	if(n>100)																	// tens
	{
		s[i]=((n%100)/10)+'0';
		i++;
	}
	s[i]=(n%10)+'0';															// at least one digit
	i++;

	return i;																	// return number of characters
}

//----------------------------------------------------------------------------------
// Creates a new file, with potentially different name
//----------------------------------------------------------------------------------
int CMemimage::NewFile(int Size, int Address, int More, char* pBlockname, int Page, int Memtype)
{
	int i,j,k;

	m_MemType=Memtype;														// remember for next file
	m_Page=Page;

	for(i=j=0;;i++)															// coin filename
	{
		if(m_Filename[i]!='?')												// regular character
		{
			m_Realname[j++]=m_Filename[i];									// copy it
			if((m_Filename[i]=='.')&&(m_Lastchar==0)) m_Lastchar=j-2;		// character to increment: before extension
			if(m_Filename[i]==0x00) break;									// end of filename
		}
		else																// ? option
		{
			switch(m_Filename[++i])											
			{
			case 'N':														// ?N = name
				if(pBlockname==NULL) break;
				for(k=0;(k<8)&&(pBlockname[k]>0x00);k++,j++)
					m_Realname[j]=pBlockname[k];							// insert block/segment name into filename
				break;
			case 'P':
				j+=MakeNumber(m_Realname+j,Page);							// ?P = page number
				break;
			case 'H':
				j+=MakeHex(m_Realname+j,Page);								// ?H = page number in hexadecimal format
				break;
			case 'I':
				m_Lastchar=j;												// ?I = increment previous char
				break;
			case 'i':
				m_Lastchar=-1;												// ?i = no incrementation
				break;
			}
		}

	}
	m_Realname[j]=0x00;
	if(m_Lastchar==0) m_Lastchar=j-1;										// remember for incrementation

	if(m_File.is_open())
		m_File.close();														// close previous file object, if any
	m_File.open(m_Realname,ios::binary);									// open memory image file
	if(!m_File)																// file error
	{
		cout << "Cannot open Memimage file '" << m_Realname << "'\n";
		if(g_Log&0x02) g_Logfile << "Cannot open Memimage file '" << m_Realname << "'\n";
		return 1;
	}
	if(g_Verbous>=4) 
	{
		cout << "New file " << m_Realname << "\n";
		if(g_Log&0x01)
		{
			g_Logfile << "New file " << m_Realname << "\n";
		}
	}

	if((m_Format==MF_EA5)||(m_Format==MF_FB6))								// issue header
	{
		if(More==0)
		{
			m_File.put((char)0x00);											// no continuation
			m_File.put((char)0x00);
		}
		else
		{
			m_File.put((char)0xFF);											// continuation flag
			m_File.put((char)0xFF);
		}
		m_File.put((char)((Size>>8)&0xFF));									// first size
		m_File.put((char)((Size)&0xFF));
		m_File.put((char)((Address>>8)&0xFF));								// then address
		m_File.put((char)((Address)&0xFF));
	}
	else if(m_Format==MF_GK)
	{
		if(More==0)															// continuation flag
		{
			m_File.put((char)0x00);											// no continuation
		}
		else
		{
			m_File.put((char)0xFF);
		}

		if(Memtype==1)														// memory flag
		{
			m_File.put((char)(((Address>>13)&0x0007)+1));					// gram flag: >01 through >08 according to address
		}
		else 
		{
			if(Page>=0)	m_File.put((char)((Page+9)&0xFF));					// ram page flag: >09 and above
			else m_File.put((char)0x00);
		}

		m_File.put((char)((Size>>8)&0xFF));									// first size
		m_File.put((char)((Size)&0xFF));
		m_File.put((char)((Address>>8)&0xFF));								// then address
		m_File.put((char)((Address)&0xFF));
	}


	return 0;

}

//----------------------------------------------------------------------------------
// Creates a new file to continue the current one
//----------------------------------------------------------------------------------
int CMemimage::NextFile(int Size, int Address, int More)
{
	m_File.seekp(0);														// rewind file
	m_File.put((char)0xFF);													// install continuation flag
	m_File.put((char)0xFF);
	m_File.close();	

	if(m_Lastchar>0)														// modify filename
	{
		if((m_Format=MF_GK)&&(!m_Incremented))
		{
			strcat(m_Realname,"1");											// append a '1' to the end of the filename
			m_Incremented=1;												// don't do it again
			m_Lastchar=strlen(m_Realname);									// will increment the '1' from now on
		}
		else
			m_Realname[m_Lastchar]++;										// increment filename
	}

	m_File.open(m_Realname,ios::binary);									// open new memory image file
	if(!m_File)																// file error
	{
		cout << "Cannot open object file " << m_Realname << "\n";
		if(g_Log&0x02) g_Logfile << "Cannot open object file " << m_Realname << "\n";
		return 1;
	}
	if(g_Verbous>=4) 
	{
		cout << "Next file " << m_Realname << "\n";
		if(g_Log&0x01)
		{
			g_Logfile << "Next file " << m_Realname << "\n";
		}
	}

	if((m_Format==MF_EA5)||(m_Format==MF_FB6))								// issue header
	{
		if(More==0)
		{
			m_File.put((char)0x00);											// no continuation
			m_File.put((char)0x00);
		}
		else
		{
			m_File.put((char)0xFF);											// continuation flag
			m_File.put((char)0xFF);
		}
		m_File.put((char)((Size>>8)&0xFF));									// first size
		m_File.put((char)((Size)&0xFF));
		m_File.put((char)((Address>>8)&0xFF));								// then address
		m_File.put((char)((Address)&0xFF));
	}
	else if(m_Format==MF_GK)
	{
		if(More==0)															// continuation flag
		{
			m_File.put((char)0x00);											// no continuation
		}
		else
		{
			m_File.put((char)0xFF);
		}

		if(m_MemType==1)													// memory flag
		{
			m_File.put((char)(((Address>>13)&0x0007)+1));					// gram flag: >01 through >08 according to address
		}
		else 
		{
			if(m_Page>=0)	m_File.put((char)((m_Page+9)&0xFF));			// ram page flag: >09 and above
			else m_File.put((char)0x00);
		}

		m_File.put((char)((Size>>8)&0xFF));									// first size
		m_File.put((char)((Size)&0xFF));
		m_File.put((char)((Address>>8)&0xFF));								// then address
		m_File.put((char)((Address)&0xFF));
	}

	return 0;
}

//----------------------------------------------------------------------------------
// Append FB6 tags and close the file
//----------------------------------------------------------------------------------
void CMemimage::CloseFile()
{
	if(m_Format==MF_FB6)													// only if FB6
	{
		m_File.put((char)0xFB);												// FB6 tag
		m_File.put((char)0x01);

		m_File.put((char)0xF1);												// flags tag
		m_File.put((char)0x02);
		m_File.put((char)0x00);
		if(m_MemType==1)													// gram/grom
			m_File.put((char)0x02);
		else
			m_File.put((char)0x00);

		if(m_Page>=0)
		{
			m_File.put((char)0x81);											// device-relative page tag
			m_File.put((char)0x02);
			m_File.put((char)((m_Page>>8)&0xFF));
			m_File.put((char)((m_Page)&0xFF));
		}

		m_File.put((char)0x00);												// end of list tag
		m_File.put((char)0x01);
	}

	m_File.close();															// close the file
}
