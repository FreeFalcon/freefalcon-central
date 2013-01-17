#ifndef DATAFILE_H
#define DATAFILE_H

// DatFile Parser format control


struct InputDataDesc {
    char *name; // name of the key in the .dat file
    enum  format { ID_INT, ID_FLOAT, ID_STRING, ID_CHAR, ID_VECTOR, ID_FLOAT_ARRAY, ID_LOOKUPTABLE, ID_2DTABLE } type; // what format it is //Cobra 10/30/04 TJL
    unsigned int offset; // offset into the associated structure
    char *defvalue; // default value
};

class FileReader {
    const InputDataDesc *m_desc;
    const InputDataDesc* FindField(char *key);
    public:
	FileReader(const InputDataDesc *desc) : m_desc(desc) {};
	virtual void Initialise(void *dataPtr);
	virtual bool ParseField(void *dataPtr, const char *line);
	bool AssignField (const InputDataDesc *desc, void *dataPtr, const char *value);
};

extern bool AssignField (const InputDataDesc *field, void *dataPtr, const char *value);
extern const InputDataDesc* FindField(const InputDataDesc *desc, const char *key);
extern bool ParseField(void *dataPtr, const char *line, const InputDataDesc *desc);
extern void Initialise (void *dataPtr, const InputDataDesc *desc);
class SimlibFileClass;
extern bool ParseSimlibFile(void *dataPtr, const InputDataDesc *desc, SimlibFileClass* inputFile);

#endif
