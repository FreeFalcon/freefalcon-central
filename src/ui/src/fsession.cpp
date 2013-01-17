#include <windows.h>
#include "resource.h"
#include "f4vu.h"
#include "sim\include\stdhdr.h"
#include "Falclib\Include\ui.h"

static int F4SessionManagerOn = FALSE;

#define SESSION_NAME_SIZE  20
#define DEFAULT_GROUP_NAME     "Default Game"
#define NEW_BOX_NAME       1
#define NEW_BOX_GROUP      2

BOOL CreateNewGame (HWND hDlg);
BOOL JoinGameProc (HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

void F4SessionUpdateGameList (HWND hDlg);
void F4SessionUpdateMemberList(HWND hDlg);
void F4SessionUpdateMemberList (HWND hDlg);
//void SynchronizeTime(VuSessionEntity *session, VU_TIME tstamp);
//void SynchronizeTime(VuGameEntity *game);

static char F4SessionMemberName[SESSION_NAME_SIZE] = "";
static char F4SessionGameName[SESSION_NAME_SIZE] = "";
static char dialogReturnString[SESSION_NAME_SIZE] = "";
static int  NewBoxType;

int InitSessionStuff (HWND hDlg);
void EndSessionStuff (void);
BOOL SessionNewProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern HINSTANCE hInst;
extern int F4GameType;

BOOL CreateNewGame (HWND hDlg)
{
   InitSessionStuff(hDlg);
   NewBoxType = NEW_BOX_GROUP;
   DialogBox(hInst,MAKEINTRESOURCE(IDD_SESSION_NEW),hDlg,(DLGPROC)SessionNewProc);
   if (dialogReturnString[0])
   {
   VuGameEntity* newGame;
   
      newGame = new VuGameEntity (vuLocalSessionEntity->Domain(), dialogReturnString);
      vuDatabase->Insert (newGame);
   }

   if (hDlg)
  	{
   	F4SessionUpdateGameList(hDlg);
	}
	return TRUE;
}

#if 0
BOOL JoinGameProc (HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
BOOL retval = FALSE;
int selItem, rc;
char tmpName[SESSION_NAME_SIZE];
VuGameEntity* myGame;

   switch (message)
   	{
      case WM_INITDIALOG:
         InitSessionStuff(hDlg);
         F4SessionUpdateGameList(hDlg);
         F4SessionUpdateMemberList(hDlg);
			SetTimer(hDlg,1,500,NULL);
     	break;

		case WM_TIMER:
         F4SessionUpdateGameList(hDlg);
         F4SessionUpdateMemberList(hDlg);
		break;

      case WM_COMMAND:                		/* message: received a command */
         switch (LOWORD(wParam))
         	{
            case IDOK:
            case IDCANCEL:
               EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
               retval = TRUE;
            break;

            case IDC_SESSION_GROUP_LIST:
               switch (HIWORD(wParam))
               	{
                  case LBN_SELCHANGE:
                  case LBN_DBLCLK:
                     selItem = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETCURSEL, 0, 0);
                     SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETTEXT,
                        selItem, (LPARAM)tmpName);
                     myGame = (VuGameEntity*)vuDatabase->Find (
                     *((VU_ID*)SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETITEMDATA,
                        selItem, (LPARAM)0)));


                     if (HIWORD(wParam) == LBN_DBLCLK && myGame)
                     {
                        gMainThread->LeaveGame();
                        rc = gMainThread->JoinGame(myGame);

                        if(rc >= 0)
                        {
                        VuDatabaseIterator dbiter;
                        VuEntity *ent = dbiter.GetFirst();
                           while (ent)
                           {
                              if (!ent->IsPrivate() && !ent->IsGlobal())
                              {
                                 VuMessage *msg = new VuReleaseEvent(ent);
                                 VuMessageQueue::PostVuMessage(msg);
                              }
                              ent = dbiter.GetNext();
                           }
                        }

//                      SynchronizeTime(myGame);
						// KCK NOTE: We really don't want to do this here
//                      VuMessage *req = new VuGetRequest(vuNullId); // gets all game ents
//                      VuMessageQueue::PostVuMessage(req);
//                      req = new VuGetRequest();	// gets all game data
//                      VuMessageQueue::PostVuMessage(req);
                        InitSessionStuff(hDlg);
                     }
                     F4SessionUpdateMemberList(hDlg);
                  break;
               	}
               retval = TRUE;
            break;
         	}
      break;
   	}
   lParam = wParam;
   return (retval);
	}
