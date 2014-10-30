#include "stdafx.h"
#include "popmenu.h"
#include "dispopts.h"
#include "cpmanager.h"
#include "cpres.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"
#include "mesg.h"
#include "msginc/wingmanmsg.h"
#include "msginc/atcmsg.h"
#include "msginc/awacsmsg.h"
#include "msginc/tankermsg.h"
#include "dispcfg.h"
#include "Graphics/Include/grinline.h"
#include "sinput.h"
#include "commands.h"
#include "inpFunc.h"
#include "wingorder.h"
#include "simdrive.h"
#include "nav.h"
#include "navsystem.h"
#include "popcbproto.h"
#include "flight.h"
#include "Sms.h"
#include "Fsound.h" // JPO for VF_FLIGHTNUMBER_OFFSET

extern bool g_bAWACSRequired;
extern bool g_bDisableCommsBorder; //Cobra 10/31/04 TJL

enum MenuColors {MENU_BLACK,
                 MENU_WHITE,
                 MENU_BRIGHT_RED,
                 MENU_BRIGHT_GREEN,
                 MENU_BRIGHT_BLUE,
                 MENU_DARK_GREY,
                 MENU_LIGHT_GREY,
                 MENU_CYAN,
                 MENU_YELLOW,
                 MENU_DARK_RED,
                 MENU_TOTAL_COLORS
                } MenuColors;

MenuColorStruct gMenuColorTable[MENU_TOTAL_COLORS] =
{
    {"black", 0xff000000},
    {"white", 0xffffffff},
    {"bright_red", 0xff0000ff},
    {"bright_green", 0xff00ff00},
    {"bright_blue", 0xffff0000},
    {"dark_grey", 0xff505050},
    {"light_grey", 0xffc0c0c0},
    {"cyan", 0xffffff00},
    {"yellow", 0xff00ffff},
    {"dark_red", 0xff000080}
};

BOOL FindMenuColorIndex(char* pColorName, int* pindex)
{
    BOOL found = FALSE;
    *pindex = 0;

    while ( not found and *pindex < MENU_TOTAL_COLORS)
    {
        if (strcmpi(gMenuColorTable[*pindex].mName, pColorName) == 0)
        {
            found = TRUE;
        }
        else
        {
            (*pindex)++;
        }
    }

    return found;
}


BOOL FindMenuColorValue(char* pColorName, ULONG* pvalue)
{
    BOOL found;
    int index;

    *pvalue = 0;

    found = FindMenuColorIndex(pColorName, &index);

    if (found)
    {
        *pvalue = gMenuColorTable[index].mValue;
    }

    return found;
}


MenuManager::MenuManager(int width, int height)
{
    float bufWidth; // Buffer width
    float bufHeight; // Buffer height
    float halfSWidth; // Screen width
    float halfSHeight; // Screen height
    RECT backDest = {0};
    int backWidth = 0;
    int backHeight = 0;
    BOOL found = FALSE;
    int i;

    mIsActive = FALSE;
    mCurMenu = 0;
    mCurPage = 0;
    mTargetId = FalconNullId;
    mpResDimensions = NULL;

    ReadDataFile("art\\ckptart\\menu.dat");

    ShiAssert(mpResDimensions); // No dimension information in the data file

    backWidth = 0;
    backHeight = 0;

    for (i = 0; i < mTotalRes; i++)
    {
        if ( not found and mpResDimensions[i].xRes == width and mpResDimensions[i].yRes == height)
        {
            backDest.top = mpResDimensions[i].mDimensions.top;
            backDest.left = mpResDimensions[i].mDimensions.left;
            backDest.bottom = mpResDimensions[i].mDimensions.bottom;
            backDest.right = mpResDimensions[i].mDimensions.right;

            found = TRUE;
        }
        else if ( not found and 
                 mpResDimensions[i].xRes < width and 
                 mpResDimensions[i].xRes > backWidth and 
                 mpResDimensions[i].yRes < height and 
                 mpResDimensions[i].yRes > backHeight)
        {

            backWidth = mpResDimensions[i].xRes;
            backHeight = mpResDimensions[i].yRes;
            backDest.top = mpResDimensions[i].mDimensions.top;
            backDest.left = mpResDimensions[i].mDimensions.left;
            backDest.bottom = mpResDimensions[i].mDimensions.bottom;
            backDest.right = mpResDimensions[i].mDimensions.right;
        }
    }

    bufWidth = (float)backDest.right - (float)backDest.left + 1.0F;
    bufHeight = (float)backDest.bottom - (float)backDest.top + 1.0F;

    mDestRect.top = backDest.top;
    mDestRect.left = backDest.left;
    mDestRect.bottom = FloatToInt32(mDestRect.top + bufHeight);
    mDestRect.right = FloatToInt32(mDestRect.left + bufWidth);

    halfSWidth = (float) DisplayOptions.DispWidth * 0.5F;
    halfSHeight = (float) DisplayOptions.DispHeight * 0.5F;

    mLeft = (mDestRect.left - halfSWidth) / halfSWidth;
    mRight = (mDestRect.right - halfSWidth) / halfSWidth;
    mTop = -(mDestRect.top - halfSHeight) / halfSHeight;
    mBottom = -(mDestRect.bottom - halfSHeight) / halfSHeight;
}


