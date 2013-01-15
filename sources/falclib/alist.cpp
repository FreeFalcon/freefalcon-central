#include "AList.h"
#include "f4thread.h"
//#include "f4compress.h"

AList::AList()
{
	Init();
}

void AList::Init(void)
{
	_Head._Pred=0;
	_Head._Succ=&_Tail;
	_Tail._Pred=&_Head;
	_Tail._Succ=0;
}

void *AList::RemHead()
{
	if(_Head._Succ->_Succ)
	{
		ANode *n = _Head._Succ;

		return(n->Remove());
	}
	return(0);
}

void *AList::RemTail()
{
	if(_Tail._Pred->_Pred)
	{
		ANode *n = _Tail._Pred;

		return(n->Remove());
	}
	return(0);
}

void AList::AddHead(ANode *n)
{
	n->InsertAfter(&_Head);
}

void AList::AddTail(ANode *n)
{
	n->InsertBefore(&_Tail);
}

void *AList::GetHead(void)
{
	if(_Head._Succ->_Succ)
	{
		return((void *)_Head._Succ);
	}
	return(0);
}

void *AList::GetTail(void)
{
	if(_Tail._Pred->_Pred)
	{
		return((void *)_Tail._Pred);
	}
	return(0);

}

void AList::AddSorted(ANode *n)
{
	ANode *a;
	
	a=(ANode *)GetHead();
	while(a)
	{
		if(a->CompareWith(n) > 0)
		{
			n->InsertBefore(a);
			return;
		}
		a=(ANode *)a->GetSucc();
	}
	AddTail(n);
}


ANode::ANode() 
{
	_Pred=_Succ=0;
}

ANode::~ANode() 
{
}


void *ANode::GetSucc() 
{
	if(_Succ->_Succ)
		return ((void *)_Succ);
	return(0);
}

void *ANode::GetPred() 
{
	if(_Pred->_Pred)
		return ((void *)_Pred);
	return(0);

}



void *ANode::Remove(void)
{
	// update links in other nodes
	_Pred->_Succ=_Succ;
	_Succ->_Pred=_Pred;

	_Pred=_Succ=0;

	return(this);
}

void ANode::InsertAfter(ANode *n)
{
	// update links in other nodes
	_Succ=n->_Succ;
	_Pred=n;

	_Succ->_Pred=this;
	_Pred->_Succ=this;

}

void ANode::InsertBefore(ANode *n)
{
	// update links in other nodes
	_Succ=n;
	_Pred=n->_Pred;

	_Succ->_Pred=this;
	_Pred->_Succ=this;

}

ProtectedAList::ProtectedAList()
{
#ifndef F4COMPRESS
	listLock=F4CreateCriticalSection("alist");
#endif
}

ProtectedAList::~ProtectedAList()
{
#ifndef F4COMPRESS
	F4DestroyCriticalSection(listLock);
#endif
}

void ProtectedAList::Lock(void)
{
#ifndef F4COMPRESS
	F4EnterCriticalSection(listLock);
#endif
}

void ProtectedAList::Unlock(void)
{
#ifndef F4COMPRESS
	F4LeaveCriticalSection(listLock);
#endif
}
