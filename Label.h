/////////////////////////////////////////////////////////////////////////////////////
// Label class. Holds label information
/////////////////////////////////////////////////////////////////////////////////////
#include "List.h"
class CLabel
{
public:
	int m_Undefined;			// flag: label is undefined
	char m_Name[9];				// 8-char label name
	int m_Value;				// label value or first link for REFs
	int m_SegIdx;				// index of segment in segment list
	char m_Type;				// type of label (tag)

	CLabel();
	CLabel(char* Label, int Value, int Tag, int SegNb);
};

class CLabelList
{
public:
	int CountUndef(int Sref);
	~CLabelList();
	int GetSize();
	CLabel* OutputTable(char* Types, int Count=0);
	CLabel* Append(char *Label,int Value, int Tag=0, int SegNb=0);
	CLabel* SetValue(char* Label, int Value, int Tag=-1, int SegNb=-1, int Undef=-1);
	CLabel* Find(char *Label, int Undef=0);
	int GetValue(char *Label);
	CList m_Labels;
};