void MenuManager::ReadDataFile(char* pfileName)
{
    FILE* pFile;
    BOOL quitFlag  = FALSE;
    char* presult = "";
    char* plinePtr;
    const int lineLen = MAX_LINE_BUFFER - 1;
    char plineBuffer[MAX_LINE_BUFFER] = "";
    char *ptoken;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x23, 0x00};
    int menuNumber = -1;
    int pageNumber = -1;
    int itemNumber = -1;

    pFile = CP_OPEN(pfileName, "r");
    F4Assert(pFile);

    presult = fgets(plineBuffer, lineLen, pFile);
    quitFlag = (presult == NULL);
    plinePtr = plineBuffer;

    while ( not quitFlag)
    {

        if (*plineBuffer == '#')
        {

            ptoken = FindToken(&plinePtr, pseparators);

            if ( not strcmpi(ptoken, TYPE_MENUMANGER_STR))
            {
                ParseManagerInfo(plinePtr);
            }
            else if ( not strcmpi(ptoken, TYPE_MENU_STR))
            {
                ParseMenuInfo(plinePtr, &menuNumber, &pageNumber);
            }
            else if ( not strcmpi(ptoken, TYPE_PAGE_STR))
            {
                ParsePageInfo(plinePtr, &menuNumber, &pageNumber, &itemNumber);
            }
            else if ( not strcmpi(ptoken, TYPE_ITEM_STR))
            {
                ParseItemInfo(plinePtr, &menuNumber, &pageNumber, &itemNumber);
            }
            else if ( not strcmpi(ptoken, TYPE_RES_STR))
            {
                ParseResInfo(plinePtr, --mResCount);
            }
            else
            {
                ShiWarning("Bad String in menu file");
            }
        }

        presult = fgets(plineBuffer, lineLen, pFile);
        plinePtr = plineBuffer;
        quitFlag = (presult == NULL);
    }

    CP_CLOSE(pFile);
    ShiAssert(mResCount == 0); // Bad data in the manager information line
}


void MenuManager::ParseResInfo(char* plinePtr, int resCount)
{
    ShiAssert(resCount >= 0);

    sscanf(plinePtr, "%d %d %d %d %d %d", &(mpResDimensions[resCount].xRes),
           &(mpResDimensions[resCount].yRes),
           &(mpResDimensions[resCount].mDimensions.top),
           &(mpResDimensions[resCount].mDimensions.left),
           &(mpResDimensions[resCount].mDimensions.bottom),
           &(mpResDimensions[resCount].mDimensions.right));


}



void MenuManager::ParseManagerInfo(char* plinePtr)
{
    sscanf(plinePtr, "%d %d %d", &mNumMenus, &mMaxTextLen, &mTotalRes);

    mMaxTextLen++;

#ifdef USE_SH_POOLS
    mpMenus = (MenuStruct *)MemAllocPtr(gCockMemPool, sizeof(MenuStruct) * mNumMenus, FALSE);
#else
    mpMenus = new MenuStruct[mNumMenus];
#endif

    mResCount = mTotalRes;

#ifdef USE_SH_POOLS
    mpResDimensions = (ResStruct *)MemAllocPtr(gCockMemPool, sizeof(ResStruct) * mTotalRes, FALSE);
#else
    mpResDimensions = new ResStruct[mTotalRes];
#endif
}


void MenuManager::ParseMenuInfo(char* plinePtr, int* menuNumber, int* pageNumber)
{
    (*menuNumber)++;
    *pageNumber = -1;

    char pdrawColor[20];
    ULONG color;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x23, 0x00};
    char *ptoken;
    char pmsgName[30] = "";
    char paiExtent[20] = "";
    BOOL found;
    int id;
    int i;


    MenuStruct* pMenu = &(mpMenus[*menuNumber]);

