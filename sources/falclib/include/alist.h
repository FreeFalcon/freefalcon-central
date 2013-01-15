#ifndef _ALIST_MLR
#define _ALIST_MLR

class ANode
{
public:
    ANode();
	void *GetSucc();
	void *GetPred();
	void *Remove();
	void InsertAfter(ANode *n);
	void InsertBefore(ANode *n);
	virtual int CompareWith(ANode *n) {return 0;};
	~ANode();
private:
	friend class AList;
	class ANode *_Pred, *_Succ;
};

class AList
{
public:
	AList();
	void Init(); // reinit list to being empty
	void *RemHead(void);
	void *RemTail(void);
	void AddHead(ANode *n);
	void AddTail(ANode *n);
	void *GetHead(void);
	void *GetTail(void);
	void *Find(int Offset, int Value);
	void *Find(int Offset, float Value);
	void AddSorted(ANode *n);
private:
	ANode _Head, _Tail;
};

class ProtectedAList : public AList
{
public:
	ProtectedAList();
	~ProtectedAList();
	void Lock(void);
	void Unlock(void);
private:
	struct F4CSECTIONHANDLE*  listLock;
};

#define ITERATE_ALIST(List,NodePtr,Type) for((NodePtr)=(Type *)(List)->GetHead();(NodePtr);(NodePtr)=(Type *)(NodePtr)->GetSucc())
#define FIND_ANODE(ListPtr,Variable,Value) 

#endif