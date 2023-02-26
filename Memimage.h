/////////////////////////////////////////////////////////////////////////////////////
// CMemimage class. Handles memory image files
/////////////////////////////////////////////////////////////////////////////////////
#include "List.h"
enum {MF_NONE,MF_EA5,MF_GK,MF_FB6,MF_BIN};
class CMemimage
{
public:
	void CloseFile();
	int m_Incremented;
	int m_Page;
	int m_MemType;
	CList* m_pAorgs;
	int m_MaxSize;
	int NewFile(int Size, int Address, int More=0, char* pBlockname=NULL, int Page=0, int Memtype=0);
	int m_Lastchar;
	int NextFile(int Size, int Address, int More=0);
	CList m_Blocks;									// list of blocks
	CList m_Segments;								// list of segments
	int m_Format;									// format of the file
public:
	CMemimage();									// default constructor
	CMemimage(char* filename, int type);			// constructor with filename
	~CMemimage();
	void Seek(int offset=0);						// rewind file
	int OutputCode(CList* pSelected, CList* pBlocks);	// generate file contents
protected:
	ofstream m_File;								// file stream
	char *m_Filename;								// file name
	char m_Realname[255];
protected:
	int MakeHex(char*s, int n);
	int MakeNumber(char* s, int n);					// format an hex number
};