#ifdef USE_SH_POOLS
    pMenu->mpTitle = (char *)MemAllocPtr(gCockMemPool, sizeof(char) * mMaxTextLen, FALSE);
#else
    pMenu->mpTitle = new char[mMaxTextLen];
#endif
    *(pMenu->mpTitle) = '\0';


    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%s",  pmsgName);
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%s",  paiExtent);
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pMenu->mNumPages));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pMenu->mCondition));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%s",  pdrawColor);

    ptoken = FindToken(&plinePtr, pseparators);

    while (ptoken)
    {
        strcat(pMenu->mpTitle, ptoken);
        ptoken = FindToken(&plinePtr, pseparators);

        if (ptoken)
        {
            strcat(pMenu->mpTitle, " ");
        }
    }

    for (i = 0; i < AiTotalExtent; i++)
    {
        if ( not strcmpi(paiExtent, gpAiExtentStr[i]))
        {
            pMenu->mWingExtent = i;;
        }
    }

    if (FindMenuColorValue(pdrawColor, &color))
    {
        pMenu->mDrawColor = color;
    }
    else
    {
        pMenu->mDrawColor = gMenuColorTable[MENU_WHITE].mValue;
    }

    id = 0;
    found = FALSE;

    while ( not found and id <= SimRoughPositionUpdateMsg)
    {

        if ( not strcmpi(pmsgName, TheEventStrings[FalconMsgIdStr[id]]))
        {
            found = TRUE;
            pMenu->mMsgId = id + (VU_LAST_EVENT + 1);
        }
        else
        {
            id++;
        }
    }

    F4Assert(found);


#ifdef USE_SH_POOLS
    pMenu->mpPages = (PageStruct *)MemAllocPtr(gCockMemPool, sizeof(PageStruct) * pMenu->mNumPages, FALSE);
#else
    pMenu->mpPages = new PageStruct[pMenu->mNumPages];
#endif
}

void MenuManager::ParsePageInfo(char* plinePtr, int* menuNumber, int* pageNumber, int* itemNumber)
{
    (*pageNumber)++;
    *itemNumber = -1;

    char pdrawColor[20];
    ULONG color;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x23, 0x00};
    char *ptoken;

    PageStruct* pPage = &(mpMenus[*menuNumber].mpPages[*pageNumber]);

#ifdef USE_SH_POOLS
    pPage->mpTitle = (char *)MemAllocPtr(gCockMemPool, sizeof(char) * mMaxTextLen, FALSE);
#else
    pPage->mpTitle = new char[mMaxTextLen];
#endif

    *(pPage->mpTitle) = '\0';

    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pPage->mNumItems));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pPage->mCondition));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%s",  pdrawColor);

    *(pPage->mpTitle) = NULL;
    ptoken = FindToken(&plinePtr, pseparators);

    while (ptoken)
    {
        strcat(pPage->mpTitle, ptoken);
        ptoken = FindToken(&plinePtr, pseparators);

        if (ptoken)
        {
            strcat(pPage->mpTitle, " ");
        }
    }

    if (FindMenuColorValue(pdrawColor, &color))
    {
        pPage->mDrawColor = color;
    }
    else
    {
        pPage->mDrawColor = gMenuColorTable[MENU_WHITE].mValue;
    }


#ifdef USE_SH_POOLS
    pPage->mpItems = (ItemStruct *)MemAllocPtr(gCockMemPool, sizeof(ItemStruct) * pPage->mNumItems, FALSE);
#else
    pPage->mpItems = new ItemStruct[pPage->mNumItems];
#endif
}

void MenuManager::ParseItemInfo(char* plinePtr, int* menuNumber, int* pageNumber, int* itemNumber)
{
    (*itemNumber)++;

    char pdrawColor[20];
    ULONG color;
    char pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x23, 0x00};
    char *ptoken;

    ItemStruct* pItem = &(mpMenus[*menuNumber].mpPages[*pageNumber].mpItems[*itemNumber]);

#ifdef USE_SH_POOLS
    pItem->mpText = (char *)MemAllocPtr(gCockMemPool, sizeof(char) * mMaxTextLen, FALSE);
#else
    pItem->mpText = new char[mMaxTextLen];
