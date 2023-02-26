/////////////////////////////////////////////////////////////////////////////////////
// CList class template. Handles a basic list of objects
/////////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <malloc.h>
#include "List.h"
//----------------------------------------------------------------------------------
// Constuctor (default)
//----------------------------------------------------------------------------------
//template <class T> CList<T>::CList()
CList::CList()
{
	m_Chunk=m_Max=16;										// default increment size
	m_Size=0;												// initial list size
	m_pBuffer=malloc(4*m_Chunk);							// allocate storage space (4 bytes per address)
}

//----------------------------------------------------------------------------------
// Constuctor (specifying buffer increment)
//----------------------------------------------------------------------------------
//template <class T> CList<T>::CList(int Chunk)
CList::CList(int Chunk)
{
	m_Chunk=m_Max=Chunk;									// user-specified increment size 
	m_Size=0;												// initial list size
	m_pBuffer=malloc(4*m_Chunk);
}

//----------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------
//template <class T> CList<T>::~CList()
CList::~CList()
{
	free(m_pBuffer);										// free entire buffer space
}

//----------------------------------------------------------------------------------
// Return current # of elements
//----------------------------------------------------------------------------------
//template <class T> int CList<T>::GetSize()
int CList::GetSize()
{
	return m_Size;											// return current size
}

//----------------------------------------------------------------------------------
// Append an element at the end of the list
//----------------------------------------------------------------------------------
//template <class T> int CList<T>::Add(T* element)
int CList::Add(PTR element)
{
	void* pTemp;

	if(m_Size>=m_Max)										// not enough space
	{
		m_Max+=m_Chunk;										// increase size
		pTemp=malloc(4*(m_Max));							// create larger buffer
		memcpy(pTemp,m_pBuffer,4*m_Size);					// copy old into new
		free(m_pBuffer);									// get rid of old buffer
		m_pBuffer=pTemp;									// use the new one
	}
	
	*((int*)m_pBuffer+m_Size)=(int)element;					// cast to int* as void has no size

	return m_Size++;										// return index of element and increase size
}

//----------------------------------------------------------------------------------
// Insert an element at a given position
//----------------------------------------------------------------------------------
int CList::InsertAt(PTR element, int pos)
{
	int i,j;
	void* pTemp;

	if(pos>m_Size) pos=m_Size;								// insert at end

	if(m_Size>=m_Max) m_Max+=m_Chunk;						// not enough space
	pTemp=malloc(4*(m_Max));								// create large enough buffer

	for(i=0;i<pos;i++)										// copy what comes before new element
	{
		*((int*)pTemp+i)=*((int*)m_pBuffer+i);				// cast to int* as void has no size
	}
	*((int*)pTemp+i)=(int)element;							// copy new element
	for(j=i+1;i<m_Size;i++,j++)								// copy what comes after new element
	{
		*((int*)pTemp+j)=*((int*)m_pBuffer+i);
	}

	free(m_pBuffer);										// get rid of old buffer
	m_pBuffer=pTemp;										// use the new one	
	m_Size++;												// increase size
	return pos;												// return index of element

}

//----------------------------------------------------------------------------------
// Removes an element from the list, don't delete the object
//----------------------------------------------------------------------------------
PTR CList::RemoveAt(int pos)
{
	PTR old;
	
	if(pos<0) pos+=m_Size;									// negative means: start from the end
	if((pos<0)||(pos>=m_Size)) return NULL;					// check range

	old=(PTR)*((PTR)m_pBuffer+pos);							// remember old element pointer

	m_Size--;												// decrement size

	for(;pos<m_Size;pos++)									// from position to end-1
	{
		*((int*)m_pBuffer+pos)=*((int*)m_pBuffer+pos+1);	// overwrite current with next
	}

	return old;
}

//----------------------------------------------------------------------------------
// Return a pointer to a given element
//----------------------------------------------------------------------------------
//template <class T> T* CList<T>::GetAt(int i)
	PTR CList::GetAt(int i)
{
	if((i<0)||(i>=m_Size)) return NULL;						// sanity check

//	return (*((int*)m_pBuffer+i);							// return pointer
	return (PTR)*((PTR)m_pBuffer+i);						// return element i
}

//----------------------------------------------------------------------------------
// Empty the list
//----------------------------------------------------------------------------------
//template <class T> void CList<T>::RemoveAll()
void CList::RemoveAll(int delall)
{
	int i;
//	T* pElement;
	PTR pElement;

	if(delall)
	{
		for(i=0;i<m_Size;i++)									// iterate the whole list
		{
	//		pElement=(T*)*((int*)m_pBuffer+i);					// cast to int* as void has no size
			pElement=(PTR)*((PTR)m_pBuffer+i);					// cast to int* as void has no size
			delete(pElement);									// delete this element
		}
	}

	m_Size=0;
}

