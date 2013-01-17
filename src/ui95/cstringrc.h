#ifndef _STRING_RESOURCE_H_
#define _STRING_RESOURCE_H_

enum
{
	_STR_HASH_SIZE_=512,
};

class C_String
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		C_Hash *Root_;  // All strings here
		long   *IDTable_; // All Userdefined IDs here (array points to Hash Table)
		long    IDSize_;
		long	LastID_;

	public:
		C_String();
		C_String(char **stream);
		C_String(FILE *fp);
		~C_String();
		long Size();
		void Save(char **stream);
		void Save(FILE *fp);

		void Setup(long NumIDs);
		void Cleanup();

		BOOL AddString(long ID,_TCHAR *str);
		_TCHAR *GetString(long ID);
		BOOL RemoveString(long)	{ return FALSE;	}

		long AddText(const _TCHAR *str);
		_TCHAR *GetText(long ID);
		BOOL RemoveText(long)	{ return FALSE;	}

		long GetLastID() { return(LastID_); }

#ifdef _UI95_PARSER_

		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *str,C_Handler *);
		void SaveText(HANDLE,C_Parser *)	{ ; }

#endif // PARSER
};

extern C_String *gStringMgr;

#endif
