/////////////////////////////////////////////////////////////////////////////////////
// CScriptfile class. Handles the script file
/////////////////////////////////////////////////////////////////////////////////////
#include "objectfile.h"
#include "list.h"
#include "Block.h"
class CScriptfile
{
protected:
	int Find(char* pStr);
	ifstream m_File;										// file object
	char m_Buffer[256],*m_pBuf;								// line buffer
	char m_Path[256];										// file path
	char m_LibPath[256];									// library path
	char m_PsegName[9];										// default segment name for SEGMENT command
	void Generate(CList *pSelected, CList* pBlocks);		// generate memory image files
	void GetCode(CList *pFiles, CList* pLibraries, CSegmentList* pSegments, CLabelList* pDefs, CLabelList* pRefs, CAorgList* pAorgs, int flags=3); // load code into memory
	void FixSymtab(CLabelList* pDefs, CLabelList* pRefs, CSegmentList* pSegments); // fix symbol table
	void SearchLib(CList *pLibList, CSegmentList* pSegments, CLabelList* pDefs, CLabelList* pRefs, CAorgList* pAorgs, char* PsegName);	// seach libraries to solve undef labels
	void MatchRefs(CLabelList* pRefs, CLabelList *pDefs);	// try to match REFs with DEFs
	int GetCondition(char* pLine, int* pPtr);				// get a conditional statment
	int CheckCondition(char* pLine, int* pLevel, int* pTruth);	// verify condition
	int GetMath(char* pBuf, int* ptr, CLabelList* pEquates=NULL, CLabelList* pDefs=NULL, CLabelList* pRefs=NULL);	// get a math statment
	int Dispatch(CBlock* pBlock=NULL, int wipe=0);			// dispatch segments to blocks
	int GetCommand(char* Word, int max, char* pBuf, int* ptr);	// get label or command word from buffer
	int GetQuotedstring(char* Word, int max, char* pBuf, int* ptr);	// get a quoted string from buffer
	int GetLabel(char* Word, int max, char* pBuf, int* ptr);// get a label from buffer
	int GetNumber(int count, char* pBuf, int* ptr);			// get a number, decimal by default
	int GetHex(int count, char* pBuf, int* ptr);			// get an hexadecimal number
	int HasWildcards(char* s);								// check if a string contains wildcards
	int Match(char* s, char* Pattern);						// match a string with a wildcard-containing pattern
	int Term(CSegment* pSeg, char* pBuf, int * ptr);		// process segment flags as a logical expression
	char m_Tables[80];										// tables to output (string of single-char flags)
	CSegmentList m_Segments;								// list of all segments
	CList m_SegList;										// list of selected segments
	CLabelList m_Defs;										// list of DEF labels
	CLabelList m_Refs;										// list of REF labels
	CLabelList m_Equates;									// list of internal labels
	CAorgList m_Aorgs;										// list of AORG intervals
	CBlockList m_Blocks;									// list of blocks, devices and models
	CList m_Objectfiles;									// list of object files
	CList m_Libraries;										// list of library files
	CList m_Selected;										// list of selected memory blocks/devices/models
	CList m_Memimages;										// list of memory image files
	CList m_Patches;										// list of patches to apply
	CList m_Searches;										// find commands for 2nd pass
	int m_MinGap;											// minimum gap to split memimage files
	int m_MaxSize;											// maximum file memimage size (without header)
	int m_Strategy;											// segment iteration strategy (0=first, 1=largest)

public:
	~CScriptfile();
	CScriptfile();											// default constructor
	CScriptfile(char* filename);							// constructor with file name
	int Open(char* filename);								// open the file
	int ParseFile(ifstream* pFile=NULL);					// parse the file line by line
	int ParseLine(char* pLine);								// parse line of text in buffer
	int GetParam(char* Word, int max, char* pBuf, int* ptr, char sep=',');	// get a parameter from buffer, using separator
	int Process();											// go ahead with second pass
};

