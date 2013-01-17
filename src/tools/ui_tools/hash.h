#ifndef _C_HASH_TABLE_H_
#define _C_HASH_TABLE_H_

typedef struct C_HashRoot C_HASHROOT;
typedef struct C_HashNode C_HASHNODE;

enum
{
	HSH_REMOVE			=0x00001,
};

struct C_HashNode
{
	long ID;
	void *Record;
	C_HASHNODE *Next;
};

struct C_HashRoot
{
	C_HashNode *Root_;
};

class C_Hash
{
	private:
		long flags_;
		long TableSize_;
		C_HASHROOT *Table_;

		long curidx_;
		C_HASHNODE *Current_;

	public:

		C_Hash();
		~C_Hash();

		void Setup(long Size);
		void Cleanup();

		void SetFlags(long flg) { flags_=flg; }
		long GetFlags() { return(flags_); }

		void *Find(long ID);
		char *FindText(long ID);
		long FindTextID(long ID);
		long FindTextID(char *txt);

		void Add(long ID,void *rec);
		long AddText(char *string);
		long AddTextID(long ID,char *string);

		void Remove(long ID);

		void *GetFirst();
		void *GetNext();
};
#endif
