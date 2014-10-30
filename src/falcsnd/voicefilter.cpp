#include <windows.h>
#include <math.h>
#include <windowsx.h>
#include <time.h>
#include "fsound.h"
#include "resource.h"
#include "conv.h"
#include "VoiceManager.h"
#include "F4Find.h"
#include "sim/include/Phyconst.h"
#include "playerop.h"
#include "campaign/include/cmpclass.h"
#include "campaign/include/find.h"
#include "vutypes.h"
#include "dispcfg.h"
#include "ui/include/falcuser.h"
#include "sim/include/simdrive.h"
#include "fsound.h"
#include "sim/include/aircrft.h"
#include "flight.h"
#include "squadron.h"
#include "falcsess.h"

#include "radiosubtitle.h" // Retro 20Dec2003 for subtitles
#include "sim/include/navsystem.h" // Retro 20Dec2003 for subtitles

extern BOOL killThread;
extern float MAX_RADIO_RANGE;
extern float RADIO_PROX_RANGE;
extern VU_TIME vuxGameTime;
extern char FalconSoundThrDirectory[];

#ifdef USE_SH_POOLS
extern MEM_POOL gTextMemPool;
#endif

char *RadioStrings[16] =
{
    "OFF",
    "FLIGHT1",//flight
    "FLIGHT2",
    "FLIGHT3",
    "FLIGHT4",
    "FLIGHT5",
    "PACKAGE1",//pacage
    "PACKAGE2",
    "PACKAGE3",
    "PACKAGE4",
    "PACKAGE5",
    "FROM PACKAGE",
    "PROXIMITY",
    "GUARD",
    "BROADCAST",
    "TOWER"
};

enum
{
    PLAY_NOTHING,
    PLAY_MESSAGE,
    PLAY_FRAG,
};

typedef struct
{
    int mode;
    int talker;
    int message;
    int frag;
    int numEvals;
    short element[15];
    short data[15];
    short maxs[15];
    short eval[15];
} VoiceToolData;

//#define NUM_FRAGS  2555

VoiceToolData VToolMsgData = {0, 0, 0, 0, {0}, {0}};
#if 0
short evalLastFrag[LastEval][NUM_VOICES] = {FALSE};
char fragsPlayed[NUM_FRAGS * NUM_VOICES] = {FALSE};
#else
short *evalLastFrag;
char *fragsPlayed;
#endif

//#define NOCALLSIGNS 1

#define EBEARING_MIN 0
#define EBEARING_MAX 71
#define EANGELS_MIN 0
#define EANGELS_MAX 45
#define ERANGE_MIN 0
#define ERANGE_MAX 34 // 2001-09-15 M.N.
#define ETHOUSANDS_MIN 1
#define ETHOUSANDS_MAX 54
#define EAIRSPEED_MIN 0
#define EAIRSPEED_MAX 47


int voice_ = 0; //default voice to play

void *map_file(char *filename, long bytestomap = 0);
extern VoiceFilter *voiceFilter;
VoiceManager *VM;
extern HINSTANCE hInst;
extern HANDLE VMWakeEventHandle;
//C_Hash *fragTable = NULL;

VoiceFilter::VoiceFilter(void)
{
    srand((unsigned)time(NULL));
}

VoiceFilter::~VoiceFilter(void)
{
    CleanUpVoiceFilter();
    EndVoiceManager();
}

void VoiceFilter::StartVoiceManager(void)
{
    VM = new VoiceManager();

    if (VM)
        VM->VMBegin();
}

void VoiceFilter::EndVoiceManager(void)
{
    delete VM;
    VM = NULL;
}

void VoiceFilter::ResetVoiceManager(void)
{
    if (VM)
        VM->VMResetVoices();
}
/*
void VoiceFilter::ResumeVoiceStreams( void )
{
if ( VM )
VM->VMResumeVoiceStreams();
}*/

void VoiceFilter::HearVoices()
{
    if (VM)
        VM->VMHearVoices();
}

void VoiceFilter::SilenceVoices()
{
    if (VM)
        VM->VMSilenceVoices();
}

void VoiceFilter::SetUpVoiceFilter(void)
{
    // int i;

    LoadCommFile();
    LoadEvalFile();
    LoadFragFile();

    /*
    mesgTable = new MESG_REPEAT_LOOKUP[LastComm];
    for ( i = 0; i < LastComm; i++ )
    {
    mesgTable[i].mesgID = i;
    mesgTable[i].timeCalled = 0;
    }*/
}

void VoiceFilter::CleanUpVoiceFilter(void)
{
    // delete [] mesgTable;

    DisposeCommData();
    DisposeEvalData();
    DisposeFragData();

}



/****************************************************************************

  LoadCommFile

 Purpose: Load Conversation Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::LoadCommFile(void)
{
    char filename[MAX_PATH];

    sprintf(filename, "%s\\commFile.bin", FalconSoundThrDirectory);

    // commData = (char *)map_file(filename);

    // ShiAssert( commData );
    if (commfile.Open(filename) not_eq TRUE)
        ShiError("Can't open commfile.bin");

    commfile.Initialise();
}

/****************************************************************************

  DisposeCommData

 Purpose: Dispose Coversation Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::DisposeCommData(void)
{
    //commData = NULL;
    commfile.Close();
}

/****************************************************************************

  LoadEvalFile

 Purpose: Load Eval Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::LoadEvalFile(void)
{
    char filename[MAX_PATH];

    sprintf(filename, "%s\\evalFile.bin", FalconSoundThrDirectory);

    // evalData = (char *)map_file(filename);

    // ShiAssert( evalData );
    if (evalfile.Open(filename) not_eq TRUE)
        ShiError("Can't open evalFile.bin");

    evalfile.Initialise();
}

/****************************************************************************

  DisposeEvalData

 Purpose: Dispose Eval Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::DisposeEvalData(void)
{
    //evalData = NULL;
    evalfile.Close();
}

/****************************************************************************

  LoadFragFile

 Purpose: Load Frag Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::LoadFragFile(void)
{
    char filename[MAX_PATH];


    sprintf(filename, "%s\\fragFile.bin", FalconSoundThrDirectory);

    //fragData = (char *)map_file(filename);

    //ShiAssert( fragData );
    if (fragfile.Open(filename) not_eq TRUE)
        ShiError("Can't open fragFile.bin");

    fragfile.Initialise();
}

/****************************************************************************

  DisposeFragData

 Purpose: Dispose Frag Data File

   Arguments:

 Returns:

****************************************************************************/
void VoiceFilter::DisposeFragData(void)
{
    //fragData = NULL;
    fragfile.Close();
}

