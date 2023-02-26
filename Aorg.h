#include "List.h"

class CAorg
{
public:
	CAorg();										// default constructor
	CAorg(int Address);								// constructor with address
	void AllocateBuffers();							// allocate memory for data
	void Include(int Address);						// adjust limits to include an address
	int Push(int value, int tag=-1);				// push data into segment buffer
	int GetData(int address);						// get data from segment buffer
	int SetData(int address, int value);			// replace data inside segment buffer
	int SetPtr(int value);							// set new pointer for pushed data
public:
	int m_Last;										// last address used
	int m_First;									// first address used
	int m_Top;										// current pointer
	char *m_pBuffer;								// data buffer
	char *m_pSegnbs;								// segment # buffer (for ref chain walking)
};

class CAorgList
{
public:
	CAorg* Find(int Address);						// find AORG object containing an address
	CAorg* SetPtr(int Address);						// set push pointer in the proper AORG object
	void AllocateBuffers();							// allocate data buffers
	void Merge(int MinGap);							// merge adjacent/overlaping blocks
	CAorg* Append(int Address);						// create object to include address (if needed)

public:
	CList m_Aorgs;
};