#endif

BOOL SessionManagerProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
BOOL retval = FALSE;
int selItem;
char tmpName[SESSION_NAME_SIZE];
VuGameEntity* myGame;
static int theTimer;

   switch (message)
   {
      case WM_INITDIALOG:
         SetDlgItemText(hDlg, IDC_SESSION_NAME, F4SessionMemberName);
         SetDlgItemText(hDlg, IDC_SESSION_GROUP, F4SessionGameName);
         CheckDlgButton (hDlg, IDC_SESSION_ENABLE, F4SessionManagerOn ? BST_CHECKED : BST_UNCHECKED);
	      F4SessionManagerOn = TRUE;
			theTimer = SetTimer(hDlg,1,500,NULL);
     	break;

		case WM_TIMER:
         F4SessionUpdateGameList(hDlg);
         F4SessionUpdateMemberList(hDlg);
      break;

      case WM_COMMAND:                		/* message: received a command */
         switch (LOWORD(wParam))
         {
            case IDOK:
            case IDCANCEL:
               EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
               retval = TRUE;
               KillTimer (hDlg, theTimer);
            break;

            case IDC_SESSION_ENABLE:
               if (IsDlgButtonChecked(hDlg, IDC_SESSION_ENABLE) == BST_CHECKED)
   					{
                  InitSessionStuff(hDlg);
                  F4SessionUpdateGameList(hDlg);
                  F4SessionUpdateMemberList(hDlg);
			         SetDlgItemText(hDlg, IDC_SESSION_NAME, F4SessionMemberName);
         			SetDlgItemText(hDlg, IDC_SESSION_GROUP, F4SessionGameName);
						}
               else
                  EndSessionStuff();
               retval = TRUE;
            break;

            case IDC_SESSION_CREATE_GROUP:
               retval = CreateNewGame(hDlg);
            break;

            case IDC_SESSION_CHANGE_NAME:
               NewBoxType = NEW_BOX_NAME;
               DialogBox(hInst,MAKEINTRESOURCE(IDD_SESSION_NEW),hDlg,(DLGPROC)SessionNewProc);
               if (dialogReturnString[0])
               {
                  vuLocalSessionEntity->SetPlayerCallsign(dialogReturnString);
                  InitSessionStuff(hDlg);
               }
               SetDlgItemText(hDlg, IDC_SESSION_NAME, F4SessionMemberName);
               F4SessionUpdateMemberList(hDlg);
               retval = TRUE;
            break;

            case IDC_SESSION_GROUP_LIST:
               switch (HIWORD(wParam))
               {
                  case LBN_SELCHANGE:
                  case LBN_DBLCLK:
                     selItem = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETCURSEL, 0, 0);
                     SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETTEXT,
                        selItem, (LPARAM)tmpName);
                     myGame = (VuGameEntity*)vuDatabase->Find (
                     *((VU_ID*)SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETITEMDATA,
                        selItem, 0)));

                     if ((HIWORD(wParam) == LBN_DBLCLK) && myGame)
                     {
                        gMainThread->LeaveGame();
                        gMainThread->JoinGame(myGame);
                        InitSessionStuff(hDlg);
                     }
                     F4SessionUpdateMemberList(hDlg);
                  break;
               }
               retval = TRUE;
            break;

            case IDC_SESSION_EXIT:
               if (vuLocalSessionEntity->Game() != vuPlayerPoolGroup)
                  gMainThread->LeaveGame();
               InitSessionStuff(hDlg);
               F4SessionUpdateGameList(hDlg);
               F4SessionUpdateMemberList(hDlg);
            break;
         }
      break;
   }

   lParam = wParam;
   return (retval);
}

int InitSessionStuff (HWND hDlg)
{
int retval = FALSE;

   sprintf (F4SessionMemberName, "%s\0", vuLocalSessionEntity->Callsign());
   vuLocalSessionEntity->SetPlayerCallsign(F4SessionMemberName);

   if (vuLocalSessionEntity->Game())
      sprintf (F4SessionGameName, "%s\0", vuLocalSessionEntity->Game()->GameName());
   else
      sprintf (F4SessionGameName, "%s\0", vuPlayerPoolGroup->GameName());

   retval = TRUE;
   SetDlgItemText(hDlg, IDC_SESSION_NAME, F4SessionMemberName);
   SetDlgItemText(hDlg, IDC_SESSION_GROUP, F4SessionGameName);

   return (retval);
}