// Retro 20Dec 2003 - pretty much ripped from somewhere else -
// (Voicemanager.cpp - FilterMessage()) - and put into its own routine
// used for the subtitles to determine over what readiochannel a specific message came
char VoiceFilter::CanUserHearThisMessage(const char radiofilter, const VU_ID from, const VU_ID to)
{
    char retval[2] = { 0, 0 };

    if ( not VM)
        return false;

    for (int i = 0; i < 2; i++)
    {
        switch (VM->radiofilter[i])
        {
            case rcfOff:
                retval[i] = FALSE;
                break;

            case rcfFlight5:
            case rcfFlight1:
            case rcfFlight2:
            case rcfFlight3:
            case rcfFlight4:
                if (TOFROM_FLIGHT bitand radiofilter)
                    retval[i] or_eq TOFROM_FLIGHT;

                break;

            case rcfPackage5:
            case rcfPackage1:
            case rcfPackage2:
            case rcfPackage3:
            case rcfPackage4:
                if ((TO_PACKAGE bitand radiofilter) or (radiofilter bitand TOFROM_FLIGHT))
                {
                    retval[i] or_eq TO_PACKAGE;
                }

                break;

            case rcfFromPackage:
                if ((TOFROM_PACKAGE bitand radiofilter) or (radiofilter bitand TOFROM_FLIGHT))
                    retval[i] or_eq TOFROM_PACKAGE;

                break;

            case rcfProx:
                if ((radiofilter bitand TOFROM_FLIGHT) or ((IN_PROXIMITY bitand radiofilter) and ((radiofilter bitand TO_TEAM) or (TO_PACKAGE bitand radiofilter))))
                    retval[i] or_eq IN_PROXIMITY;

                break;

            case rcfTeam:
                if ((TO_TEAM bitand radiofilter) or (radiofilter bitand TOFROM_FLIGHT) or (TOFROM_PACKAGE bitand radiofilter))
                    retval[i] or_eq TO_TEAM;

                break;

            case rcfAll:
                if ((TO_WORLD bitand radiofilter) or (radiofilter bitand TOFROM_FLIGHT) or (TOFROM_PACKAGE bitand radiofilter) or (TO_TEAM bitand radiofilter))
                    retval[i] or_eq TO_WORLD;

                break;

            case rcfTower:
                if (radiofilter bitand TOFROM_FLIGHT)
                    retval[i] or_eq TOFROM_FLIGHT;
                else if ((TOFROM_TOWER bitand radiofilter) and gNavigationSys)
                {
                    VU_ID ATCId;
                    gNavigationSys->GetAirbase(&ATCId);

                    if (ATCId == from or ATCId == to)
                        retval[i] or_eq TOFROM_TOWER;
                }

            default:
                break;
        }
    }

    char ret = (retval[0] bitor retval[1]);

    return ret;
}

