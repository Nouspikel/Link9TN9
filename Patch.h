#include "List.h"

class CPatch
{
public:
	~CPatch();
	int Append(int Value);
	CPatch(int Type, char* Segname, int Address, int Value1);
	char* m_SegName;
	int m_Type;
	CList m_Values;
	int m_Address;
};