#endif

    *(pItem->mpText) = '\0';

    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pItem->mCondition));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pItem->mPoll));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%s",  pdrawColor);
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%f",  &(pItem->mSpacing));
    ptoken = FindToken(&plinePtr, pseparators);
    sscanf(ptoken, "%d",  &(pItem->mMessage));

    int i = 0;

    while (*plinePtr not_eq '\n' and *plinePtr not_eq NULL)
    {
        pItem->mpText[i++] = *(plinePtr++);
    }

    pItem->mpText[i] = '\0';

    if (FindMenuColorValue(pdrawColor, &color))
    {
        pItem->mNormColor = color;
    }
    else
    {
        pItem->mNormColor = gMenuColorTable[MENU_WHITE].mValue;
    }
}



MenuManager::~MenuManager()
{
    int i, j, k;

    for (i = 0; i < mNumMenus; i++)
    {
        for (j = 0; j < mpMenus[i].mNumPages; j++)
        {
            for (k = 0; k < mpMenus[i].mpPages[j].mNumItems; k++)
            {
                delete [] mpMenus[i].mpPages[j].mpItems[k].mpText;
            }

            delete [] mpMenus[i].mpPages[j].mpItems;
            delete [] mpMenus[i].mpPages[j].mpTitle;
        }

        delete [] mpMenus[i].mpPages;
        delete [] mpMenus[i].mpTitle;
    }

    delete [] mpResDimensions;
    delete [] mpMenus;
}


void MenuManager::InitPage()
{
    mCallerIdx = SimDriver.GetPlayerEntity()->GetCampaignObject()->GetComponentIndex(SimDriver.GetPlayerEntity());
    mNumInFlight = ((FlightClass*) SimDriver.GetPlayerEntity()->GetCampaignObject())->GetTotalVehicles();
    mExtent = mpMenus[mCurMenu].mWingExtent;

    mpPage = &(mpMenus[mCurMenu].mpPages[mCurPage]);
    mNumItems = mpPage->mNumItems;
    // check for OnGround status
    mOnGround = false;

    if (SimDriver.GetPlayerEntity()->OnGround())
        mOnGround = true;

    // check for an AWACS in the sky
    mAWACSavail = false;

    if ( not g_bAWACSRequired) // Only check for AWACS available if user wants AWACS required...
        mAWACSavail = true;

    Unit nu, cf;
    VuListIterator myit(AllAirList);
    nu = (Unit) myit.GetFirst();

    while (nu and not mAWACSavail)
    {
        cf = nu;
        nu = (Unit) myit.GetNext();

        // 2002-03-07 MN of course only AWACS from our team - doh
        if ( not cf->IsFlight() or cf->IsDead())
            continue;

        if (cf->GetUnitMission() == AMIS_AWACS and cf->GetTeam() == SimDriver.GetPlayerEntity()->GetTeam())
        {
            mAWACSavail = true;
        }
    }
}


void MenuManager::CheckItemConditions(BOOL poll)
{
    int i;
    int condition;

    // COBRA - RED - CTD Fix
    if ( not SimDriver.GetPlayerEntity()) return;

    mIsPolling = poll;
    //Cobra this should update our menu list while open
    bool doLoop = TRUE;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    mTargetId = AiDesignateTarget(playerAC);

    for (i = 0; i < mNumItems; i++)
    {
        if (doLoop == TRUE or poll == FALSE or mpPage->mpItems[i].mPoll)
        {
            condition = mpPage->mpItems[i].mCondition;

            if (condition < 0)
            {
                mpPage->mpItems[i].mIsAvailable = TRUE;
                mpPage->mpItems[i].mDrawColor = mpPage->mpItems[i].mNormColor;
            }
            else if (condition == 5 and not mOnGround)
            {
                mpPage->mpItems[i].mIsAvailable = TRUE;
                mpPage->mpItems[i].mDrawColor = mpPage->mpItems[i].mNormColor;
            }
            else if (condition == 6 and mAWACSavail)
            {
                mpPage->mpItems[i].mIsAvailable = TRUE;
                mpPage->mpItems[i].mDrawColor = mpPage->mpItems[i].mNormColor;
            }
            else if (condition == 7 and mOnGround)
            {
                mpPage->mpItems[i].mIsAvailable = TRUE;
                mpPage->mpItems[i].mDrawColor = mpPage->mpItems[i].mNormColor;
            }
            else if (MenuCallbackArray[mpPage->mpItems[i].mCondition](mCallerIdx, mNumInFlight, mExtent, mIsPolling, mTargetId))
            {
                mpPage->mpItems[i].mIsAvailable = TRUE;
                mpPage->mpItems[i].mDrawColor = mpPage->mpItems[i].mNormColor;
            }
            else
            {
                mpPage->mpItems[i].mIsAvailable = FALSE;
                mpPage->mpItems[i].mDrawColor = gMenuColorTable[MENU_DARK_GREY].mValue;
            }
        }
    }
}