/****************************************************************************

  PlayRadioMessage

 Purpose: Evalutes the talker, message and data to retrieve a conversation
 from the .tlk file and place the conversation in the circular
 buffer queue.

   Arguments: int talker - the person who is speaking;
   int msgid - the conversation to play;
   int *data - NULL is none. Used for Evals ( i.e. altitude, degrees ... )

 Returns:

****************************************************************************/
void VoiceFilter::PlayRadioMessage(
    char talker, short msgid, short *data, VU_TIME playTime,
    char radiofilter, char channel, VU_ID from, int evalby, VU_ID to
)
{
    int i;
    short evalElement;
    short fragNumber = 0, evalHdrNumber = 0;//, fileNumber;
    COMM_FILE_INFO* commHdrInfo;
    // char *dcommPtr;
    short *commInfo;
    short *dfileNum;
    CONVERSATION message;
    int eval = 0;

    // sfr: placing back original code and removing JB fucking hack which is wrong BTW, since
    // player entity can be an eject class instead of aircraft class...
    SimMoverClass *pEntity = SimDriver.GetPlayerEntity();

    if (
        // sfr: JB code commented out
        (
            pEntity /* and not F4IsBadReadPtr(SimDriver.GetPlayerEntity(), sizeof(AircraftClass))*/ and 
            pEntity->IsEject()
        ) or
        (
            VM /* and not F4IsBadReadPtr(VM, sizeof(VoiceManager))*/ and 
            VM->falconVoices[channel].exitChannel
        ) or
        killThread
    )
    {
        int player = 0, exit = 0;

        if (pEntity and pEntity->IsEject())
        {
            player = 1;
        }

        if (VM and VM->falconVoices[channel].exitChannel)
        {
            exit = 1;
        }

        //MonoPrint(
        // "Dumping Message: %d  Player: %d  ExitChan: %d KillThread: %d\n", msgid, player, exit, killThread
        //);
        return;
    }

    VM->RemoveDuplicateMessages(from, to, msgid);

    //get pointer to message data for the requested message
    if (msgid < 0)
    {
        msgid = 0;
    }
    else if (msgid >= commfile.MaxComms())
    {
        // jpo dynamic calc
        msgid = 0;
    }

    //dcommPtr = commData + ( msgid ) * sizeof(COMM_FILE_INFO);

    //commHdrInfo = (COMM_FILE_INFO *)dcommPtr;
    commHdrInfo = commfile.GetComm(msgid);
    ShiAssert(FALSE == F4IsBadReadPtr(commHdrInfo, sizeof * commHdrInfo));
    // use offset in  just aquired data to position pointer
    //dcommPtr = commData + commHdrInfo->commOffset;
    commInfo = commfile.GetCommInd(commHdrInfo);
    ShiAssert(FALSE == F4IsBadReadPtr(commInfo, sizeof * commInfo * commHdrInfo->totalElements));
    // setup message structure with appropriate values
    message.message = msgid;
    message.status = SLOT_IN_USE;
    ShiAssert(talker >= 0 and talker < fragfile.MaxVoices());

    if (talker < 0)
    {
        talker = 0;
    }
    else if (talker >= fragfile.MaxVoices())
    {
        talker = 11;
    }

    message.speaker = talker;
    message.channelIndex = channel;
    message.filter = radiofilter;
    message.from = from;
    message.to = to;
    message.sizeofConv = (char)(commHdrInfo->totalElements + 2); //need two extra spaces for pops
    message.interrupt = QUEUE_CONV;
    message.playTime = playTime; //when the message should be played
    message.priority = commHdrInfo->priority;
    message.convIndex = 0;

    //dfileNum = message.conversations = new short[commHdrInfo->totalElements];

#ifdef USE_SH_POOLS
    dfileNum = message.conversations = (short *)MemAllocPtr(
                                           gTextMemPool,  sizeof(short) * (message.sizeofConv), FALSE
                                       );
#else
    dfileNum = message.conversations = new short[message.sizeofConv];
#endif

    if (message.conversations == NULL)
    {
        return;
    }

    //add pop at beginning
    //int pop = rand() % NUM_VOICES;
    int pop = rand() % 12; //JPO - only 12 of these
    static const short RADIO_POP = 2142;
    *dfileNum = FragToFile(pop, RADIO_POP);
    dfileNum++;

    bool newMessage = true; // Retro 20Dec2003 for the RadioLabels

    for (i = 0; i < commHdrInfo->totalElements; i ++)
    {

        //fragNumber = *((short *)dcommPtr);
        fragNumber = *commInfo;

        if (fragNumber < 0)
        {
            evalHdrNumber = fragNumber;
            evalHdrNumber *= -1;

            if (data)
            {
                evalElement = data[eval];
            }
            else
            {
                evalElement = 0;
            }

            if (evalby)
            {
                eval++;

                if (evalElement == -1)
                {
                    message.sizeofConv--;
                    commInfo ++;
                    // dcommPtr += sizeof( short );
                    continue;
                }

                fragNumber = IndexElement(evalHdrNumber, evalElement);
            }
            else
            {
                switch (evalHdrNumber)
                {
                    case eBEARING:
                    case eBEARINGLAST:
                        evalElement = DegreesToElement(evalElement);
                        break;

                    case eANGELS:
                        evalElement = FeetToAngel(evalElement);
                        break;

                    case eTHOUSANDS:
                        evalElement = FeetToThousands(evalElement);
                        break;

                    case eFLIGHTSIZE:
                        evalElement--;
                        break;

                    case eMAINTAINAIRSPEED:
                        evalElement = KnotsToElement(evalElement);
                        break;

                    case eREDUCEAIRSPEED:
                    case eINCREASEAIRSPEED:
                        evalElement = KnotsToReduceIncreaseToElement(evalElement);
                        break;

                    case eRANGE:
                    case eRANGELAST:
                        evalElement = KilometersToNauticalMiles(evalElement);
                        break;

                    case eCALLNUM:
                    case eCALLNUM2:
                        /*// KCK HACK: Convert to the correct eval.
                        // This should become irrelevant once Joe gets his changes in
                        if (evalElement > VF_SHORTCALLSIGN_OFFSET)
                        {
                         evalElement -= VF_SHORTCALLSIGN_OFFSET;
                         evalHdrNumber = eSHORTCALL;
                        }
                        if (evalElement > VF_FLIGHTNUMBER_OFFSET)
                        {
                         evalElement -= VF_FLIGHTNUMBER_OFFSET;
                         evalHdrNumber = eFLIGHTNUMBER;
                        }
                        */
                        break;
                }

                if (data)
                {
                    eval++;
                }

                if (evalElement == -1 or evalElement == 32766)  // 32766 = airbase without ATC
                {
                    message.sizeofConv--;
                    commInfo ++;
                    //dcommPtr += sizeof( short );
                    continue;
                }

                fragNumber = EvaluateElement(evalHdrNumber, evalElement);
            }
        }

        // Retro 20Dec2003 start
        if ((radioLabel) and (SimDriver.InSim()))
        {
            // however there´s a prob as there are a few 'continues' up so maybe I´m missing some chunks here..
            char theChannel = CanUserHearThisMessage(radiofilter, from, to);

            if (theChannel)
            {
                if (newMessage)
                {
                    radioLabel->NewMessage(talker, fragNumber, playTime, theChannel);
                    newMessage = false;
                }
                else
                {
                    radioLabel->AddToMessage(talker, fragNumber);
                }
            }
        }

        // Retro 20Dec2003 end

        *dfileNum = FragToFile(talker, fragNumber);
        ShiAssert(*dfileNum >= 0);

        dfileNum++;
        commInfo ++;
        //dcommPtr += sizeof( short );
    }

    //add closing pop
    pop = rand() % 12; // jpo ditto
    *dfileNum = FragToFile(pop, RADIO_POP);

#ifdef _DEBUG

    for (i = 0; i < message.sizeofConv; i++)
        ShiAssert(message.conversations[i] >= 0);

#endif
#ifdef DAVE_DBG
    //MonoPrint("Add message %d to queue %d\n", message.message, message.channelIndex);
#endif
    VM->AddToConversationQueue(&message);
    SetEvent(VMWakeEventHandle);
}

/****************************************************************************

KilometersToNauticalMiles

Purpose: Convert Kilometers to nautical miles

Arguments: int feet

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::KilometersToNauticalMiles(short km)
{
    if (km == -1)
        return -1;

    int index;
    float dist;

    // 2001-09-15 M.N. MORE DISCRETE DISTANCES: FROM 60-400 NM IN 20 NM INCREMENTS
    // index = (short)FloatToInt32(KM_TO_NM * km); -> NOT NEEDED, GETS CHANGED IN ANY CASE BELOW
    dist = KM_TO_NM * km;

    // Ranges over 400 nm are in 50 nm increments
    if (dist >= 400.0F)
        index = FloatToInt32(22.0F + dist / 50.0F + 0.5F);

    // 2001-09-15 M.N.

    // Ranges over 60 nm are in 20 nm increments
    else if (dist >= 60.0F)
        index = FloatToInt32(10.0F + dist / 20.0F + 0.5F);
    // Ranges over 40 nm are in 10 nm increments
    else if (dist >= 40.0F)
        index = FloatToInt32(7.0F + dist / 10.0F + 0.5F);
    // Ranges over 5 nm are in 5 nm increments
    else if (dist >= 5.0F)
        index = FloatToInt32(3.0F + dist / 5.0F + 0.5F);
    else
        index = FloatToInt32(dist - 1.0F + 0.5F);

    if (index < ERANGE_MIN)
        index = -1;
    else if (index > ERANGE_MAX)
        index = ERANGE_MAX;

    return (short)index;
}

/****************************************************************************

FeetToAngel

Purpose: Convert feet to angles

Arguments: int feet

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::FeetToAngel(int feet)
{
    int index;

    if (feet == -1)
        return -1;

    // 1000ft. = 1 Angel
    index = feet / 1000;

    if (index > 50)
        index = 36 + index / 10;
    else if (index > 40)
        index = 31 + index / 5;
    else
        index--;

    if (index < EANGELS_MIN)
        index = -1;
    else if (index > EANGELS_MAX)
        index = EANGELS_MAX;

    return (short)index;
}

/****************************************************************************

FeetToThousands

Purpose: Convert feet to thousands

Arguments: int feet

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::FeetToThousands(int feet)
{
    if (feet == -1)
        return -1;

    int index;

    // 1000ft. = 1 Angel
    index = feet / 1000;

    if (index > 60)
    {
        index = 36 + index / 5;

    }
    else if (index > 40)
        index = 24 + 2 * (index / 5);
    else if (index > 40)
        index = 31 + index / 5;
    else
        index;

    if (index > ETHOUSANDS_MAX)
        index = ETHOUSANDS_MAX;

    if (index < ETHOUSANDS_MIN)
        index = -1;

    return (short)index;
}

/****************************************************************************

DegreesToElement

Purpose: Convert degrees to eval index.

DegreesToElement

Purpose: Convert degrees to eval index.

Arguments: int degree

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::DegreesToElement(int degree)
{
    if (degree == -1)
        return -1;

    int index;

    // Degree = 350 / 10 = 35 - index into Eval
    if (degree < 0)
        degree += 360;

    index = (degree + 2) / 5;

    if (index < EBEARING_MIN)
        index = EBEARING_MIN;
    else if (index > EBEARING_MAX)
        index = EBEARING_MAX;

    return (short)index;
}

/****************************************************************************

KnotsToElement

Purpose: Convert knots to eval index.

Arguments: int knots

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::KnotsToElement(int knots)
{
    if (knots == -1)
        return -1;

    int index;

    index = (knots - 95) / 10;

    if (index < 0)
        index = 0;

    if (index > 36)
        index = 36;

    return (short)index;
}

/****************************************************************************

KnotsToReduceIncreaseToElement

Purpose: Convert knots to eval index.

Arguments: int knots

Returns: int - eval index ( element )

****************************************************************************/
short VoiceFilter::KnotsToReduceIncreaseToElement(int knots)
{
    if (knots == -1)
        return -1;

    int index;

    index = (knots - 110) / 10;

    if (index < 0)
        index = 0;

    if (index > 35)
        index = 35;

    return (short)index;
}

