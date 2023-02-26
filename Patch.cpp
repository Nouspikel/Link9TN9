#include "Patch.h"
#include <string.h>
#include <malloc.h>

/////////////////////////////////////////////////////////////////////////////////////
// CPatch class. Handles patches and verifications
/////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------
CPatch::CPatch(int Type, char* Segname, int Address, int Value1)
{
	m_Type=Type;
	m_SegName=strdup(Segname);
	m_Address=Address;
	m_Values.Add((PTR)Value1);
}

//----------------------------------------------------------------------------------
// Increments the list of values
//----------------------------------------------------------------------------------
int CPatch::Append(int Value)
{
	return m_Values.Add((PTR)Value);
}

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------

CPatch::~CPatch()
{
	if(m_SegName!=NULL) free(m_SegName);
}