void MenuManager::DisplayDraw(void)
{
    if (mIsActive)
    {

        float top;
        float left;
        float bottom;
        float right;

        float position = 0.8F;

        int mesgNum;
        int eType;
        int elementNum;
        int numElements;
        int oldFont;

        char ptextStr[30] = "";
        int i;

        // save the current viewport
        oldFont = VirtualDisplay::CurFont();
        OTWDriver.renderer->GetViewport(&left, &top, &right, &bottom);

        // set the new viewport
        VirtualDisplay::SetFont(OTWDriver.pCockpitManager->PopUpFont());

        OTWDriver.renderer->context.ClearBuffers(MPR_CI_ZBUFFER);
        OTWDriver.renderer->StartDraw();

        // ASSO: disable the radio comms menu border //Cobra 10/31/04 TJL
        if ( not g_bDisableCommsBorder)
        {
            OTWDriver.renderer->SetViewport(-1.0, 1.0, 1.0, -1.0);

            OTWDriver.renderer->SetColor(0x997B5200); // 60% alpha blue
            OTWDriver.renderer->context.RestoreState(STATE_ALPHA_SOLID);
            OTWDriver.renderer->Render2DTri((float)mDestRect.left, (float)mDestRect.top,
                                            (float)mDestRect.right - 1.0F, (float)mDestRect.top,
                                            (float)mDestRect.right - 1.0F, (float)mDestRect.bottom);
            OTWDriver.renderer->Render2DTri((float)mDestRect.left, (float)mDestRect.top,
                                            (float)mDestRect.left, (float)mDestRect.bottom,
                                            (float)mDestRect.right - 1.0F, (float)mDestRect.bottom);

            OTWDriver.renderer->SetColor(0xFF000000); // black

            OTWDriver.renderer->Render2DLine((float)mDestRect.left, (float)mDestRect.top,
                                             (float)mDestRect.right - 1.0F, (float)mDestRect.top);
            OTWDriver.renderer->Render2DLine((float)mDestRect.right - 1.0F, (float)mDestRect.top,
                                             (float)mDestRect.right - 1.0F, (float)mDestRect.bottom);
            OTWDriver.renderer->Render2DLine((float)mDestRect.right - 1.0F, (float)mDestRect.bottom,
                                             (float)mDestRect.left, (float)mDestRect.bottom);
            OTWDriver.renderer->Render2DLine((float)mDestRect.left, (float)mDestRect.bottom,
                                             (float)mDestRect.left, (float)mDestRect.top);
        }


        OTWDriver.renderer->SetViewport(mLeft, mTop, mRight, mBottom);

        // set the color and print the text for the menu
        OTWDriver.renderer->SetColor(mpMenus[mCurMenu].mDrawColor);
        OTWDriver.renderer->TextCenter(0.0F, position, mpMenus[mCurMenu].mpTitle);

        // set the color and print the text for the page
        position -= 0.15F;
        OTWDriver.renderer->SetColor(mpMenus[mCurMenu].mpPages[mCurPage].mDrawColor);
        OTWDriver.renderer->TextCenter(0.0F, position, mpMenus[mCurMenu].mpPages[mCurPage].mpTitle);

        CheckItemConditions(TRUE);

        mesgNum = mpMenus[mCurMenu].mMsgId;

        mesgNum -= (VU_LAST_EVENT + 1);
        numElements = MsgNumElements[mesgNum];
        elementNum = 0;
        eType = -1;

        while (eType < 0 and elementNum < numElements)
        {
            eType = FalconMsgElementTypes[mesgNum][elementNum] - 1001;
            elementNum++;
        }

        position -= 0.2F;

        // draw text here
        for (i = 0; i < mpMenus[mCurMenu].mpPages[mCurPage].mNumItems; i++)
        {
            position -= mpMenus[mCurMenu].mpPages[mCurPage].mpItems[i].mSpacing;

            sprintf(ptextStr, "%d", (i + 1) % 10);

            if (mpMenus[mCurMenu].mpPages[mCurPage].mpItems[i].mIsAvailable)
            {
                OTWDriver.renderer->SetColor(gMenuColorTable[MENU_BRIGHT_GREEN].mValue);
            }
            else
            {
                OTWDriver.renderer->SetColor(gMenuColorTable[MENU_DARK_GREY].mValue);
            }

            OTWDriver.renderer->TextLeft(-0.70F, position, ptextStr);

            OTWDriver.renderer->SetColor(mpMenus[mCurMenu].mpPages[mCurPage].mpItems[i].mDrawColor);
            OTWDriver.renderer->TextLeft(-0.60F, position, mpMenus[mCurMenu].mpPages[mCurPage].mpItems[i].mpText);
        }

        OTWDriver.renderer->EndDraw();

        // restore the old viewport
        OTWDriver.renderer->SetViewport(left, top, right, bottom);
        VirtualDisplay::SetFont(oldFont);
    }
}