/****************************************************************************

EvaluateElement

  Purpose: Find the file number of given the eval and element

 Arguments: short evalHdrNumber - eval to check;
 int evalElement - element in eval.

   Returns: short - frag file number
Rewritten to use real data structures - JPO
****************************************************************************/
short VoiceFilter::EvaluateElement(short evalHdrNumber, short evalElement)
{
    //  char *dEvalData;
    EVAL_FILE_INFO *eEvalData;
    EVAL_ELEM *eEvalElem;

    // EVAL_FILE_INFO evalHdrInfo;
    short fragHdrNbr = 0;

    //index into evaldata to get appropriate offset
    //  dEvalData = evalData + (evalHdrNumber * sizeof( EVAL_FILE_INFO ));
    ShiAssert(evalHdrNumber >= 0 and evalHdrNumber < evalfile.MaxEvals());
    eEvalData = evalfile.GetEval(evalHdrNumber);
    ShiAssert(FALSE == F4IsBadReadPtr(eEvalData, sizeof * eEvalData));

    if (eEvalData == NULL) return 0;

    //use binary search to find appropriate frag, the values
    //are in order but they are not necessarily consecutive
    //  int upper = ((EVAL_FILE_INFO *)dEvalData)->numEvals -1;
    int upper = eEvalData->numEvals - 1;
    int lower = 0;
    int center;

    // 2001-09-22 M.N. random evalIndex (only for consecutive indexes) if evalElement == 32767 (max short+)

    if (evalElement == 32767) // allows later addition of random fragments, if wanted
        evalElement = rand() % (upper + 1); // => no hardcoded limits anymore

    // END of added section

    //use offset to index into start of frags for this eval
    //  dEvalData = evalData + ((EVAL_FILE_INFO *)dEvalData)->evalOffset;
    eEvalElem = evalfile.GetEvalElem(eEvalData);
    ShiAssert(FALSE == F4IsBadReadPtr(eEvalElem, sizeof * eEvalElem));

    if (eEvalElem == NULL) return 0;

    while (upper >= lower)
    {
        center = (lower + upper) / 2;

        //   if(evalElement == (( short *)dEvalData)[center * 2])
        if (evalElement == eEvalElem[center].evalElem)
        {
            //   dEvalData += sizeof( short );
            //   fragHdrNbr = ((short *)dEvalData)[center * 2];
            fragHdrNbr = eEvalElem[center].evalFrag;
            break;
        }

        //   if(evalElement < (( short *)dEvalData)[center * 2])
        if (evalElement < eEvalElem[center].evalElem)
            upper = center - 1;
        else
            lower = center + 1;
    }

    //return the appropriate frag
    return(fragHdrNbr);
}

/****************************************************************************

IndexElement

  Purpose: Find the file number of given the eval and index

 Arguments: short evalHdrNumber - eval to check;
 int evalElement - element in eval.

   Returns: short - frag file number

****************************************************************************/
short VoiceFilter::IndexElement(short evalHdrNumber, short evalElement)
{
    if (killThread)
        return 0;

    //  char *dEvalData;
    EVAL_FILE_INFO *eEvalData;
    EVAL_ELEM *eEvalElem;

    // EVAL_FILE_INFO evalHdrInfo;
    short fragHdrNbr = 0;

    //index into evaldata to get appropriate offset
    //  dEvalData = evalData + (evalHdrNumber * sizeof( EVAL_FILE_INFO ));
    eEvalData = evalfile.GetEval(evalHdrNumber);
    ShiAssert(FALSE == F4IsBadReadPtr(eEvalData, sizeof * eEvalData));

    if (eEvalData == NULL) return  0;

    if (evalElement < 0)
        evalElement = 0;
    //  else if(evalElement >  ((EVAL_FILE_INFO *)dEvalData)->numEvals - 1)
    //   evalElement = (short)(((EVAL_FILE_INFO *)dEvalData)->numEvals - 1);
    else if (evalElement >  eEvalData->numEvals - 1)
        evalElement = eEvalData->numEvals - 1;


    //use offset to index into start of frags for this eval
    //  dEvalData = evalData + ((EVAL_FILE_INFO *)dEvalData)->evalOffset + (sizeof(short)*2 *evalElement) + sizeof( short );
    eEvalElem = evalfile.GetEvalElem(eEvalData);
    ShiAssert(FALSE == F4IsBadReadPtr(eEvalElem, sizeof * eEvalElem));

    //  fragHdrNbr = *((short *)dEvalData);
    fragHdrNbr = eEvalElem[evalElement].evalFrag;
    //return the appropriate frag
    return(fragHdrNbr);
}

