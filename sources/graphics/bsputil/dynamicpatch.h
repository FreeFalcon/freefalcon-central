/***************************************************************************\
	DynamicPatch.h
    Scott Randolph
    April 8, 1998

    Keeps a list of polygon/vertex combinations which runtime moveable
	(ie: Dyanmic vertices).
\***************************************************************************/
#ifndef _DYANMICPATCH_H_
#define _DYANMICPATCH_H_

extern class DynamicPatchClass		TheDynamicPatchList;


typedef struct DynamicPatchRecord {
	char	name[256];

	DynamicPatchRecord *next;
} DynamicPatchRecord;

class DynamicPatchClass {
public:
	DynamicPatchClass()	{};
	~DynamicPatchClass()	{};

	void Setup(void);
	void Load( char *filename );
	void Cleanup(void);
	
	void	AddPatch( char *name );
	BOOL	IsDynamic( char *name );

	DynamicPatchRecord	*PatchList;
};

#endif // _ALPHAPATCH_H_