void MenuManager::StepNextPage(int state)
{
    if (state bitand KEY_DOWN)
    {
        if (mpMenus[mCurMenu].mNumPages > 1)
        {
            mCurPage++;

            if (mCurPage >= mpMenus[mCurMenu].mNumPages)
            {
                mCurPage = 0;
            }

            InitPage();
            CheckItemConditions(FALSE);
        }

        CommandsSetKeyCombo(CommandsKeyCombo, state, NULL);
    }
}

void MenuManager::StepPrevPage(int state)
{
    if (state bitand KEY_DOWN)
    {
        if (mpMenus[mCurMenu].mNumPages > 1)
        {
            mCurPage--;

            if (mCurPage < 0)
            {
                mCurPage = mpMenus[mCurMenu].mNumPages - 1;
            }

            InitPage();
            CheckItemConditions(FALSE);
        }

        CommandsSetKeyCombo(CommandsKeyCombo, state, NULL);
    }
}


void MenuManager::DeActivate(void)
{
    mIsActive = FALSE;
}

void MenuManager::DeActivateAndClear(void)
{
    mIsActive = FALSE;
    CommandsKeyCombo = 0;
    CommandsKeyComboMod = 0;
}


void MenuManager::ProcessInput(unsigned long val, int state, int type, int extent)
{
    int message;
    int item = -1;
    int i;
    ItemStruct* pitem;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (state bitand KEY_DOWN)
    {

        if (mIsActive)
        {

            switch (val)
            {

                case DIK_0:
                    item = 9;
                    break;

                case DIK_1:
                case DIK_2:
                case DIK_3:
                case DIK_4:
                case DIK_5:
                case DIK_6:
                case DIK_7:
                case DIK_8:
                case DIK_9:
                    item = val - DIK_1;
                    break;
            }

            if (item not_eq -1)
            {
                DeActivateAndClear();

                pitem = &(mpMenus[mCurMenu].mpPages[mCurPage].mpItems[item]);

                if (pitem->mIsAvailable)
                {
                    message = pitem->mMessage;
                    mTargetId = AiDesignateTarget(playerAC);
                    SendMenuMsg(mpMenus[mCurMenu].mMsgId, message, mpMenus[mCurMenu].mWingExtent, mTargetId);
                }
            }
            else if (val == DIK_SYSRQ)
            {
                CommandsKeyCombo = 0;
                CommandsKeyComboMod = 0;
                OTWDriver.takeScreenShot = TRUE;
            }
            else
            {
                DeActivateAndClear();
            }
        }
        else
        {
            mIsActive = TRUE;

            mTargetId = AiDesignateTarget(playerAC);

            for (i = 0; i < mNumMenus; i++)
            {
                if (mpMenus[i].mMsgId == type and mpMenus[i].mWingExtent == extent)
                {
                    mCurMenu = i;
                }
            }

            mCurPage = 0;
            InitPage();
            CheckItemConditions(FALSE);
            CommandsSetKeyCombo(val, state, NULL);
        }
    }
}


void MenuManager::SendMenuMsg(int msgType, int enumId, int aiExtent, VU_ID targetId)
{
    if (SimDriver.GetPlayerEntity())
    {
        switch (msgType)
        {
            case AWACSMsg:
                MenuSendAwacs(enumId, targetId);
                break;

            case ATCMsg:
                MenuSendAtc(enumId);
                break;

            case TankerMsg:
                MenuSendTanker(enumId);
                break;

            case WingmanMsg:
                AiSendPlayerCommand(enumId, aiExtent, targetId);
                break;

            default:
                ShiWarning("Unsupported Message Type"); // unsupported message
                break;
        }
    }
}


