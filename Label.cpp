#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <malloc.h>
#include "label.h"
extern ofstream g_Logfile;
extern int g_Log;
extern int g_Verbous;
/////////////////////////////////////////////////////////////////////////////////////
// Label class. Holds label information
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Constuctor (empty)
//----------------------------------------------------------------------------------
CLabel::CLabel()
{
	m_Name[0]=0;										// init empty name
	m_Name[6]=m_Name[7]=m_Name[8]=0;					// extra 0 at end
	m_Value=0;
	m_SegIdx=0;
	m_Type=0;
	m_Undefined=1;
}

//----------------------------------------------------------------------------------
// Constuctor, with parameters
//----------------------------------------------------------------------------------
CLabel::CLabel(char* Label, int Value, int Tag, int SegNb)
{
	strcpy(m_Name,Label);								// copy name
	m_Name[6]=m_Name[7]=m_Name[8]=0;					// extra 0 at end
	m_Value=Value;
	m_SegIdx=SegNb;
	m_Type=Tag;
	if(Value==-1) m_Undefined=1;
	else m_Undefined=0;
}

/////////////////////////////////////////////////////////////////////////////////////
// LabelList class. Manages a list of labels
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
CLabelList::~CLabelList()
{
	m_Labels.RemoveAll();
}

//----------------------------------------------------------------------------------
// Append new label to chain
//----------------------------------------------------------------------------------
CLabel* CLabelList::Append(char* Label, int Value, int Tag, int SegNb)
{
	CLabel *pLab;

	if((pLab=Find(Label))!=NULL) return pLab;			// already exists, return it
	
	pLab=new CLabel(Label,Value,Tag,SegNb);				// create new label
	m_Labels.Add((PTR)pLab);							// add it to list

	return pLab;										// return it
}

//----------------------------------------------------------------------------------
// Modify label value (create new one if it does not exist)
//----------------------------------------------------------------------------------
CLabel* CLabelList::SetValue(char* Label, int Value, int Tag, int SegIdx, int Undef)
{
	CLabel *pLab;

	if((pLab=Find(Label))==NULL)						// not found
	{
		pLab=new CLabel(Label,Value,Tag,SegIdx);		// create new label
		m_Labels.Add((PTR)pLab);						// add it to list
	}

	if(Value>=0) pLab->m_Value=Value;					// modify parameter: negative = don't change
	if(Tag>=0) pLab->m_Type=Tag;
	if(SegIdx>=0) pLab->m_SegIdx=SegIdx;
	if(Undef>=0) pLab->m_Undefined=Undef;

	return pLab;										// return it
}

//----------------------------------------------------------------------------------
// Find a label by name
//----------------------------------------------------------------------------------
CLabel* CLabelList::Find(char *Label, int Undef)
{
	CLabel *pLab;
	int i;

	for(i=0;i<m_Labels.GetSize();i++)
	{
		pLab=(CLabel*)m_Labels.GetAt(i);

		if((Undef)&&(pLab->m_Value>=0)) continue;		// we want only undefined ones
		if(strcmp(pLab->m_Name,Label)==0)	return pLab;	// names match, return this label
	}

	return NULL;										// not found
}

//----------------------------------------------------------------------------------
// Find label value from name
//----------------------------------------------------------------------------------
int CLabelList::GetValue(char* Label)
{
	CLabel *pLab;
	int i;

	for(i=0;i<m_Labels.GetSize();i++)
	{
		pLab=(CLabel*)m_Labels.GetAt(i);
		if(strcmp(pLab->m_Name,Label)==0)	return pLab->m_Value;	// names match, return value
	}

	return -1;											// not found
}

//----------------------------------------------------------------------------------
// Output symbol table
//----------------------------------------------------------------------------------
CLabel* CLabelList::OutputTable(char* Type, int Count)
{
	CLabel *pLab, *pMem=NULL;
	int i,tot=0;
	
	if(Count==0)
	{
		cout << "Label\tValue\tType\tSegment\n--------------------------------\n";
		if(g_Log&0x02) g_Logfile << "\nLabel\tValue\tType\tSegment\n--------------------------------\n";
	}

	for(i=0;i<m_Labels.GetSize();i++)							// iterate all labels			
	{
		pLab=(CLabel*)m_Labels.GetAt(i);						// get one
		if((Type!=NULL)&&(!strchr(Type,pLab->m_Type))) continue;	// type does not match: skip
		if((Type!=NULL)&&(strchr(Type,'U')&&(pLab->m_Undefined!=1))) continue;	// list undefined labels only

		if(Count)												// save info for FIND
		{
			tot++;												// found one
			if(tot==Count) pMem=pLab;							// it's the one we wanted: remember it
		}
		else													// output info
		{
			cout.setf(ios::hex|ios::uppercase);
			cout << pLab->m_Name << "\t" << pLab->m_Value;
			cout.unsetf(ios::hex|ios::uppercase);
			cout << "\t" << pLab->m_Type << "\t" << pLab->m_SegIdx << "\n";
			if(g_Log&0x02)
			{
				g_Logfile.setf(ios::hex|ios::uppercase);
				g_Logfile << pLab->m_Name << "\t" << pLab->m_Value;
				g_Logfile.unsetf(ios::hex|ios::uppercase);
				g_Logfile << "\t" << pLab->m_Type << "\t" << pLab->m_SegIdx << "\n";
			}
		}
	}

	return pMem;												// return wanted label, if any
}

//----------------------------------------------------------------------------------
// Return current # of labels
//----------------------------------------------------------------------------------
int CLabelList::GetSize()
{
	return m_Labels.GetSize();									// get it from list
}

//----------------------------------------------------------------------------------
// Count undefined labels
//----------------------------------------------------------------------------------
int CLabelList::CountUndef(int Sref)
{
	int i,count=0;
	CLabel *pLab;

	for(i=0;i<m_Labels.GetSize();i++)							// iterate list of labels
	{
		pLab=(CLabel*)m_Labels.GetAt(i);						// get one
		if(pLab->m_Undefined==0) continue;						// this one is defined
		if(strcmp(pLab->m_Name,"PG$SEG")==0) continue;			// ignore special loader labels
		if(strcmp(pLab->m_Name,"THISPG")==0) continue;			// ignore special loader labels
		if(Sref) count++;										// count SREF
		else if((pLab->m_Type!='V')&&(pLab->m_Type!='Y')&&(pLab->m_Type!='Z'))  // this is not SREF
		{
			count++;											// count this one
			if(g_Verbous>=4)
			{
				cout << pLab->m_Name << " undefined\n";
				if(g_Log&0x02)
				{
					g_Logfile << pLab->m_Name << " undefined\n";
				}
			}
		}
	}

	return count;												// return the count	
}

