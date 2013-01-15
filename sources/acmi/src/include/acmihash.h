#ifndef _ACMI_HASH_TABLE_H_
#define _ACMI_HASH_TABLE_H_

class ACMI_HASHNODE
{
	public:
		VU_ID ID;
		long  Index;
		char  label[16];
		long  color;
		ACMI_HASHNODE *Next;
};

class ACMI_HASHROOT
{
	public:
		ACMI_HASHNODE *Root_;
};

class ACMI_Hash
{
	private:
		long ID_;
		unsigned long TableSize_;
		ACMI_HASHROOT *Table_;

	public:

		ACMI_Hash();
		~ACMI_Hash();

		void Setup(unsigned long Size);
		void Cleanup();

		ACMI_HASHNODE *Get(VU_ID ID);
		long Find(VU_ID ID);

		long Add(VU_ID ID,char *lbl,long color);

		void Remove(VU_ID ID);

		long GetFirst(ACMI_HASHNODE **cur,unsigned long *curidx);
		long GetNext(ACMI_HASHNODE **cur,unsigned long *curidx);

		long GetLastID() { return(ID_); }
};
#endif