void MenuSendAtc(int enumId, int sendRequest)
{
    VU_ID ATCId = vuNullId;
    ObjectiveClass *theATC;
    FalconATCMessage *atcMsg;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    gNavigationSys->GetAirbase(&ATCId);

    if (sendRequest and PlayerOptions.PlayerRadioVoice and playerAC)
    {
        switch (enumId)
        {
            case FalconATCMessage::ContactApproach:
            case FalconATCMessage::RequestClearance:
                SendCallToATC(playerAC, ATCId, rcLANDCLEARANCE, FalconLocalGame);
                break;

            case FalconATCMessage::RequestEmerClearance:
                SendCallToATC(playerAC, ATCId, rcLANDCLEAREMERGENCY, FalconLocalGame);
                break;

            case FalconATCMessage::RequestTakeoff:
                SendCallToATC(playerAC, ATCId, rcREADYFORDERARTURE, FalconLocalGame);
                break;

            case FalconATCMessage::RequestTaxi:
                SendCallToATC(playerAC, ATCId, rcREADYFORDERARTURE, FalconLocalGame);
                break;

            case FalconATCMessage::AbortApproach:
                SendCallToATC(playerAC, ATCId, rcABORTAPPROACH, FalconLocalGame);
                break;

                //RAS-17Jan04-Added for traffic call acknowledgement
            case FalconATCMessage::TrafficInSight:
                SendCallToATC(playerAC, rcCOPY, FalconLocalGame);
                break;

                //TJL 08/16/04 Hotpit Refuel //Cobra 10/31/04 TJL
            case FalconATCMessage::RequestHotpitRefuel:
                SendCallToATC(playerAC, rcCOPY, FalconLocalGame);
                break;

            case FalconATCMessage::UpdateStatus:
            default:
                break;
        }
    }

    theATC = (ObjectiveClass*)vuDatabase->Find(ATCId);
    ShiAssert(theATC and theATC->brain);

    if (theATC and theATC->brain)
        atcMsg = new FalconATCMessage(ATCId, FalconLocalGame);
    else
        atcMsg = new FalconATCMessage(FalconNullId, FalconLocalGame);

    atcMsg->dataBlock.type = enumId;

    if (SimDriver.GetPlayerEntity())
        atcMsg->dataBlock.from = SimDriver.GetPlayerEntity()->Id();
    else
        atcMsg->dataBlock.from = FalconNullId;

    FalconSendMessage(atcMsg, FALSE);
}