/****************************************************************************

FragToFile

Purpose: Find the file number of a frag and talker

Arguments: int talker - speaker;
 short fragNumber - frag file number

Returns: short - snd file number

****************************************************************************/
short VoiceFilter::FragToFile(int talker, short fragNumber)
{
    if (killThread)
        return 0;

    SPEAKER_TO_FILE *dFragData;
    FRAG_FILE_INFO *fragHdrInfo;
    short fileNumber = 0;
    int index = 0;
    SPEAKER_TO_FILE *headerInfo = NULL;

    //get pointer into fragData where FRAG_FILE_INFO structure is stored
    //  fragHdrInfo = (FRAG_FILE_INFO *)(fragData + (sizeof(FRAG_FILE_INFO)*fragNumber));
    ShiAssert(fragNumber < fragfile.MaxFrags());

    if (fragNumber >= fragfile.MaxFrags())
        return 0;

    fragHdrInfo = fragfile.GetFragInfo(fragNumber);
    ShiAssert(FALSE == F4IsBadReadPtr(fragHdrInfo, sizeof * fragHdrInfo));

    //use offset from header to go to beginning of the data for each voice
    //  dFragData = (SPEAKER_TO_FILE*)(fragData + fragHdrInfo->fragOffset);

    dFragData = fragfile.GetSpeaker(fragHdrInfo);
    ShiAssert(FALSE == F4IsBadReadPtr(dFragData, sizeof * dFragData));
    headerInfo = dFragData;

    //since all the frags no longer have all the voices, we need to search
    //for the frag with the correct voice
    for (index = 0; index < fragHdrInfo->totalSpeakers; index++)
    {
        if (talker == headerInfo->speaker)
        {
            fileNumber = headerInfo->fileNbr;
            break;
        }
        else if (headerInfo->speaker > talker)
        {
            break;
        }

        headerInfo++;
        ShiAssert(FALSE == F4IsBadReadPtr(headerInfo, sizeof * dFragData));
    }


    //return the file number used to get correct offset into .tlk file
    return(fileNumber);
}


