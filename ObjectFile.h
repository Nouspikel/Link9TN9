#include "segment.h"
#include "label.h"
#include "Aorg.h"
/////////////////////////////////////////////////////////////////////////////////////
// CObjectfile class. Handles tagged object files
/////////////////////////////////////////////////////////////////////////////////////
class CObjectfile
{
protected:
	ifstream m_File;								// file stream
public:
	int m_Offset;
	int m_Start;
	~CObjectfile();
	char* m_Filename;
	char m_PsegName[9];
//	CObjectfile* m_pNext;
	CObjectfile();									// default constructor
	CObjectfile(char* filename);					// constructor with filename
	int FirstPass(CSegmentList *pSegments,CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs, char *PsegName, int GetDefs=0);		// first pass parser
	int SecondPass(CSegmentList *pSegments,CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs);		// second pass parser
	void Seek(int offset=0);						// rewind file
protected:
	int GetHex(int count=4);						// get hex value
	int GetLabel(char* pBuf, int size=6);			// get label ID
	int m_HasCR;
	int m_Compressed;
};

/////////////////////////////////////////////////////////////////////////////////////
// CLibrary class. Handles library files
/////////////////////////////////////////////////////////////////////////////////////
class CLibrary:public CObjectfile
{
public:
	CLibrary(char* filename);						// constructor with filename
	int Search(CSegmentList *pSegs, CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs, char *PsegName); // search for a target matching unresolved REF
	int SecondPass(CSegmentList *pSegments,CLabelList *pDefs, CLabelList *pRefs, CAorgList *pAorgs);		// second pass parser

protected:
	CList m_Modules;
	CList m_Objectfiles;
};

