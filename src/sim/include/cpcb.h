#ifndef _CPCB_H
#define _CPCB_H

extern const int TOTAL_CPCALLBACK_SLOTS, TOTAL_BUTTONCALLBACK_SLOTS;

typedef void (*CPCallback)(void *);
typedef void (*ButtonCallback) (void *, int);

/** callbacks for everyting but buttons */
typedef struct {
	CPCallback	ExecCallback;
	CPCallback	EventCallback;
	CPCallback	DisplayCallback;
} CPCallbackStruct;
/** array of callbacks */
extern CPCallbackStruct	CPCallbackArray[];

/** button callbacks */
typedef struct {
	ButtonCallback TransAeroToState;
	ButtonCallback	TransStateToAero;
} ButtonCallbackStruct;
/** array of button callbacks */
extern ButtonCallbackStruct ButtonCallbackArray[];

#endif