//returns true if position data is within proximity, false if not, or it is not
//a position based radio message
int VoiceFilter::GetBullseyeComm(int *mesgID, short *data)
{
    COMM_FILE_INFO *commHdrInfo;
    // char *dcommPtr;

    // dcommPtr = commData;

    //get pointer to message data for the requested message
    if (*mesgID < 1)
        *mesgID = 1;
    else if (*mesgID >= commfile.MaxComms()) // JPO dynamic
        *mesgID = 1;

    // dcommPtr += ( *mesgID ) * sizeof(COMM_FILE_INFO);

    // commHdrInfo = (COMM_FILE_INFO *)dcommPtr;
    commHdrInfo = commfile.GetComm(*mesgID);

    if ( not commHdrInfo or F4IsBadReadPtr(commHdrInfo, sizeof(COMM_FILE_INFO))) // JB 010331 CTD
        return FALSE;

    float dist = 0;

    if (commHdrInfo->positionElement > -1)
    {
        GridIndex x1, y1, x2, y2;
        float theta, xs1, ys1, xs2, ys2;
        x1 = data[commHdrInfo->positionElement];
        y1 = data[commHdrInfo->positionElement + 1];

        if (SimDriver.InSim())
        {
            ys1 = GridToSim(x1);
            xs1 = GridToSim(y1);

            if (FalconLocalSession->GetPlayerEntity())
            {
                xs2 = FalconLocalSession->GetPlayerEntity()->XPos();
                ys2 = FalconLocalSession->GetPlayerEntity()->YPos();
            }
            else if (FalconLocalSession->GetPlayerFlight())
            {
                xs2 = FalconLocalSession->GetPlayerFlight()->XPos();
                ys2 = FalconLocalSession->GetPlayerFlight()->YPos();
            }
            else
            {
                xs2 = FalconLocalSession->GetPlayerSquadron()->XPos();
                ys2 = FalconLocalSession->GetPlayerSquadron()->YPos();
            }

            if (PlayerOptions.BullseyeOn())
                dist = abs(Distance(xs1, ys1, xs2, ys2));
        }

        if (((commHdrInfo->bullseye > -1) and PlayerOptions.BullseyeOn()
            and dist * FT_TO_NM > 25 // JB 010121 if the dist is less than 25 miles don't use a bullseye call
            ) or not SimDriver.InSim() or
            (commHdrInfo->bullseye == *mesgID))
        {
            TheCampaign.GetBullseyeLocation(&x2, &y2);
            theta = (float)atan2((double)(x1 - x2), (double)(y1 - y2));
            data[commHdrInfo->positionElement] = (short)FloatToInt32(theta * RTD);
            data[commHdrInfo->positionElement + 1] = (short)FloatToInt32(Distance(x1, y1, x2, y2));
            *mesgID = commHdrInfo->bullseye;
        }
        else
        {
            if (FalconLocalSession->GetPlayerEntity())
            {
                y2 = SimToGrid(FalconLocalSession->GetPlayerEntity()->XPos());
                x2 = SimToGrid(FalconLocalSession->GetPlayerEntity()->YPos());
            }
            else if (FalconLocalSession->GetPlayerFlight())
            {
                y2 = SimToGrid(FalconLocalSession->GetPlayerFlight()->XPos());
                x2 = SimToGrid(FalconLocalSession->GetPlayerFlight()->YPos());
            }
            else
            {
                y2 = SimToGrid(FalconLocalSession->GetPlayerSquadron()->XPos());
                x2 = SimToGrid(FalconLocalSession->GetPlayerSquadron()->YPos());
            }

            theta = (float)atan2((double)(x1 - x2) , (double)(y1 - y2));
            data[commHdrInfo->positionElement] = (short)FloatToInt32(theta * RTD);
            data[commHdrInfo->positionElement + 1] = (short)FloatToInt32(Distance(x1, y1, x2, y2));
        }

        if (SimDriver.InSim())
        {
            /* JB 010121
             ys1 = GridToSim(x1);
             xs1 = GridToSim(y1);

             if (FalconLocalSession->GetPlayerEntity())
             {
             ys2 = FalconLocalSession->GetPlayerEntity()->XPos();
             xs2 = FalconLocalSession->GetPlayerEntity()->YPos();
             }
             else if(FalconLocalSession->GetPlayerFlight())
             {
             ys2 = FalconLocalSession->GetPlayerFlight()->XPos();
             xs2 = FalconLocalSession->GetPlayerFlight()->YPos();
             }
             else
             {
             ys2 = FalconLocalSession->GetPlayerSquadron()->XPos();
             xs2 = FalconLocalSession->GetPlayerSquadron()->YPos();
             }
            JB 010121 */
            if (DistSqu(xs1, ys1, xs2, ys2) < RADIO_PROX_RANGE * RADIO_PROX_RANGE)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}


int VoiceFilter::GetWarp(int mesgID)
{
    return commfile.GetWarp(mesgID);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void InitDialogData(void)
{
    int n = voiceFilter->evalfile.MaxEvals() * voiceFilter->fragfile.MaxVoices();
    evalLastFrag = new short[n];
    memset(evalLastFrag, FALSE, n * sizeof(short));
    n = voiceFilter->fragfile.MaxVoices() * voiceFilter->fragfile.MaxFrags();
    fragsPlayed = new char[n];
    memset(fragsPlayed, FALSE, n * sizeof(char));
}

void SetupNewMsg(void)
{
    COMM_FILE_INFO* commHdrInfo;
    // char *dcommPtr;
    short *commInfo;
    short evalHdrNumber = 0;
    // char *dEvalData;
    EVAL_FILE_INFO *eEvalData;
    int evalNum = 0;

    int i = 0;

    for (i = 0; i < 15; i++)
    {
        VToolMsgData.maxs[i] = 0;
    }

    commHdrInfo = voiceFilter->commfile.GetComm(VToolMsgData.message);

    // use offset in  just aquired data to position pointer
    // dcommPtr = voiceFilter->commData + commHdrInfo->commOffset;
    commInfo = voiceFilter->commfile.GetCommInd(commHdrInfo);

    for (i = 0; i < commHdrInfo->totalElements; i ++)
    {
        evalHdrNumber = *commInfo;

        if (evalHdrNumber < 0)
        {
            evalHdrNumber *= -1;
            eEvalData = voiceFilter->evalfile.GetEval(evalHdrNumber);
            VToolMsgData.maxs[evalNum] = eEvalData->numEvals - 1;

            if (VToolMsgData.data[evalNum] > VToolMsgData.maxs[evalNum])
                VToolMsgData.data[evalNum] = VToolMsgData.maxs[evalNum];

            VToolMsgData.eval[evalNum] = evalHdrNumber;
            VToolMsgData.element[evalNum] = (short)i;
            evalNum++;
        }

        commInfo ++;
    }

    for (i = commHdrInfo->totalElements; i < 15; i++)
    {
        VToolMsgData.data[i] = 0;
    }
}

double CalcCombinations(void)
{
    double sum = 0;
    double comm = 1;

    for (int i = 0; i < voiceFilter->commfile.MaxComms(); i++)
    {
        comm = 1;
        VToolMsgData.message = i;
        SetupNewMsg();
        int j = 0;

        while (VToolMsgData.maxs[j])
        {
            comm *= VToolMsgData.maxs[j] + 1;
            j++;
        }

        sum += comm;
    }

    return sum;
}

void IncDecMsgToPlay(int delta)
{

    VToolMsgData.message += delta;

    if (VToolMsgData.message < 0)
        VToolMsgData.message = 0;
    else if (VToolMsgData.message >= voiceFilter->commfile.MaxComms())
        VToolMsgData.message = voiceFilter->commfile.MaxComms() - 1;

    for (int i = 0; i < 15; i++)
    {
        VToolMsgData.data[i] = 0;
    }

    SetupNewMsg();

}

#pragma warning (disable : 4244)
void IncDecIndex(int index, int delta)
{
    int i = 0;

    for (i = 0; i < 15; i++)
    {
        if (VToolMsgData.element[i] == index)
            break;
    }

    if (delta > 0)
    {
        if (VToolMsgData.data[i] < VToolMsgData.maxs[i])
        {
            VToolMsgData.data[i] += delta;

            if (VToolMsgData.data[i] >= evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker])
                evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker] = VToolMsgData.data[i];
        }
    }
    else if (VToolMsgData.data[i] > -1)
    {
        VToolMsgData.data[i] += delta;
    }
}

void IncDecDataToPlay(int delta)
{
    if (VToolMsgData.mode == PLAY_MESSAGE)
    {
        int wasInc = FALSE;

        for (int i = 0; i < 15; i++)
        {
            if (delta > 0)
            {
                if (VToolMsgData.maxs[i] > evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker])
                {
                    int fragNumber = 0;
                    VToolMsgData.data[i] += delta;
                    fragNumber = voiceFilter->IndexElement(VToolMsgData.eval[i], VToolMsgData.data[i]);

                    while (fragsPlayed[fragNumber + (VToolMsgData.talker * voiceFilter->fragfile.MaxFrags())] and (VToolMsgData.data[i] < VToolMsgData.maxs[i]))
                    {
                        evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker] = VToolMsgData.data[i];
                        VToolMsgData.data[i] += delta;
                        fragNumber = voiceFilter->IndexElement(VToolMsgData.eval[i], VToolMsgData.data[i]);
                    }

                    if (VToolMsgData.data[i] == evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker] + 1)
                        evalLastFrag[VToolMsgData.eval[i] * voiceFilter->fragfile.MaxVoices() + VToolMsgData.talker] = VToolMsgData.data[i];

                    wasInc = TRUE;

                    if (VToolMsgData.data[i] > VToolMsgData.maxs[i])
                        VToolMsgData.data[i] = VToolMsgData.maxs[i];
                }
                else
                {
                    srand((unsigned)time(NULL));

                    if (VToolMsgData.maxs[i])
                        VToolMsgData.data[i] = (short)(rand() % VToolMsgData.maxs[i]);
                }
            }
            else if (VToolMsgData.data[i] > 0)
            {
                VToolMsgData.data[i] += delta;
                wasInc = TRUE;
            }
        }

        if ( not wasInc)
            IncDecMsgToPlay(delta);
    }
    else if (VToolMsgData.mode == PLAY_FRAG)
    {
        VToolMsgData.frag += delta;

        if (VToolMsgData.frag < 0)
            VToolMsgData.frag = 0;
        else if (VToolMsgData.frag >= voiceFilter->fragfile.MaxFrags())
            VToolMsgData.frag = voiceFilter->fragfile.MaxFrags();
    }
}

void IncDecFragToPlay(int delta)
{
    VToolMsgData.frag += delta;

    if (VToolMsgData.frag < 0)
        VToolMsgData.frag = 0;
    else if (VToolMsgData.frag >= voiceFilter->fragfile.MaxFrags())
        VToolMsgData.frag = voiceFilter->fragfile.MaxFrags() - 1;
}

void IncDecTalkerToPlay(int delta)
{

    VToolMsgData.talker += delta;

    if (VToolMsgData.talker < 0)
        VToolMsgData.talker = 0;
    else if (VToolMsgData.talker > 11)
        VToolMsgData.talker = 11;

}

