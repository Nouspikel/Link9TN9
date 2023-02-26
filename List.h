/////////////////////////////////////////////////////////////////////////////////////
// CList class template. Handles a basic list of objects
/////////////////////////////////////////////////////////////////////////////////////
#ifndef CLIST
#define CLIST
#define	PTR	int*
class CList
{
protected:
	void* m_pBuffer;										// storage space for array of pointers
	int m_Size;												// current list size
	int m_Max;												// maximum size given storage space
	int m_Chunk;											// increment for storage space
public:
	PTR RemoveAt(int pos);
	int InsertAt(PTR element, int pos);
	CList();												// constructor
	CList(int Chunk);										// constructor with different increment
	~CList();												// destructor
	int Add(PTR element);									// add an element to the list
	PTR GetAt(int i);										// return pointer to a given element
	int GetSize();											// return current size
	void RemoveAll(int delall=1);							// remove all elements (delete objects)
};
#endif
/*
template <class T> class CList
{
protected:
	void* m_pBuffer;										// storage space for array of pointers
	int m_Size;												// current list size
	int m_Max;												// maximum size given storage space
	int m_Chunk;											// increment for storage space
public:
	CList();												// constructor
	CList(int Chunk);										// constructor with different increment
	~CList();												// destructor
	int Add(T* element);									// add an element to the list
	T* GetAt(int i);										// return pointer to a given element
	int GetSize();											// return current size
	void RemoveAll();										// remove all elements
};
*/
