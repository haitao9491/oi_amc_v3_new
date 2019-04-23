/*
 *	Filename   MemAlloc_T.h
 *	Description:	memory manager template class
 *
 */
#ifndef MEMALLOC_T_H_HEADER_INCLUDED_B7685972
#define MEMALLOC_T_H_HEADER_INCLUDED_B7685972

#include <stdlib.h>
#include <stdio.h>
#include "mutex.h"

enum MEM_ALLOC_MODE
{
	MEM_ALLOC_MODE_NEWDEL = 0,
	MEM_ALLOC_MODE_BLOCKLIST = 1,
	MEM_ALLOC_MODE_ANYELSE = 1000
};

// node in memory ,preserve per point of alloc memory
template<class Val_T>
class MemNode_T
{
public:
	typedef MemNode_T<Val_T> Node_T;
	MemNode_T(Node_T *n = NULL)
	{
		m_ptrNext = NULL;
	}

	~MemNode_T()
	{
	}

public:
	Node_T *m_ptrNext;
	Val_T  *m_ptrVal;
};

// manage memory alloc
template <class Val_T> class MemAlloc_T
{
	typedef MemNode_T<Val_T> Node_T;

public:
	int m_nNodeSumNum;
	int m_nNodeUsingNum;
	int m_nTotalMemSize;

	Node_T *m_ptrAllocHead;
	Node_T *m_ptrIdleHead;
	int	 m_nPerSize;
	void *m_lock;

	int m_nAllocMode;
	int m_nBaseSize;

	int m_nAllocCalledCount;
	int m_nHeavyAllocCalledCount;

	int m_nFreeCalledCount;
	int m_nHeavyFreeCalledCount;


public:
	MemAlloc_T(MEM_ALLOC_MODE nAllocMode = MEM_ALLOC_MODE_NEWDEL,
	//MemAlloc_T(MEM_ALLOC_MODE nAllocMode = MEM_ALLOC_MODE_BLOCKLIST,
				int nPerSize = 128,
				int nBaseSize = 8192 )
	{
		m_nAllocMode = nAllocMode;
		if (m_nAllocMode == MEM_ALLOC_MODE_NEWDEL) {
			return;
		}

		m_nNodeSumNum = 0;
		m_nNodeUsingNum = 0;
		m_ptrAllocHead = NULL;
		m_ptrIdleHead = NULL;
		m_nPerSize = nPerSize;	// alloc memory size
		m_nTotalMemSize = sizeof(MemAlloc_T);

		m_nBaseSize = nBaseSize;
		m_nAllocCalledCount = 0;
		m_nHeavyAllocCalledCount = 0;

		m_nFreeCalledCount = 0;
		m_nHeavyFreeCalledCount = 0;

		m_lock = mutex_open(NULL);

		// Pre alloc the base memory block
		InitBaseMemBlock();
	}

	int InitBaseMemBlock()
	{
		return IncreaseMem(m_nBaseSize);
	}

	// Increase pre-alloc memory, sized by nCount
	int IncreaseMem(int nCount)
	{
		int i = 0;
		Node_T *ptrNewNode = NULL;
		Val_T *ptrNewVal = NULL;

		if (nCount <= 0)
			return 0;

		for (i = 0; i < nCount; i ++)
		{
			// Alloc node memory
			ptrNewNode = new Node_T;
			if (!ptrNewNode) {
				return i;
			}

			// Alloc object memory
			ptrNewVal = new Val_T;
			if (!ptrNewVal){
				delete ptrNewNode;
				return i;
			}

			m_nTotalMemSize += (sizeof(Node_T) + sizeof(Val_T));
			m_nNodeSumNum ++;

			// bind new object with new object
			ptrNewNode->m_ptrVal = ptrNewVal;

			// link the alloced node to the head of idle queue
			ptrNewNode->m_ptrNext = m_ptrIdleHead;
			m_ptrIdleHead = ptrNewNode;
		}

		return nCount;
	}

	~MemAlloc_T()
	{
		int cnt = 0;
		Node_T *pTmp = NULL;
		Node_T *pNext = NULL;

		if (m_nAllocMode == MEM_ALLOC_MODE_NEWDEL) {
			return;
		}

		pTmp = m_ptrIdleHead;
		while (pTmp)
		{
			pNext = pTmp->m_ptrNext;
			if (pTmp->m_ptrVal)
				delete (Val_T *)(pTmp->m_ptrVal);

			delete (Node_T *)(pTmp);
			cnt++;
			pTmp = pNext;
		}

		pTmp = m_ptrAllocHead;
		while (pTmp)
		{
			pNext = pTmp->m_ptrNext;

			delete (Node_T *)(pTmp);
			cnt++;
			pTmp = pNext;
		}

		mutex_close(m_lock);
	}

	Val_T *fAlloc()
	{
		Node_T *ptrNode = NULL;
		Val_T  *ptrValue = NULL;

		if (m_nAllocMode == MEM_ALLOC_MODE_NEWDEL) {
			return new Val_T;
		}

		mutex_lock(m_lock);

		// No available idle node to use. Alloc m_nPerSize* first
		if (!m_ptrIdleHead)
		{
			if (IncreaseMem(m_nPerSize) <= 0) {
				mutex_unlock(m_lock);
				return NULL;
			}
			m_nHeavyAllocCalledCount++;
		}
		ptrNode = m_ptrIdleHead;

		// pop one idle node, idle queue head point to the next
		m_ptrIdleHead = m_ptrIdleHead->m_ptrNext;

		// link the popped node tho the alloced queue head
		ptrNode->m_ptrNext = m_ptrAllocHead;
		m_ptrAllocHead = ptrNode;

		m_nNodeUsingNum++;
		m_nAllocCalledCount++;

		ptrValue = ptrNode->m_ptrVal;

		mutex_unlock(m_lock);

		return ptrValue;
	}

	void fFree(Val_T *ptrVal)
	{
		Node_T *ptrTemp = NULL;

		if (!ptrVal)
			return;

		if (m_nAllocMode == MEM_ALLOC_MODE_NEWDEL) {
			delete ptrVal;
			return;
		}

		mutex_lock(m_lock);

		if (m_ptrAllocHead == NULL) {
			mutex_unlock(m_lock);
			return;
		}

		// put the "freeed" object to idle queue
		ptrTemp = m_ptrAllocHead;
		m_ptrAllocHead = m_ptrAllocHead->m_ptrNext;

		ptrTemp->m_ptrVal = ptrVal;
		ptrTemp->m_ptrNext = m_ptrIdleHead;
		m_ptrIdleHead = ptrTemp;

		m_nNodeUsingNum--;
		m_nFreeCalledCount++;

		// Reduce memory block when free nodes reached 3/4 of grant total
		// ReduceFreeMemBlock();

		mutex_unlock(m_lock);
	}

	int GetNoAllocCnt()
	{
		int cnt = 0;
		Node_T *pTmp = m_ptrIdleHead;

		while (pTmp != NULL)
		{
			pTmp = pTmp->m_ptrNext;
			cnt++;
		}

		return cnt;
	}

	int GetAllocCnt()
	{
		int cnt = 0;
		Node_T *pTmp = m_ptrAllocHead;

		while (pTmp != NULL)
		{
			pTmp = pTmp->m_ptrNext;
			cnt++;
		}

		return cnt;
	}

	int GetNodeSum()
	{
		return m_nNodeSumNum;
	}

private:
	unsigned int ReduceFreeMemBlock(void)
	{
		int i = 0;
		int cnt = 0;
		Node_T *ptrTemp = NULL;

		if (m_nNodeSumNum <= m_nBaseSize) {
			return 0;
		}

		if (m_nNodeUsingNum >= (m_nNodeSumNum >> 2)) {
			return 0;
		}

		for (i = 0; m_ptrIdleHead && (i < (m_nNodeSumNum >> 1)); i++)
		{
			ptrTemp = m_ptrIdleHead->m_ptrNext;

			if (m_ptrIdleHead->m_ptrVal)
				delete (Val_T *)(m_ptrIdleHead->m_ptrVal);
			delete (Node_T*)(m_ptrIdleHead);
			m_nTotalMemSize -= (sizeof(Val_T) + sizeof(Node_T));

			m_ptrIdleHead = ptrTemp;

			cnt++;

			m_nNodeSumNum--;
			if (m_nNodeSumNum <= m_nBaseSize) {
				break;
			}
		}
		m_nHeavyFreeCalledCount++;

		return cnt;
	}
};

#endif /* MEMALLOC_T_H_HEADER_INCLUDED_B7685972 */