void PlayRandomMessage(int channel)
{

    VToolMsgData.message = rand() % voiceFilter->commfile.MaxComms();

    SetupNewMsg();

    for (int i = 0; i < 15; i++)
    {
        if (VToolMsgData.maxs[i])
            VToolMsgData.data[i] = (short)(rand() % VToolMsgData.maxs[i]);
    }

    if (VToolMsgData.message <= 286)
        VToolMsgData.talker = rand() % 12;
    else
        VToolMsgData.talker = rand() % 2 + 12;

    if (voiceFilter)
        voiceFilter->PlayRadioMessage((char)VToolMsgData.talker, (short)VToolMsgData.message, VToolMsgData.data, vuxGameTime, -1, (char)channel, vuNullId, EVAL_BY_INDEX);

    SetEvent(VMWakeEventHandle);
}

int PlayToolMessage(HWND hwnd)
{
    char buffer[MAX_PATH];

    if (VToolMsgData.message < voiceFilter->commfile.MaxComms())
    {
        if (VToolMsgData.mode == PLAY_MESSAGE)
        {
            if (voiceFilter)
                voiceFilter->PlayRadioMessage((char)VToolMsgData.talker, (short)VToolMsgData.message, VToolMsgData.data, vuxGameTime, -1, 0, vuNullId, EVAL_BY_INDEX);

            SetEvent(VMWakeEventHandle);
            PostMessage(FalconDisplay.appWin, FM_GIVE_FOCUS, NULL, NULL);
            SetEvent(VMWakeEventHandle);

            for (int i = IDC_DATA0; i < IDC_DATA0 + 15; i++)
            {
                int fragNumber = 0;
                GetWindowText(GetDlgItem(hwnd, i), (LPTSTR) buffer, MAX_PATH);

                fragNumber = atoi(buffer);

                if (fragNumber >= 0)
                    fragsPlayed[fragNumber + (VToolMsgData.talker * voiceFilter->fragfile.MaxFrags())] = TRUE;

            }

            return TRUE;
        }
        else if (VToolMsgData.mode == PLAY_FRAG)
        {
            CONVERSATION message;

            message.status = SLOT_IN_USE;
            message.speaker = (char)VToolMsgData.talker;
            message.channelIndex = 0;
            message.filter = -1;
            message.sizeofConv = 1;
            message.interrupt = QUEUE_CONV;
            message.playTime = vuxGameTime; //when the message should be played
            message.priority = 1; //NORMAL_PRIORITY;
            message.convIndex = 0;

#ifdef USE_SH_POOLS
            message.conversations = (short *)MemAllocPtr(gTextMemPool,  sizeof(short), FALSE);
#else
            message.conversations = new short[1];
#endif

            message.conversations[0] = voiceFilter->FragToFile(VToolMsgData.talker, (short)VToolMsgData.frag);

            VM->AddToConversationQueue(&message);

            SetEvent(VMWakeEventHandle);
            PostMessage(FalconDisplay.appWin, FM_GIVE_FOCUS, NULL, NULL);
            SetEvent(VMWakeEventHandle);
            return TRUE;
        }
    }

    return FALSE;
}

void UpdateVoiceDialog(HWND hwnd)
{
    char buffer[MAX_PATH];
    int i = 0;
    COMM_FILE_INFO* commHdrInfo;
    //char *dcommPtr;
    short *commInfo;
    short fragNumber = 0, evalHdrNumber = 0;
    int evalNum = 0;

    _itoa(VToolMsgData.talker, buffer, 10);
    Edit_SetText(GetDlgItem(hwnd, IDC_VOICE), buffer);
    _itoa(VToolMsgData.message, buffer, 10);
    Edit_SetText(GetDlgItem(hwnd, IDC_MESSAGE), buffer);
    _itoa(VToolMsgData.frag, buffer, 10);
    Edit_SetText(GetDlgItem(hwnd, IDC_FRAG), buffer);

    CheckDlgButton(hwnd, IDC_PLAY_MESSAGE, (VToolMsgData.mode == PLAY_MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_PLAY_FRAG, (VToolMsgData.mode == PLAY_FRAG) ? BST_CHECKED : BST_UNCHECKED);

    commHdrInfo = voiceFilter->commfile.GetComm(0);

    if (VToolMsgData.message < voiceFilter->commfile.MaxComms())
    {
        commHdrInfo = voiceFilter->commfile.GetComm(VToolMsgData.message);

        // use offset in  just aquired data to position pointer
        //dcommPtr = voiceFilter->commData + commHdrInfo->commOffset;
        commInfo = voiceFilter->commfile.GetCommInd(commHdrInfo);

        for (i = 0; i < commHdrInfo->totalElements; i ++)
        {
            fragNumber = 0;
            evalHdrNumber = *commInfo;

            if (evalHdrNumber < 0)
            {
                evalHdrNumber *= -1;

                fragNumber = voiceFilter->IndexElement(evalHdrNumber, VToolMsgData.data[evalNum]);

                VToolMsgData.eval[evalNum] = evalHdrNumber;
                _itoa(evalHdrNumber, buffer, 10);
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA15 + i), buffer);
                _itoa(VToolMsgData.maxs[evalNum], buffer, 10);
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA30 + i), buffer);
                _itoa(VToolMsgData.data[evalNum], buffer, 10);
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA45 + i), buffer);
                evalNum++;
            }
            else
            {
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA15 + i), "");
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA30 + i), "");
                Edit_SetText(GetDlgItem(hwnd, IDC_DATA45 + i), "");
                fragNumber = evalHdrNumber;
            }

            _itoa(fragNumber, buffer, 10);
            Edit_SetText(GetDlgItem(hwnd, IDC_DATA0 + i), buffer);
            commInfo ++;
        }
    }

    for (i = commHdrInfo->totalElements; i < 15; i++)
    {
        VToolMsgData.data[i] = 0;
        Edit_SetText(GetDlgItem(hwnd, IDC_DATA0 + i), "");
        Edit_SetText(GetDlgItem(hwnd, IDC_DATA15 + i), "");
        Edit_SetText(GetDlgItem(hwnd, IDC_DATA30 + i), "");
        Edit_SetText(GetDlgItem(hwnd, IDC_DATA45 + i), "");
    }
}


