#include "List.h"
enum {BT_NONE,BT_BLOCK,BT_DEVICE,BT_MODEL};
class CBlock
{
public:
	int m_MemType;									// type of memory
	int m_Overlay;									// flag: block is reserved for overlays
	int m_Type;										// type of block: model, device or block
	char m_Name[7];									// block name
	int	m_Address;									// start address
	int	m_ToAddr;									// end address (if smaller than start: load from top)
	int m_Page;										// first page
	int m_CurPage;									// current page (when assigning)
	int m_ToPage;									// last page (may be smaller than first)
	int m_Step;										// page numbering increment
	CList m_Pages;									// list of segment lists, one per loaded page
	CList m_Stubs;									// list of stub segments
	CList m_Children;								// for models or devices: list of child blocks
	char* m_Condition;								// condition to load a segment in this block
	int m_Bottom;									// current bottom of block in current page
	int m_Top;										// current top of block in current page
public:
	CBlock();										// default constructor
	~CBlock();										// destructor
	CBlock(char* Name, int Type);					// constructor for devices and models
	CBlock(char* Name, int Address, int Size,int Page, int Topage, int Step, int Memtype); // constructor for blocks
	void Define(int Address, int ToAddr, int Page, int ToPage, int Step, int Memtype); // in case block is defined after construction
	int GetPageSize(int page);						// calculate size of code loaded in one page (stubs+segments)
	int Assign(void* pSeg, int ForcePage=-1, int ForceAddress=-1);	// assign a segment to a block (unconditionnally)
	int AssignStub(void* pSegment);					// assign a segment as a stub
};

class CBlockList
{
public:
	void Reset(int what);
	~CBlockList();									// destructor
	CBlock* AddDevice(char* Name, int Type);		// add a device or a model
	CBlock* AddBlock(char* Name, int Address, int ToAddr, int Page=0, int ToPage=0, int Step=1, int Memtype=0); // add a block
	CBlock* Find(char* Name, int Type=-1);			// find a block by name (and optionally by type)
	CBlock* OutputTable(char* Types=NULL, int* pCount=NULL);				// output memory table
public:													
	CList m_Blocks;									// list of blocks
};
