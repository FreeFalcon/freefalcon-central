#ifndef TRAINING_SCRIPT_INCLUDED
#define TRAINING_SCRIPT_INCLUDED
#define MAX_LINE_LENGTH 1000
#define MAX_SCRIPTFUNCS 70
#define SCRIPTFUNC_SIZE 30

#include "InpFunc.h"

class TrainingScriptClass;

class ArgType {
public:

	char *arg;
	int arglength;
	ArgType *next;
	

	ArgType(void);
	~ArgType(void);
};

class LineType {

public:

	int function;

	ArgType *startarg;
	ArgType *endarg;
	int numargs;


	LineType *next;
	LineType *previous;

	LineType(void);
	~LineType(void);


	bool AddArg(char *argtext);
	char *GetArg(int argnum);

	long localtimer;
	unsigned int localcolor;
	float localcursorx;
	float localcursory;
	int localfont;
	int localsoundid;
	int localflash;
	int localtextboxed;
	int localtextorientation;


	
};

typedef bool (TrainingScriptClass::*ScriptFunctionType)(LineType *);

struct FunctionTableType
{
	char funcname[SCRIPTFUNC_SIZE];
	ScriptFunctionType function;
};

class StackType 
{
public:
	LineType *line;
	StackType *next;
};

class RepeatListType 
{
public:
	LineType *line;
	bool remove;
	RepeatListType *next;
};

class CaptureListType
{
public:
	InputFunctionType theFunc;
	int callback;
	CaptureListType *next;
	CaptureListType();
};



class TrainingScriptClass {

	private:

	LineType *startline;
	LineType *endline;
	LineType *lastline;
	LineType *nextline;

	int numlines;

	struct FunctionTableType *functiontable;
	int numfunctions;

	StackType *stacktop;

	RepeatListType *repeatlist;

	CaptureListType *capturelist;
	bool capturing;

	CaptureListType *blockallowlist;
	bool isblocklist;


	int stacksize;
	long timer;
	bool result;
	unsigned int color;
	float cursorx;
	float cursory;
	int font;
	int soundid;
	int flash;
	int textboxed;
	int textorientation;

	bool inrepeatlist;
	bool firstrun;
	bool incritical;

	bool CmdPrint(LineType *line);

	bool CmdWait(LineType *line);
	bool CmdWaitPrint(LineType *line);

	bool CmdWaitInput(LineType *line);
	bool CmdWaitMouse(LineType *line);
	bool CmdWaitSound(LineType *line);
	bool CmdWaitSoundStop(LineType *line);
	bool CmdSound(LineType *line);
	bool CmdIfTrue(LineType *line);

	bool CmdIfNotTrue(LineType *line);
	bool CmdEndSection(LineType *line);
	bool CmdEndScript(LineType *line);
	bool CmdJumpSection(LineType *line);
	bool CmdCallSection(LineType *line);

	bool CmdSetColor(LineType *line);
	bool CmdSetCursor(LineType *line);
	bool CmdSetFont(LineType *line);
	bool CmdOval(LineType *line);
	bool CmdLine(LineType *line);
	bool CmdEnterCritical(LineType *line);
	bool CmdEndCritical(LineType *line);
	
	bool CmdSimCommand(LineType *line);	
	bool CmdCallIf(LineType *line);
	bool CmdCallIfNot(LineType *line);
	bool CmdJumpIf(LineType *line);//write
	bool CmdWhile (LineType *line);
	bool CmdWhileNot(LineType *line);
	bool CmdClear(LineType *line);
	bool CmdClearLast(LineType *line);
	bool CmdSetFlash(LineType *line);
	bool CmdSetCursorCallback(LineType *line);
	bool CmdSetCursorDial(LineType *line);
	bool CmdWaitCallbackVisible(LineType *line);
	bool CmdWaitDialVisible(LineType *line);
	bool CmdSetTextBoxed(LineType *line);
	bool CmdMoveCursor(LineType *line);
	bool CmdSetTextOrientation(LineType *line);
	bool CmdBlock(LineType *line);
	bool CmdAllow(LineType *line);
	bool CmdSetViewCallback(LineType *line);
	bool CmdSetViewDial(LineType *line);
	bool CmdSetPanTilt(LineType *line);
	bool CmdMovePanTilt(LineType *line);
	bool CmdSetCursor3D(LineType *line);

	bool RunLine(LineType *line);

	void PushStack(LineType *returnline);
	LineType *PopStack();
	void ClearStack();

	bool AddLine(char *linetext);
	void ClearAllLines();

	void AddRepeatList(LineType *line);
	void ClearRepeatList();
	bool DelRepeatList(LineType *line);
	bool DelAllRepeatList();
	void RunRepeatList();
	void CleanupRepeatList();

	void ClearCaptureList();
	bool IsCaptured(InputFunctionType theFunc, int callback);

	void ClearBlockAllowList();
	bool AddBlockAllowCommand(InputFunctionType theFunc, int callback);

	void InitFunctions();
	int FindFunction(char *funcname);
	bool AddFunction(char *funcname, ScriptFunctionType function);




public:
	TrainingScriptClass(void);
  	~TrainingScriptClass (void);
	
	bool RunScript();
	bool LoadScript(char *name);
	bool RestartScript();
	bool CaptureCommand(InputFunctionType theFunc, int callback);
	bool IsCapturing() {return capturing;}
	bool IsBlocked(InputFunctionType theFunc, int callback);


};

extern TrainingScriptClass* TrainingScript;



#endif // TRAINING_SCRIPT_INCLUDED