void GetDialogValues(HWND hwnd)
{
    char buffer[MAX_PATH];

    GetWindowText(GetDlgItem(hwnd, IDC_VOICE), (LPTSTR) buffer, MAX_PATH);
    VToolMsgData.talker = atoi(buffer);
    GetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), (LPTSTR) buffer, MAX_PATH);

    if (atoi(buffer) not_eq VToolMsgData.message)
    {
        VToolMsgData.message = atoi(buffer);
        SetupNewMsg();
    }

    GetWindowText(GetDlgItem(hwnd, IDC_FRAG), (LPTSTR) buffer, MAX_PATH);
    VToolMsgData.frag = atoi(buffer);

    if (IsDlgButtonChecked(hwnd, IDC_PLAY_MESSAGE))
        VToolMsgData.mode = PLAY_MESSAGE;
    else if (IsDlgButtonChecked(hwnd, IDC_PLAY_FRAG))
        VToolMsgData.mode = PLAY_FRAG;
    else
        VToolMsgData.mode = PLAY_NOTHING;

    for (int i = IDC_DATA45; i < IDC_DATA45 + 15; i++)
    {
        int j;

        for (j = 0; j < 15; j++)
        {
            if (VToolMsgData.element[j] + IDC_DATA45 == i)
                break;
        }

        if (j == 15)
            continue;

        GetWindowText(GetDlgItem(hwnd, i), (LPTSTR) buffer, MAX_PATH);

        if (atoi(buffer) < -1)
            VToolMsgData.data[j] = -1;
        else if (atoi(buffer) > VToolMsgData.maxs[j])
            VToolMsgData.data[j] = VToolMsgData.maxs[j];
        else
            VToolMsgData.data[j] = (short)atoi(buffer);
    }
}

LRESULT CALLBACK PlayVoicesProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buffer[MAX_PATH];
    char *pBuf;
    int index = 0;
    FILE *fp;
    double result;
    //long *lptr;
    int dec, sign;


    switch (message)
    {
        case WM_INITDIALOG:
            InitDialogData();
            SetupNewMsg();
            UpdateVoiceDialog(hwnd);
            ShowWindow(hwnd, SW_SHOW);
            return DefWindowProc(hwnd, message, wParam, lParam);
            //DialogBox(hInst,MAKEINTRESOURCE(IDD_PLAYVOICES),hwnd,(DLGPROC)PlayVoicesProc);
            break;

        case WM_COMMAND:                 /* message: received command */
            switch (LOWORD(wParam))
            {
                case IDOK:    /* "OK" box selected. */
                case IDCANCEL:
                    EndDialog(hwnd, TRUE);     /* Exits the dialog box       */
                    return (TRUE);
                    break;

                case IDC_PLAY:
                    GetDialogValues(hwnd);
                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_UPDATE:
                    GetDialogValues(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_CLOSE:
                    GetDialogValues(hwnd);
                    EndDialog(hwnd, TRUE);
                    break;

                case IDC_INC_MESSAGE:
                    GetDialogValues(hwnd);
                    IncDecMsgToPlay(1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_DEC_MESSAGE:
                    GetDialogValues(hwnd);
                    IncDecMsgToPlay(-1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_INC_VOICE:
                    GetDialogValues(hwnd);
                    IncDecTalkerToPlay(1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_DEC_VOICE:
                    GetDialogValues(hwnd);
                    IncDecTalkerToPlay(-1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_INC_FRAG:
                    GetDialogValues(hwnd);
                    IncDecFragToPlay(1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_DEC_FRAG:
                    GetDialogValues(hwnd);
                    IncDecFragToPlay(-1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_INC_ALLINDEXES:
                    GetDialogValues(hwnd);
                    IncDecDataToPlay(1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_DEC_ALLINDEXES:
                    GetDialogValues(hwnd);
                    IncDecDataToPlay(-1);

                    PlayToolMessage(hwnd);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_RESET:
                    memset(evalLastFrag, 0, sizeof(short)*voiceFilter->evalfile.MaxEvals()*voiceFilter->fragfile.MaxVoices());
                    memset(fragsPlayed, 0, sizeof(char)*voiceFilter->fragfile.MaxFrags()*voiceFilter->fragfile.MaxVoices());
                    break;

                case IDC_SAVE:
                    GetDialogValues(hwnd);
                    sprintf(buffer, "%s\\VoiceTool.sav", FalconDataDirectory);
                    fp = fopen(buffer, "wb");

                    if (fp)
                    {
                        fwrite(&VToolMsgData, sizeof(VoiceToolData), 1, fp);
                        fwrite(evalLastFrag, sizeof(short), voiceFilter->evalfile.MaxEvals()*voiceFilter->fragfile.MaxVoices(), fp);
                        fwrite(fragsPlayed, sizeof(char), voiceFilter->fragfile.MaxFrags()*voiceFilter->fragfile.MaxVoices(), fp);
                        fclose(fp);
                    }

                    break;

                case IDC_LOAD:
                    sprintf(buffer, "%s\\VoiceTool.sav", FalconDataDirectory);
                    fp = fopen(buffer, "rb");

                    if (fp)
                    {
                        fread(&VToolMsgData, sizeof(VoiceToolData), 1, fp);
                        fread(evalLastFrag, sizeof(short), voiceFilter->evalfile.MaxEvals()*voiceFilter->fragfile.MaxVoices(), fp);
                        fread(fragsPlayed, sizeof(char), voiceFilter->fragfile.MaxFrags()*voiceFilter->fragfile.MaxVoices(), fp);
                        fclose(fp);
                        UpdateVoiceDialog(hwnd);
                    }

                    break;

                case IDC_COMBOS:
                    result = CalcCombinations();
                    pBuf = _fcvt(result, 0, &dec, &sign);
                    Edit_SetText(GetDlgItem(hwnd, IDC_COMBINATIONS), pBuf);
                    break;

                case IDC_DEC_INDEX1:
                case IDC_DEC_INDEX2:
                case IDC_DEC_INDEX3:
                case IDC_DEC_INDEX4:
                case IDC_DEC_INDEX5:
                case IDC_DEC_INDEX6:
                case IDC_DEC_INDEX7:
                case IDC_DEC_INDEX8:
                case IDC_DEC_INDEX9:
                case IDC_DEC_INDEX10:
                case IDC_DEC_INDEX11:
                case IDC_DEC_INDEX12:
                case IDC_DEC_INDEX13:
                case IDC_DEC_INDEX14:
                case IDC_DEC_INDEX15:
                    index = (LOWORD(wParam) - IDC_DEC_INDEX1) / 2;
                    IncDecIndex(index, -1);
                    UpdateVoiceDialog(hwnd);
                    break;

                case IDC_INC_INDEX1:
                case IDC_INC_INDEX2:
                case IDC_INC_INDEX3:
                case IDC_INC_INDEX4:
                case IDC_INC_INDEX5:
                case IDC_INC_INDEX6:
                case IDC_INC_INDEX7:
                case IDC_INC_INDEX8:
                case IDC_INC_INDEX9:
                case IDC_INC_INDEX10:
                case IDC_INC_INDEX11:
                case IDC_INC_INDEX12:
                case IDC_INC_INDEX13:
                case IDC_INC_INDEX14:
                case IDC_INC_INDEX15:
                    index = (LOWORD(wParam) - IDC_INC_INDEX1) / 2;
                    IncDecIndex(index, 1);
                    UpdateVoiceDialog(hwnd);
                    break;

                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
            }

            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return TRUE;
}
#pragma warning (default : 4244)