void EndSessionStuff (void)
{
}

void F4SessionUpdateGameList (HWND hDlg)
{
int retval, selItem;
VuGameEntity* curGame;
VuDatabaseIterator gameWalker;
char tmpName[SESSION_NAME_SIZE];
VU_ID* tmpId;

   if (hDlg == NULL)
      return;

   selItem = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETCURSEL, 0, 0);
   SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETTEXT, selItem, (LPARAM)tmpName);

   // Free Pointers
   retval = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETCOUNT, 0, 0);
   for (selItem = 0; selItem < retval; selItem++)
   {
      delete ((VU_ID*)SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETITEMDATA,
                        selItem, 0));
   }
   SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_RESETCONTENT, 0, 0);

   curGame = (VuGameEntity*)gameWalker.GetFirst();
   while (curGame)
   {
      if (curGame->Type() == F4GameType+VU_LAST_ENTITY_TYPE)
      {
         tmpId = new VU_ID;
         *tmpId = curGame->Id();

         retval = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_ADDSTRING,
            0, (LPARAM)curGame->GameName());
         SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_SETITEMDATA,
            retval, (LPARAM)tmpId);
      }
      curGame = (VuGameEntity*)gameWalker.GetNext();
   }

   SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_SELECTSTRING,
      (WPARAM)-1, (LPARAM)tmpName);
}

void F4SessionUpdateMemberList (HWND hDlg)
{
int retval, selItem;
VuSessionEntity* curSession;
VuGameEntity* myGame;
VU_ID* tmpId;

   if (hDlg == NULL)
      return;

   // Get the selected game
   selItem = SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETCURSEL, 0, 0);

   if (selItem >= 0)
   {
      myGame = (VuGameEntity*)vuDatabase->Find (
         *((VU_ID*)SendDlgItemMessage (hDlg, IDC_SESSION_GROUP_LIST, LB_GETITEMDATA, selItem, 0)));
      VuSessionsIterator sessionWalker(myGame);

      // Free Pointers
      retval = SendDlgItemMessage (hDlg, IDC_SESSION_MEMBER_LIST, LB_GETCOUNT, 0, 0);
      for (selItem = 0; selItem < retval; selItem++)
      {
         delete ((VU_ID*)SendDlgItemMessage (hDlg, IDC_SESSION_MEMBER_LIST, LB_GETITEMDATA,
                           selItem, 0));
      }
      SendDlgItemMessage (hDlg, IDC_SESSION_MEMBER_LIST, LB_RESETCONTENT, 0, 0);

      curSession = (VuSessionEntity*)sessionWalker.GetFirst();
      while (curSession)
      {
         tmpId = new VU_ID;
         *tmpId = curSession->Id();

         retval = SendDlgItemMessage (hDlg, IDC_SESSION_MEMBER_LIST, LB_ADDSTRING,
            0, (LPARAM)curSession->Callsign());
         SendDlgItemMessage (hDlg, IDC_SESSION_MEMBER_LIST, LB_SETITEMDATA,
            retval, (LPARAM)tmpId);
         curSession = (VuSessionEntity*)sessionWalker.GetNext();
      }
   }
}

BOOL SessionNewProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
BOOL retval = FALSE;

   switch (message)
   {
      case WM_INITDIALOG:
         if (NewBoxType == NEW_BOX_GROUP)
         {
            SetDlgItemText(hDlg, IDC_SESSION_NEW_NAME, vuLocalSessionEntity->Game()->GameName());
            SetDlgItemText (hDlg, IDC_SESSION_NEW_PROMPT, "New Game :");
         }
         else if (NewBoxType == NEW_BOX_NAME)
         {
            SetDlgItemText(hDlg, IDC_SESSION_NEW_NAME, vuLocalSessionEntity->Callsign());
            SetDlgItemText (hDlg, IDC_SESSION_NEW_PROMPT, "New Name :");
         }
      break;

      case WM_COMMAND:                		/* message: received a command */
         switch (LOWORD(wParam))
         {
            case IDOK:   						/* "OK" box selected?        */
               GetDlgItemText (hDlg, IDC_SESSION_NEW_NAME, dialogReturnString, SESSION_NAME_SIZE);
            case IDCANCEL:
               EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
               retval = TRUE;
            break;
         }
      break;
   }
   return (retval);
}
