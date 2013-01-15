#include <windows.h>
#include <commdlg.h>    // includes common dialog functionality
#include <commctrl.h>
#include <dlgs.h>       // includes common dialog template defines
#include "resource.h"
#include "f4vu.h"
#include "sim\include\stdhdr.h"
#include "PlayerOp.h"

void InitSimOptionStuff (HWND hDlg)
{
   CheckDlgButton (hDlg, IDC_SIMOPTION_AUTOTARGET, PlayerOptions.AutoTargetingOn() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_BLACKOUT,   PlayerOptions.BlackoutOn() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_FUEL,       PlayerOptions.UnlimitedFuel() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_AMMO,       PlayerOptions.UnlimitedAmmo() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_CHAFF,      PlayerOptions.UnlimitedChaff() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_COLLISION,  PlayerOptions.CollisionsOn() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton (hDlg, IDC_SIMOPTION_NAMETAGS,   PlayerOptions.NameTagsOn() ? BST_CHECKED : BST_UNCHECKED);

   // Flight Model
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FLIGHTMODEL, CB_ADDSTRING,
   0, (LPARAM)"Simple");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FLIGHTMODEL, CB_ADDSTRING,
   0, (LPARAM)"Moderate");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FLIGHTMODEL, CB_ADDSTRING,
   0, (LPARAM)"Acurate");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FLIGHTMODEL, CB_SETCURSEL,
      PlayerOptions.GetFlightModelType(), 0);

   // Weapon Model
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_WEAPONEFFECT, CB_ADDSTRING,
   0, (LPARAM)"Exaggerated");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_WEAPONEFFECT, CB_ADDSTRING,
   0, (LPARAM)"Enhanced");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_WEAPONEFFECT, CB_ADDSTRING,
   0, (LPARAM)"Acurate");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_WEAPONEFFECT, CB_SETCURSEL,
      PlayerOptions.GetWeaponEffectiveness(), 0);

   // Radar Model
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_RADARTYPE, CB_ADDSTRING,
   0, (LPARAM)"360");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_RADARTYPE, CB_ADDSTRING,
   0, (LPARAM)"Easy");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_RADARTYPE, CB_ADDSTRING,
   0, (LPARAM)"APG 68");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_RADARTYPE, CB_SETCURSEL,
      PlayerOptions.GetAvionicsType(), 0);

   // Autopilot
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_AUTOPILOT, CB_ADDSTRING,
   0, (LPARAM)"Intelligent");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_AUTOPILOT, CB_ADDSTRING,
   0, (LPARAM)"Enhanced");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_AUTOPILOT, CB_ADDSTRING,
   0, (LPARAM)"Normal");
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_AUTOPILOT, CB_SETCURSEL,
      PlayerOptions.GetAutopilotMode(), 0);

   // Detail levels
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FEAT_DETAIL, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0,4));
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_VEH_DETAIL, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0,15));
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_MAGNIFICATION, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0,9));
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_FEAT_DETAIL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)PlayerOptions.BldDeaggLevel);
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_VEH_DETAIL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)PlayerOptions.ObjDeaggLevel);
   SendDlgItemMessage (hDlg, IDC_SIMOPTION_MAGNIFICATION, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)PlayerOptions.ObjMagnification);
}

void UpdateSimOptions (HWND hDlg)
{
   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_AUTOTARGET))
      PlayerOptions.SetSimFlag (SIM_AUTO_TARGET);
   else
      PlayerOptions.ClearSimFlag (SIM_AUTO_TARGET);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_BLACKOUT))
      PlayerOptions.ClearSimFlag (SIM_NO_BLACKOUT);
   else
      PlayerOptions.SetSimFlag (SIM_NO_BLACKOUT);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_FUEL))
      PlayerOptions.SetSimFlag (SIM_UNLIMITED_FUEL);
   else
      PlayerOptions.ClearSimFlag (SIM_UNLIMITED_FUEL);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_CHAFF))
      PlayerOptions.SetSimFlag (SIM_UNLIMITED_CHAFF);
   else
      PlayerOptions.ClearSimFlag (SIM_UNLIMITED_CHAFF);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_AMMO))
      PlayerOptions.SetSimFlag (SIM_UNLIMITED_AMMO);
   else
      PlayerOptions.ClearSimFlag (SIM_UNLIMITED_AMMO);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_COLLISION))
      PlayerOptions.ClearSimFlag (SIM_NO_COLLISIONS);
   else
      PlayerOptions.SetSimFlag (SIM_NO_COLLISIONS);

   if (IsDlgButtonChecked (hDlg, IDC_SIMOPTION_NAMETAGS))
      PlayerOptions.SetSimFlag (SIM_NAMETAGS);
   else
      PlayerOptions.ClearSimFlag (SIM_NAMETAGS);

   PlayerOptions.BldDeaggLevel = SendDlgItemMessage (hDlg, IDC_SIMOPTION_FEAT_DETAIL, TBM_GETPOS, 0, 0);
   PlayerOptions.ObjDeaggLevel = SendDlgItemMessage (hDlg, IDC_SIMOPTION_VEH_DETAIL, TBM_GETPOS, 0, 0);
   PlayerOptions.ObjMagnification = static_cast<float>(SendDlgItemMessage (hDlg, IDC_SIMOPTION_MAGNIFICATION, TBM_GETPOS, 0, 0));

   PlayerOptions.SaveOptions ("Default");
}

BOOL DoSimOptions (HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
int retval = FALSE;

   switch (message)
   {
      case WM_INITDIALOG:
         InitSimOptionStuff(hDlg);
         retval = TRUE;
     	break;

      case WM_COMMAND:                		/* message: received a command */
         switch (LOWORD(wParam))
         {
            case IDOK:
               UpdateSimOptions(hDlg);
            case IDCANCEL:
               EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
            break;
         }
         retval = TRUE;
      break;

      default:
//         retval = DefDlgProc(hDlg, message, wParam, lParam);
      break;
   }
   lParam = wParam;
   return (retval);
}
