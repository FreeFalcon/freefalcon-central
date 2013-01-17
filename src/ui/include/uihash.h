#ifndef _GENERIUI_HASH_TABLE_H_
#define _GENERIUI_HASH_TABLE_H_

class UI_HASHNODE
{
	public:
		unsigned long ID;
		void *Record;
		UI_HASHNODE *Next;
};

class UI_HASHROOT
{
	public:
		UI_HASHNODE *Root_;
};

class UI_Hash
{
	private:
		unsigned long TableSize_;
		UI_HASHROOT *Table_;

		void (*Callback_)(void *rec);

	public:

		UI_Hash();
		~UI_Hash();

		void Setup(unsigned long Size);
		void Cleanup();

		void SetCallback(void (*cb)(void*)) { Callback_=cb; }

		void *Find(unsigned long ID);

		void Add(unsigned long ID,void *rec);

		void Remove(unsigned long ID);

		void *GetFirst(UI_HASHNODE **cur,unsigned long *curidx);
		void *GetNext(UI_HASHNODE **cur,unsigned long *curidx);
};
#endif
