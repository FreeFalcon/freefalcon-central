#ifndef _LHSP_H
#define _LHSP_H

#include "falcvoice.h"

#define MAXPCMSIZE		1000
#define MAXCODESIZE		100

#define MAX_INDECODE_SIZE 5060
#define MAX_OUTDECODE_SIZE 80960


class LHSP
	{
	public:
		HANDLE			hAccess;
		long			PMSIZE, CODESIZE;
//		unsigned char	*lpInputUncoded;

		LHSP( void );
		~LHSP( void );
		void InitializeLHSP( void );
//		FILE *VoiceOpen( void );
		long ReadLHSPFile( COMPRESSION_DATA *input, unsigned char **buffer );
		void CleanupLHSP( void );
//		void VoiceClose( FILE	*falconVoiceFile );

	};

#endif  /* _LHSP_H */