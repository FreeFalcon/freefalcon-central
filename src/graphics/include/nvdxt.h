int fileout;

void ConvertToNormalMap(int kerneltype,int colorcnv,int alpha,float scale,int minz,
						bool wrap,bool bInvertX,bool bInvertY,int w,int h,int bits,void * data);
void ReadDTXnFile(unsigned long count, void * buffer);
void WriteDTXnFile(unsigned long count, void *buffer);
