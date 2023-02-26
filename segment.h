/////////////////////////////////////////////////////////////////////////////////////
// CSegment class. Holds segment information
/////////////////////////////////////////////////////////////////////////////////////
#include "List.h"
class CSegment
{
public:
	int FixThispg(int Page);
	int GetSegnumber(int Address);
	char m_Name[7];									// name
	int m_Size;										// total size
	int m_Address;									// base address
	int m_Page;										// page where to load it
	char *m_pBuffer;								// data buffer
	char *m_pSegnbs;								// segment # buffer (for ref chain walking)
	int m_Top;										// buffer pointer
	int m_Bottom;									// buffer startpoint after partial loading from a file
	int m_Number;									// local number
	int m_Flags;									// loading flags
	int m_Special;									// special flags
	CList m_Thispg;									// refs to THISPG for stub segments

	CSegment();										// default constuctor
	CSegment(char* Name, int Size=0, int Tag=0, int Segnb=-1);	// constructor with initialization
	~CSegment();									// destructor
	void AllocateBuffers();							// allocate data buffer
	int Push(int value, int segnb=-1);				// push data into segment buffer
	int GetData(int address, int IsOffset=0);		// get data from segment buffer
	int SetData(int address, int value, int segnb=-1, int IsOffset=0);	// replace data inside segment buffer
	int SetPtr(int value);							// set new pointer for pushed data
};

class CSegmentList
{

public:
	int CheckList();
	CList m_Segments;								// list of segment pointers

	~CSegmentList();								// destructor
	int GetSize();									// get list size
	CSegment* GetAt(int i);							// get one segment object
	void AllocateBuffers();							// allocate data buffers
	void OutputTable();								// output segment table
	int SetNumber(char* Name, int SegNb);			// set segment number
	void ResetNumbers();							// reset all segment numbers
	int FindIndex(char* Name);						// find segment position in list, by name
	int FindIndex(int Number);						// find segment position in list, by local number
	CSegment* Find(char *Name);						// find segment by name
	CSegment* Find(int Number);						// find segment by local number
	CSegment* Find(char Type,int *pCount);			// find segment by type
	int GetAddress(char* Label);					// get segment address, by name
	int GetAddress(int number);						// get segment address, by local number
	CSegment* Append(char* Name, int Size=0, int Tag=0, int Segnb=-1);
	int SetPtr(int number, int value);				// set new data pointer, by local number
	int SetPtr(char* Name, int value);				// set new data pointer, by name
	int GetPage(int Segnb);							// get segment page (by number)
	int GetSegnumber(int Segnb, int Address);		// get assigned segment number
	int SetData(int Segnb, int Address, int Value, int Tag=-1, int IsOffset=0);		// set data in segment (by local number)
	int SetData(char* Segname, int Address, int Value, int Tag=-1, int IsOffset=0);	// set data in segment (by name)
	int GetData(int Segnb, int Address, int IsOffset=0);			// get data from segment (by local number)
	int GetData(char* Segname, int Address, int IsOffset=0);		// get data from segment (by name)
//	int Push(int number,int value, int tag=-1);		// push data into segment buffer
};
