#include "stdhdr.h"
#include "Graphics/Include/Render2D.h"
#include "radar.h"
#include "mfd.h"
#include "missile.h"
#include "misldisp.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "harmpod.h"
#include "fcc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "falcsess.h"
#include "playerop.h"
#include "commands.h"
#include "SmsDraw.h"
#include "sms.h"
#include "otwdrive.h"
#include "vu2.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "aircrft.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "airframe.h"
#include "campbase.h" // Marco for AIM9P
#include "classtbl.h"
#include "otwdrive.h" //MI
#include "cpmanager.h" //MI
#include "icp.h" //MI
#include "aircrft.h" //MI
#include "fcc.h" //MI
#include "radardoppler.h" //MI
#include "simdrive.h" //MI
#include "hud.h" //MI
#include "fault.h" //MI

#define FirstLineX 0.0F
#define FirstLineY 0.2F
#define SecondLineX FirstLineX
#define SecondLineY 0.1F
#define InputLineX1 -0.5F
#define InputLineX2 0.5F
#define InputLineX3 0.0F
#define InputLineY -0.1F

extern bool g_bMLU;
extern int maxripple; // M.N.

void SmsDrawable::InputDisplay(void)
{
    InputFlash = (vuxRealTime bitand 0x180);

    switch (InputModus)
    {
        case RELEASE_PULSE:
            InputRP();
            break;

        case RELEASE_SPACE:
            InputRS();
            break;

        case CONTROL_PAGE:
            CNTLPage();
            break;

        case ARMING_DELAY:
            ADPage();
            break;

        case BURST_ALT:
            InputBA();
            break;

        case C1:
        case C2:
        case C3:
        case C4:
            CDisplay();
            break;

        case REL_ANG:
            RelAngDisplay();
            MaxInputLines = 1;
            break;

        case LADD_MODE:
            LADDDisplay();
            MaxInputLines = 3;
            break;

        default:
            break;
    }
}
void SmsDrawable::InputRP(void)
{
    LabelOSB();

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "RELEASE PULSES");
    display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
    display->TextCenter(InputLineX2, InputLineY, "\x02", 2);

    if (wrong)
    {
        WrongInput();
        return;
    }

    if ( not Manual_Input)
        //sprintf(inputstr, "%d", Sms->rippleCount + 1);
        sprintf(inputstr, "%d", Sms->GetAGBRippleCount() + 1);
    else
        FillInputString();

    display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);
}
void SmsDrawable::InputRS(void)
{
    LabelOSB();
    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "IMPACT SPACING");
    display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
    display->TextCenter(InputLineX2, InputLineY, "\x02", 2);

    if (wrong)
    {
        WrongInput();
        return;
    }

    if ( not Manual_Input)
        // sprintf(inputstr, "%dFT", Sms->rippleInterval);
        sprintf(inputstr, "%dFT", Sms->GetAGBRippleInterval());
    else
        FillInputString();

    display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);
}
void SmsDrawable::CNTLPage(void)
{
    float x = 0, y = 0;
    char tempstr[20];
    FireControlComputer *FCC = Sms->ownship->GetFCC();

    if ( not FCC)
        return;

    //OSB1
    if (FCC->IsAGMasterMode())
        LabelButton(0, "A-G");
    else
        ShiWarning("Should not be here when not in AG mode");

    //OSB2
    LabelButton(1, FCC->subModeString);
    //OSB4
    LabelButton(3, "INV");
    //OSB5
    LabelButton(4, "CNTL", NULL, 2);
    //OSB6
    GetButtonPos(5, &x, &y);
    float step = display->TextHeight() / 2;
    step += 0.04F;
    y += 1.5F * step;
    display->TextLeft(x, y, "L");
    display->TextLeft(x - 0.75F, y, "PR 25000FT");
    y -= step;
    display->TextLeft(x, y, "A");
    display->TextLeft(x - 0.75F, y, "TOF 28.00SEC");
    y -= step;
    display->TextLeft(x, y, "D");
    display->TextLeft(x - 0.75F, y, "MRA 1105FT");
    y -= step;
    display->TextLeft(x, y, "D");
    //OSB10
    //sprintf(tempstr, "%d", Sms->angle);
    sprintf(tempstr, "%d", Sms->GetAGBReleaseAngle());
    LabelButton(9, "Rel Ang", tempstr);

    BottomRow();

    //OSB17
    GetButtonPos(16, &x, &y);
    y += step;
    display->TextLeft(x, y, "C", C4Weap ? 2 : 0);
    display->TextLeft(x + 0.1f, y, "AD1 2.00SEC");
    y -= step;
    display->TextLeft(x, y, "4", C4Weap ? 2 : 0);
    display->TextLeft(x + 0.1f, y, "AD2 2.50SEC");
    y -= step;
    display->TextLeft(x + 0.1f, y, "BA 75FT");

    //OSB18
    GetButtonPos(17, &x, &y);
    y += step;
    display->TextLeft(x, y, "C", C3Weap ? 2 : 0);
    display->TextLeft(x + 0.1f, y, "AD 12.25SEC");
    y -= step;
    display->TextLeft(x, y, "3", C3Weap ? 2 : 0);
    display->TextLeft(x + 0.1f, y, "BA 100FT");

    //OSB19
    GetButtonPos(18, &x, &y);
    y += step;
    display->TextLeft(x, y, "C ", C2Weap ? 2 : 0);
    //sprintf(tempstr, "%.2fSEC", Sms->C2AD / 100);
    sprintf(tempstr, "%.2fSEC", Sms->GetAGBC2ArmDelay() / 100);
    display->TextLeft(x + 0.1F, y, "AD", C2Weap ? 2 : 0);
    display->TextLeft(x + 0.3F, y, tempstr);
    y -= step;
    display->TextLeft(x, y, "2 ", C2Weap ? 2 : 0);
    //sprintf(tempstr, "%dFT", Sms->C2BA);
    sprintf(tempstr, "%dFT", Sms->GetAGBBurstAlt());
    display->TextLeft(x + 0.1F, y, "BA", C2Weap ? 2 : 0);
    display->TextLeft(x + 0.3F, y, tempstr);

    //OSB20
    GetButtonPos(19, &x, &y);
    y += step;
    display->TextLeft(x, y, "C", C1Weap ? 2 : 0);

    if (Sms->curProfile == 0)
    {
        sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay1() / 100);
        display->TextLeft(x + 0.1F, y, "AD1", Sms->GetAGBFuze() == 1 ? 2 : 0);
        display->TextLeft(x + 0.3F, y, tempstr);
    }
    else
    {
        sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay1() / 100);

        if (C1Weap)
            display->TextLeft(x + 0.1F, y, "AD1", Sms->GetAGBFuze() == 1 ? 2 : 0);
        else
            display->TextLeft(x + 0.1F, y, "AD1");

        display->TextLeft(x + 0.3F, y, tempstr);
    }


    /*
     if(Sms->Prof1)
     {
     sprintf(tempstr, "%.2fSEC", Sms->C1AD1 / 100);
     display->TextLeft(x + 0.1F, y, "AD1", Sms->Prof1NSTL == 1 ? 2 : 0);
     display->TextLeft(x + 0.3F, y, tempstr);
     }
     else
     {
     sprintf(tempstr, "%.2fSEC", Sms->C1AD1 / 100);
     if(C1Weap)
     display->TextLeft(x + 0.1F, y, "AD1", Sms->Prof2NSTL == 1 ? 2 : 0);
     else
     display->TextLeft(x + 0.1F, y, "AD1");
     display->TextLeft(x + 0.3F, y, tempstr);
     }
    */

    y -= step;
    display->TextLeft(x, y, "1", C1Weap ? 2 : 0);

    if (Sms->curProfile == 0)
    {
        sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay2() / 100);

        if (C1Weap)
            display->TextLeft(x + 0.1F, y, "AD2", Sms->GetAGBFuze() not_eq 1 ? 2 : 0);
        else
            display->TextLeft(x + 0.1F, y, "AD2");

        display->TextLeft(x + 0.3F, y, tempstr);
    }
    else
    {
        sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay2() / 100);
        display->TextLeft(x + 0.1F, y, "AD2", Sms->GetAGBFuze() not_eq 1 ? 2 : 0);
        display->TextLeft(x + 0.3F, y, tempstr);
    }

    /*
    if(Sms->Prof1)
    {
     sprintf(tempstr, "%.2fSEC", Sms->C1AD2 / 100);
     if(C1Weap)
     display->TextLeft(x + 0.1F, y, "AD2", Sms->Prof1NSTL not_eq 1 ? 2 : 0);
     else
     display->TextLeft(x + 0.1F, y, "AD2");
     display->TextLeft(x + 0.3F, y, tempstr);
    }
    else
    {
     sprintf(tempstr, "%.2fSEC", Sms->C1AD2 / 100);
     display->TextLeft(x + 0.1F, y, "AD2", Sms->Prof2NSTL not_eq 1 ? 2 : 0);
     display->TextLeft(x + 0.3F, y, tempstr);
    }*/
}
void SmsDrawable::ADPage(void)
{
    LabelOSB();

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "ARMING DELAY");
    display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
    display->TextCenter(InputLineX2, InputLineY, "\x02", 2);

    if ( not Manual_Input)
        sprintf(inputstr, "%.2fSEC", Sms->armingdelay / 100);
    else
        FillInputString();

    display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);
}
void SmsDrawable::InputBA(void)
{
    LabelOSB();

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "BURST HEIGHT");
    display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
    display->TextCenter(InputLineX2, InputLineY, "\x02", 2);

    if ( not Manual_Input)
        sprintf(inputstr, "BA %.0f", Sms->burstHeight);
    else
        FillInputString();

    display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);
}
void SmsDrawable::CDisplay(void)
{
    LabelOSB();
    char tempstr[10];

    if (InputModus == C1)
    {
        display->TextCenter(SecondLineX, SecondLineY, "CAT 1 AD1/AD2");

        if (InputLine <= 0)
        {
            //Line1
            if ( not Manual_Input)
                //sprintf(inputstr, "%.2fSEC", Sms->C1AD1 / 100); // MLR 4/3/2004 -
                sprintf(inputstr, "%.2fSEC", Sms->GetAGBC1ArmDelay1() / 100);
            else
                FillInputString();

            display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);

            //Line2
            //sprintf(tempstr, "%.2fSEC", Sms->C1AD2 / 100);
            sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay2() / 100);
            display->TextCenter(InputLineX3, InputLineY - 0.1F, tempstr);
        }
        else if (InputLine > 0)
        {
            //Line1
            //sprintf(tempstr, "%.2fSEC", Sms->C1AD1 / 100); // MLR 4/3/2004 -
            sprintf(tempstr, "%.2fSEC", Sms->GetAGBC1ArmDelay1() / 100);
            display->TextCenter(InputLineX3, InputLineY, tempstr);

            //Line2
            if ( not Manual_Input)
                //sprintf(inputstr, "%.2fSEC", Sms->C1AD2 / 100);
                sprintf(inputstr, "%.2fSEC", Sms->GetAGBC1ArmDelay2() / 100);
            else
                FillInputString();

            display->TextCenter(InputLineX3, InputLineY - 0.1F, inputstr, Manual_Input ? 2 : 0);
        }
    }
    else if (InputModus == C2)
    {
        display->TextCenter(SecondLineX, SecondLineY, "CAT 2 AD/BA");

        if (InputLine <= 0)
        {
            //Line1
            if ( not Manual_Input)
                //sprintf(inputstr, "%.2fSEC", Sms->C2AD / 100);
                sprintf(inputstr, "%.2fSEC", Sms->GetAGBC2ArmDelay() / 100);
            else
                FillInputString();

            display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);

            //Line2
            //sprintf(tempstr, "%dFT", Sms->C2BA);
            sprintf(tempstr, "%dFT", Sms->GetAGBBurstAlt());
            display->TextCenter(InputLineX3, InputLineY - 0.1F, tempstr);
        }
        else if (InputLine > 0)
        {
            //Line1
            //sprintf(tempstr, "%.2fSEC", Sms->C2AD / 100);
            sprintf(tempstr, "%.2fSEC", Sms->GetAGBC2ArmDelay() / 100);
            display->TextCenter(InputLineX3, InputLineY, tempstr);

            //Line2
            if ( not Manual_Input)
                //sprintf(inputstr, "%dFT", Sms->C2BA);
                sprintf(inputstr, "%dFT", Sms->GetAGBBurstAlt());
            else
                FillInputString();

            display->TextCenter(InputLineX3, InputLineY - 0.1F, inputstr, Manual_Input ? 2 : 0);
        }
    }
    else if (InputModus == C3)
    {
        display->TextCenter(SecondLineX, SecondLineY, "CAT 3 AD/BA");
        //Line1
        display->TextCenter(InputLineX3, InputLineY, "12.25SEC");
        //Line2
        display->TextCenter(InputLineX3, InputLineY - 0.1F, "100FT");
    }
    else if (InputModus == C4)
    {
        display->TextCenter(SecondLineX, SecondLineY, "CAT 4 AD1/AD2/BA");
        //Line1
        display->TextCenter(InputLineX3, InputLineY, "2.00SEC");
        //Line2
        display->TextCenter(InputLineX3, InputLineY - 0.1F, "2.50SEC");
        //Line3
        display->TextCenter(InputLineX3, InputLineY - 0.2F, "75FT");
    }

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");

    if (InputLine <= 0)
    {
        display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY, "\x02", 2);
    }
    else if (InputLine == 1)
    {
        display->TextCenter(InputLineX1, InputLineY - 0.1F, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY - 0.1F, "\x02", 2);
    }
    else if (InputLine >= 2)
    {
        display->TextCenter(InputLineX1, InputLineY - 0.2F, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY - 0.2F, "\x02", 2);
    }
}
void SmsDrawable::RelAngDisplay(void)
{
    LabelOSB();

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "RELEASE ANGLE");
    display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
    display->TextCenter(InputLineX2, InputLineY, "\x02", 2);

    if (wrong)
    {
        WrongInput();
        return;
    }

    if ( not Manual_Input)
        //sprintf(inputstr, "%d", Sms->angle);
        sprintf(inputstr, "%d", Sms->GetAGBReleaseAngle());
    else
        FillInputString();

    display->TextCenter(InputLineX3, InputLineY, inputstr, Manual_Input ? 2 : 0);
}
void SmsDrawable::LADDDisplay(void)
{
    LabelOSB();

    display->TextCenter(FirstLineX , FirstLineY, "ENTER");
    display->TextCenter(SecondLineX, SecondLineY, "LADD PR/TOF/MRA");
    //Line1
    display->TextCenter(InputLineX3, InputLineY, "25000FT");
    //Line2
    display->TextCenter(InputLineX3, InputLineY - 0.1F, "28.00SEC");
    //Line3
    display->TextCenter(InputLineX3, InputLineY - 0.2F, "1105FT");

    if (InputLine <= 0)
    {
        display->TextCenter(InputLineX1, InputLineY, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY, "\x02", 2);
    }
    else if (InputLine == 1)
    {
        display->TextCenter(InputLineX1, InputLineY - 0.1F, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY - 0.1F, "\x02", 2);
    }
    else if (InputLine >= 2)
    {
        display->TextCenter(InputLineX1, InputLineY - 0.2F, "\x02", 2);
        display->TextCenter(InputLineX2, InputLineY - 0.2F, "\x02", 2);
    }
}
void SmsDrawable::InputPushButton(int whichButton, int whichMFD)
{
    if ( not Manual_Input)
    {
        for (int i = 0; i < STR_LEN; i++)
            inputstr[i] = ' ';

        inputstr[STR_LEN - 1] = '\0';
    }

    if (InputsMade == PossibleInputs and CheckButton(whichButton))
        return;
    else if (CheckButton(whichButton))
        InputsMade++;

    switch (whichButton)
    {
        case 1:
            CheckInput();
            break;

        case 2:
            ClearDigits();

            if (InputModus == C1 or InputModus == C2 or InputModus == C3 or
                InputModus == C4 or InputModus == REL_ANG or InputModus == LADD_MODE)
            {
                InputModus = CONTROL_PAGE;
            }
            else
            {
                if (InputModus not_eq CONTROL_PAGE)
                    SetDisplayMode(lastInputMode);
            }

            break;

        case 3:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                displayMode = Inv;
            }
            else
                ClearDigits();

            break;

        case 4:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                SetDisplayMode(lastInputMode);
            }

            break;

        case 5:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = LADD_MODE;
            }
            else
                AddInput(whichButton);

            break;

        case 6:
        case 7:
        case 8:
            AddInput(whichButton);
            break;

        case 9:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = REL_ANG;
                PossibleInputs = 2;
            }
            else
                AddInput(-1);

            break;

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            MfdDrawable::PushButton(whichButton, whichMFD);
            break;

        case 15:
            AddInput(19 - whichButton);
            break;

        case 16:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = C4;
                MaxInputLines = 3;
            }
            else if (InputModus not_eq CONTROL_PAGE)
            {
                AddInput(19 - whichButton);
            }

            break;

        case 17:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = C3;
                MaxInputLines = 2;
            }
            else if (InputModus not_eq CONTROL_PAGE)
            {
                AddInput(19 - whichButton);
                Manual_Input = TRUE;
            }

            break;

        case 18:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = C2;
                PossibleInputs = 4;
                InputLine = 0;
                MaxInputLines = 2;
            }
            else if (InputModus not_eq CONTROL_PAGE)
            {
                AddInput(19 - whichButton);
            }

            break;

        case 19:
            if (InputModus == CONTROL_PAGE)
            {
                ClearDigits();
                InputModus = C1;
                PossibleInputs = 4;
                InputLine = 0;
                MaxInputLines = 2;
            }
            else if (InputModus not_eq CONTROL_PAGE)
            {
                AddInput(19 - whichButton);
            }

            break;

        default:
            break;
    }
}
void SmsDrawable::AddInput(int whichButton)
{
    if (Manual_Input)
    {
        Input_Digits[0] = Input_Digits[1];
        Input_Digits[1] = Input_Digits[2];
        Input_Digits[2] = Input_Digits[3];
        Input_Digits[3] = Input_Digits[4];
        Input_Digits[4] = Input_Digits[5];
        Input_Digits[5] = Input_Digits[6];
    }

    Input_Digits[6] = whichButton + 1;
    Manual_Input = TRUE;
}
void SmsDrawable::FillInputString(void)
{
    if (InputModus == RELEASE_PULSE or InputModus == REL_ANG)
    {
        for (int i = 0; i < MAX_DIGITS; i++)
        {
            if (Input_Digits[i] < 10)
                inputstr[i] = '0' + Input_Digits[i];
            else
                inputstr[i] = ' ';
        }
    }
    else if (InputModus == RELEASE_SPACE)
    {
        for (int i = 0; i < MAX_DIGITS; i++)
        {
            if (Input_Digits[i] < 10)
                inputstr[i] = '0' + Input_Digits[i];
            else
                inputstr[i] = ' ';

            inputstr[MAX_DIGITS] = 'F';
            inputstr[MAX_DIGITS + 1] = 'T';
        }
    }
    else if (InputModus == ARMING_DELAY or InputModus == C1 or (InputModus == C2 and InputLine <= 0))
    {
        inputstr[8] = 'S';
        inputstr[9] = 'E';
        inputstr[10] = 'C';

        if (Input_Digits[6] < 10)
            inputstr[7] = '0' + Input_Digits[6];
        else
            inputstr[7] = ' ';

        if (Input_Digits[5] < 10)
            inputstr[6] = '0' + Input_Digits[5];
        else
            inputstr[6] = ' ';

        inputstr[5] = '.';

        if (Input_Digits[4] < 10)
            inputstr[4] = '0' + Input_Digits[4];
        else
            inputstr[4] = ' ';

        if (Input_Digits[3] < 10)
            inputstr[3] = '0' + Input_Digits[3];
        else
            inputstr[3] = ' ';

        if (Input_Digits[2] < 10)
            inputstr[2] = '0' + Input_Digits[2];
        else
            inputstr[2] = ' ';

        if (Input_Digits[1] < 10)
            inputstr[1] = '0' + Input_Digits[1];
        else
            inputstr[1] = ' ';

        if (Input_Digits[0] < 10)
            inputstr[0] = '0' + Input_Digits[0];
        else
            inputstr[0] = ' ';
    }
    else if (InputModus == BURST_ALT or (InputModus == C2 and InputLine > 0))
    {
        for (int i = 0; i < MAX_DIGITS; i++)
        {
            if (Input_Digits[i] < 10)
                inputstr[i] = '0' + Input_Digits[i];
            else
                inputstr[i] = ' ';

            inputstr[MAX_DIGITS] = 'F';
            inputstr[MAX_DIGITS + 1] = 'T';
        }
    }

    ShiAssert(strlen(inputstr) < sizeof(inputstr));
    inputstr[STR_LEN - 1] = '\0';
}
void SmsDrawable::CheckInput(void)
{
    int var = 0;

    switch (InputModus)
    {
        case RELEASE_PULSE:
            var = AddUp();

            if (var <= maxripple)
            {
                //Sms->rippleCount = Sms->agbProfile[Sms->curProfile].rippleCount = var - 1; // MLR 4/3/2004 -
                //Sms->rippleCount = Sms->agbProfile[Sms->curProfile].rippleCount = var - 1; // MLR 4/3/2004 -
                Sms->SetAGBRippleCount(var - 1);

                /*
                if(Sms->Prof1)
                 Sms->rippleCount = Sms->Prof1RP = var - 1;
                else
                 Sms->rippleCount = Sms->Prof2RP = var - 1;
                 */
                CorrectInput();
            }
            else
            {
                wrong = TRUE;
                WrongInput();
            }

            break;

        case RELEASE_SPACE:
            var = AddUp();
            Sms->SetAGBRippleInterval(var);
            //Sms->rippleInterval = Sms->GetAGBRippleInterval();
            /*
            if(Sms->Prof1)
             Sms->rippleInterval = Sms->Prof1RS = var;
            else
             Sms->rippleInterval = Sms->Prof2RS = var;
            */

            CorrectInput();
            break;

        case ARMING_DELAY:
            if (g_bMLU)
            {
                var = AddUp();
                Sms->armingdelay = (float)var;
                ClearDigits();
                Manual_Input = FALSE;
                SetDisplayMode(lastInputMode);
            }
            else
            {
                var = AddUp();
                Sms->armingdelay = (float)var;
                CorrectInput();
            }

            break;

        case BURST_ALT:
            var = AddUp();
            //Sms->C2BA = var;
            Sms->SetAGBBurstAlt(var);
            CorrectInput();
            break;

        case C1:
            var = AddUp();

            if (Manual_Input and InputLine <= 0)
                //Sms->C1AD1 = var; // MLR 4/3/2004 -
                Sms->SetAGBC1ArmDelay1((float)var);
            else if (Manual_Input and InputLine > 0)
                //Sms->C1AD2 = var; // MLR 4/3/2004 -
                Sms->SetAGBC1ArmDelay2((float)var);

            CorrectInput();
            break;

        case C2:
            var = AddUp();

            if (Manual_Input and InputLine <= 0)
                //Sms->C2AD = var;
                Sms->SetAGBC2ArmDelay((float)var);
            else if (Manual_Input and InputLine > 0)
                //Sms->C2BA = var;
                Sms->SetAGBBurstAlt(var);

            CorrectInput();
            break;

        case REL_ANG:
            var = AddUp();

            if (var < 46)
            {
                //Sms->angle = var;
                Sms->SetAGBReleaseAngle(var);
                CorrectInput();
            }
            else
            {
                wrong = TRUE;
                WrongInput();
            }

            break;

        case LADD_MODE:
        case C3:
        case C4:
            //just step thru these
            CorrectInput();
            break;

        default:
            break;
    }
}
int SmsDrawable::AddUp(void)
{
    int var = 0;
    CheckDigits();

    if (Input_Digits[6] >= 0)
        var += Input_Digits[6];

    if (Input_Digits[5] >= 0)
        var += Input_Digits[5] * 10;

    if (Input_Digits[4] >= 0)
        var += Input_Digits[4] * 100;

    if (Input_Digits[3] >= 0)
        var += Input_Digits[3] * 1000;

    if (Input_Digits[2] >= 0)
        var += Input_Digits[2] * 10000;

    if (Input_Digits[1] >= 0)
        var += Input_Digits[1] * 100000;

    if (Input_Digits[0] >= 0)
        var += Input_Digits[0] * 1000000;

    return var;
}
void SmsDrawable::CheckDigits(void)
{
    for (int i = 0; i < MAX_DIGITS; i++)
    {
        if (Input_Digits[i] > 10)
            Input_Digits[i] = -1;
    }
}
void SmsDrawable::CorrectInput(void)
{
    ClearDigits();
    Manual_Input = FALSE;
    InputLine++;

    if (InputLine >= MaxInputLines)
        SetDisplayMode(lastInputMode);
}
void SmsDrawable::WrongInput(void)
{
    if (InputFlash)
        display->TextCenter(InputLineX3, InputLineY, inputstr, 2);
}
void SmsDrawable::ClearDigits(void)
{
    for (int i = 0; i < STR_LEN; i++)
        inputstr[i] = ' ';

    inputstr[STR_LEN - 1] = '\0';

    for (int i = 0; i < MAX_DIGITS; i++)
    {
        Input_Digits[i] = 25;
    }

    wrong = FALSE;
    InputsMade = 0;
    Manual_Input = FALSE;
}
int SmsDrawable::CheckButton(int whichButton)
{
    if (whichButton == 1 or
        whichButton == 2 or
        whichButton == 3 or
        whichButton == 10 or
        whichButton == 11 or
        whichButton == 12 or
        whichButton == 13 or
        whichButton == 14)
        return FALSE;
    else
        return TRUE;

    return FALSE;
}
void SmsDrawable::LabelOSB(void)
{
    LabelButton(1, "ENTR");
    LabelButton(2, "RTN");
    LabelButton(3, "RCL");
    LabelButton(5, "6");
    LabelButton(6, "7");
    LabelButton(7, "8");
    LabelButton(8, "9");
    LabelButton(9, "0");
    LabelButton(19, "1");
    LabelButton(18, "2");
    LabelButton(17, "3");
    LabelButton(16, "4");
    LabelButton(15, "5");
    DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
    BottomRow();
}
