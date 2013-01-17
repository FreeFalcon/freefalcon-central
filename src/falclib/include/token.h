 // MLR 12/13/2003 - Simple token parsing

float TokenF(float def); 
int TokenI(int def);
int TokenFlags(int def, char *flagstr);
char *TokenStr(char *def);


float TokenF(char *str, float def); 
int TokenI(char *str, int def);
int TokenFlags(char *str, int def, char *flagstr);
char *TokenStr(char *str, char *def);

int TokenEnum(char **enumnames, int def);
int TokenEnum(char *str, char **enumnames, int def);

void SetTokenString(char *);