void MenuSendAwacs(int enumId, VU_ID targetId, int sendRequest)
{
    FalconAWACSMessage *pawacsMsg;
    FalconRadioChatterMessage* radioMessage;
    Flight flight;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    // Send the 'call' message
    if (sendRequest and PlayerOptions.PlayerRadioVoice)
    {
        switch (enumId)
        {
            case FalconAWACSMessage::Unable:
                SendCallToAWACS(playerAC, rcUNABLE, FalconLocalGame);
                break;

            case FalconAWACSMessage::Wilco:
                SendCallToAWACS(playerAC, rcCOPY, FalconLocalGame);
                break;

            case FalconAWACSMessage::Judy:
                // SendCallToAWACS(playerAC, rcJUDY);
                break;

            case FalconAWACSMessage::RequestPicture:
                SendCallToAWACS(playerAC, rcPICTUREQUERY, FalconLocalGame);
                break;

            case FalconAWACSMessage::GivePicture:
                break;

            case FalconAWACSMessage::RequestHelp:
                radioMessage = CreateCallToAWACS(playerAC, rcNEEDHELP, FalconLocalGame);
                radioMessage->dataBlock.edata[1] -= VF_FLIGHTNUMBER_OFFSET; // jpo - rewrite this...
                radioMessage->dataBlock.edata[2] = SimToGrid(playerAC->YPos());
                radioMessage->dataBlock.edata[3] = SimToGrid(playerAC->XPos());
                FalconSendMessage(radioMessage, FALSE);
                break;

            case FalconAWACSMessage::RequestRelief:
                flight = (Flight)playerAC->GetCampaignObject();

                if (flight)
                {
                    int hasWeaps = 0, role, hp;
                    SMSClass *sms = (SMSClass*) playerAC->GetSMS();

                    role = flight->GetUnitCurrentRole();

                    if (sms)
                    {
                        for (hp = 1; hp < sms->NumHardpoints(); hp++)
                        {
                            if (role == ARO_CA)
                            {
                                if (
                                    sms->hardPoint[hp] and 
                                    sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdAir
                                )
                                {
                                    hasWeaps++;
                                }
                            }
                            else if (role == ARO_S or role == ARO_GA or role == ARO_SB or role == ARO_SEAD)
                            {
                                if (
                                    sms->hardPoint[hp] and 
                                    sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdGround
                                )
                                {
                                    hasWeaps++;
                                }
                            }
                            else if (role == ARO_ASW or role == ARO_ASHIP)
                            {
                                if (
                                    sms->hardPoint[hp] and 
                                    sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdGround
                                )
                                {
                                    hasWeaps++;
                                }
                            }
                        }
                    }

                    // Pick what we say depending on our status
                    if ( not hasWeaps)
                        SendCallToAWACS(playerAC, rcENDCAPARMS, FalconLocalGame);
                    else
                        SendCallToAWACS(playerAC, rcENDCAPFUEL, FalconLocalGame);
                }

                break;

            case FalconAWACSMessage::RequestDivert:
                SendCallToAWACS(playerAC, rcREQUESTTASK, FalconLocalGame);
                break;

            case FalconAWACSMessage::RequestSAR:
                SendCallToAWACS(playerAC, rcSENDCHOPPERS, FalconLocalGame);
                // SendCallToAWACS(plane, rcAIRMANDOWND, FalconLocalGame); // JPO addition doesn't work
                break;

            case FalconAWACSMessage::OnStation:
                //TJL 12/14/03 Changing FAC Request
                //SendCallToAWACS(plane, rcFACCONTACT, FalconLocalGame);
                SendCallToAWACS(playerAC, rcFACREADY, FalconLocalGame);
                break;

            case FalconAWACSMessage::OffStation:
                // KCK: I don't think we have the speach to impliment this
                // SendCallToAWACS(plane, rcFACCONTACT);
                // VWF: It seems we dont have a "check out" call. Vamoose is as
                // close as it gets.
                //TJL 12/14/03 Enable Check Out Speech.
                SendCallToAWACS(playerAC, rcIMADOT, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorHome:
                SendCallToAWACS(playerAC, rcVECTORHOMEPLATE, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToAltAirfield:
                SendCallToAWACS(playerAC, rcDIVERTFIELD, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToPackage:
                SendCallToAWACS(playerAC, rcVECTORTOPACKAGE, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToTanker:
                SendCallToAWACS(playerAC, rcREQUESTVECTORTOTANKER, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToCarrier: // Carrier
                SendCallToAWACS(playerAC, rcVECTORTOCARRIER, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToThreat:
                SendCallToAWACS(playerAC, rcVECTORTOTHREAT, FalconLocalGame);
                break;

            case FalconAWACSMessage::VectorToTarget:
                SendCallToAWACS(playerAC, rcVECTORTOTARGET, FalconLocalGame);
                break;

            case FalconAWACSMessage::DeclareAircraft:
                SendCallToAWACS(playerAC, rcDECLARE, FalconLocalGame);
                break;

            default:
                break;
        }
    }

    // Now send the message
    pawacsMsg = new FalconAWACSMessage(SimDriver.GetPlayerEntity()->Id(), FalconLocalGame);
    pawacsMsg->dataBlock.type = enumId;
    pawacsMsg->dataBlock.caller = targetId;

    MonoPrint("Sending AWACS message #%d.\n", enumId);

    FalconSendMessage(pawacsMsg, TRUE);
}

void MenuSendWingman(int enumId, int extent)
{
    AiSendPlayerCommand(enumId, extent, AiDesignateTarget(SimDriver.GetPlayerAircraft()));
}

void MenuSendTanker(int enumId)
{
    VU_ID TankerId = vuNullId;
    AircraftClass *theTanker = NULL;
    FalconTankerMessage *TankerMsg;
    FlightClass *flight;

    gNavigationSys->GetTacanVUID(gNavigationSys->GetControlSrc(), &TankerId);

    if (TankerId == FalconNullId)
        flight = SimDriver.FindTanker(SimDriver.GetPlayerEntity());
    else
    {
        flight = (Flight)vuDatabase->Find(TankerId);

        if ( not flight->IsFlight())
        {
            flight = SimDriver.FindTanker(SimDriver.GetPlayerEntity());
        }
    }

    if (flight)
        theTanker = (AircraftClass*) flight->GetComponentLead();

    if (theTanker)
        TankerMsg = new FalconTankerMessage(theTanker->Id(), FalconLocalGame);
    else
        TankerMsg = new FalconTankerMessage(FalconNullId, FalconLocalGame);

    TankerMsg->dataBlock.type = enumId;
    TankerMsg->dataBlock.data1  = 1;
    TankerMsg->dataBlock.caller = SimDriver.GetPlayerEntity()->Id();
    FalconSendMessage(TankerMsg);
}

