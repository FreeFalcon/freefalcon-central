#ifndef CONVERT_H
#define	CONVERT_H

#define TEXCODELEN		16384			// Number of possible textures
#define FILENAMELEN		14				// Length of each texture name entry

extern short	MaxTextureType;

extern void InitConverter (char *filename);

extern void CleanupConverter (void);

extern char* GetFilename (short x, short y);

extern int GetTextureIndex (short x, short y);

extern char* GetTextureId (int index);

extern int readTexCodes( char *codeFile);
	
extern int readMap(char *mapFile);

#endif   /* _CONVERT_ */

