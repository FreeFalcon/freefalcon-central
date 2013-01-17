#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct IDStr IDLIST;

struct IDStr
{
	char Label[41];
	long Value;
	long fileno;
};

#define MAX_IDS 10000

long IDCount=0;
IDLIST UserIDs[MAX_IDS];

long FindString(char *IDStr)
{
	int i;

	for(i=0;i<IDCount;i++)
	{
		if(!stricmp(&UserIDs[i].Label[0],IDStr))
			return(i);
	}
	return(-1);
}

long FindValue(long Value)
{
	int i;

	return(-1);
	for(i=0;i<IDCount;i++)
	{
		if(UserIDs[i].Value == Value)
			return(i);
	}
	return(-1);
}

void SortIDList()
{
	int i,j;
	IDLIST temp;

	for(i=1;i<IDCount;i++)
		for(j=0;j<i;j++)
//			if(stricmp(UserIDs[i].Label,UserIDs[j].Label) < 0)
			if(UserIDs[i].Value < UserIDs[j].Value)
			{
				memcpy(&temp,&UserIDs[i],sizeof(IDLIST));
				memcpy(&UserIDs[i],&UserIDs[j],sizeof(IDLIST));
				memcpy(&UserIDs[j],&temp,sizeof(IDLIST));
			}
}

void main(int argc,char **argv)
{
	FILE *ifp,*ofp,*lfp;
	char *token,*token2;
	char buffer[256];
	long ID,curfileno,Value,i;

	printf("GENIDS Ver 1.0 - Create a header file from ALL the USERIDS.ID Files\n");

	if(argc != 3)
	{
		printf("Usage: GENIDS <input file> <output file>\n");
		printf("where  <input file> contains a list if USERIDS.ID files you want to process\n");
		printf(" and   <output file> is the '.h' file which will be included in falcon4\n");
		printf("Note:  The program can handle a MAXIMUM of 10000 unique IDs\n");
		return;
	}

	lfp=fopen(argv[1],"r");
	if(lfp == NULL)
	{
		printf("<input file> (%s) NOT found... exitting\n",argv[1]);
		return;
	}
	ofp=fopen(argv[2],"w");
	if(ofp == NULL)
	{
		printf("Can't open <output file> (%s)...exitting\n",argv[2]);
		fclose(lfp);
		return;
	}
	curfileno=0;

	while(fgets(buffer,250,lfp) > 0)
	{
		curfileno++;
		token=strtok(buffer," \n");
		if(token != NULL)
			ifp=fopen(token,"r");
		else
			ifp=NULL;
		if(ifp != NULL)
		{
			printf("processing %s\n",buffer);

			while(fgets(buffer,200,ifp) > 0)
			{
				token=strtok(buffer," \t\n");
				if(token != NULL)
				{
					token2=strtok(NULL," \t\n");
					if(token2 != NULL)
					{
						ID=FindString(token);
						if(ID >= 0)
							printf("Duplicate ID [%s] in file #%1d of <%s> (first found in file #%1d)\n",token,curfileno,argv[1],UserIDs[ID].fileno);
						else
						{
							Value=atol(token2);
							ID=FindValue(Value);
							if(ID >= 0)
								printf("Duplicate Value [%s][%1ld] in file #%1d of <%s> (first found in file #%1d [%s][%1ld])\n",token,Value,curfileno,argv[1],UserIDs[ID].fileno,UserIDs[ID].Label,UserIDs[ID].Value);
							else if(IDCount < MAX_IDS)
							{ // Add ID to List
								strcpy(&UserIDs[IDCount].Label[0],token);
								UserIDs[IDCount].Value=Value;
								UserIDs[IDCount].fileno=curfileno;
								IDCount++;
							}
							else
								printf("***ERROR*** MAX # of IDs Exceeded (%1ld)\n",MAX_IDS);
						}
					}
				}
			}
			fclose(ifp);
		}
		else
			printf("Can't open ID file (%s)\n",buffer);
	}
	fclose(lfp);

	SortIDList();

	printf("Saving Header file (%s)\n",argv[2]);
	// Save Header file
	fprintf(ofp,"// USERIDS Header file generated on [DATE]\n\n");
	fprintf(ofp,"#ifndef _USERIDS_H_\n#define _USERIDS_H_\n\n");

	fprintf(ofp,"enum\n{\n");

	for(i=0;i<IDCount;i++)
		fprintf(ofp,"\t%-40s    =%1ld,\n",UserIDs[i].Label,UserIDs[i].Value);

	fprintf(ofp,"};\n\n#endif\n");
	fclose(ofp);
}
