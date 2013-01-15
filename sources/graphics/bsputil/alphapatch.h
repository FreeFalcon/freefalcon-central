/***************************************************************************\
	AlphaPatch.h
    Scott Randolph
    April 2, 1998

    Keeps a list of polygon/vertex combinations which have overriden
	alpha values.
\***************************************************************************/
#ifndef _ALPHAPATCH_H_
#define _ALPHAPATCH_H_

extern class AlphaPatchClass		TheAlphaPatchList;


typedef struct AlphaPatchRecord {
	char	name[256];
	float	alpha;

	BOOL	OLD;			// These should go, but for backward compatability....
	float	r, g, b;		// These should go, but for backward compatability....

	AlphaPatchRecord *next;
} AlphaPatchRecord;

class AlphaPatchClass {
public:
	AlphaPatchClass()	{};
	~AlphaPatchClass()	{};

	void Setup(void);
	void Load( char *filename );
	void Cleanup(void);
	
	void				AddPatch( char *name, float alpha, float r, float g, float b );
	void				AddPatch( char *name, float alpha );
	AlphaPatchRecord*	GetPatch( char *name );

	AlphaPatchRecord	*PatchList;
};

#endif // _ALPHAPATCH_H_

