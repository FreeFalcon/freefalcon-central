// Trivial COM Printer interface
// Julian Onions
#include <windows.h>
#include "falclib.h"
#import "GMPrint.tlb"
#include <atlbase.h>
#include "include/comsup.h"

extern int g_nPrintToFile;
extern bool g_bAppendToBriefingFile;
extern bool g_bBriefHTML;	//THW 2003-12-07 Don't ignore <tags> when parsing the .b layout files


int WriteBriefingToFile(_TCHAR *string, char *fname);

int
SendStringToPrinter(_TCHAR *string, _TCHAR *title)
{
    int retval = 1;
    ShiAssert(IsBadStringPtr(string, 8192) == 0);
    ShiAssert(IsBadStringPtr(title, 1024) == 0);

	if ((g_nPrintToFile & 0x03) || g_bBriefHTML) // 0x01 + 0x02 means write to file ...and to it anyway if html is wanted
	{
		char filename[_MAX_PATH];
		if (g_bBriefHTML)
			sprintf(filename,"%s.html", title);		
		else
			sprintf(filename,"%s.txt", title);
		WriteBriefingToFile (string, filename);
		return 1;
	}

//	if (!g_nPrintToFile || g_nPrintToFile & 0x02) // 0x00 + 0x02 means print out
	//THW 2004-04-12 Never print out if HTML-Briefings are enabled
	if (!g_bBriefHTML || !g_nPrintToFile || (g_nPrintToFile & 0x02)) // 0x00 + 0x02 means print out
	{
	    CoInitialize(NULL);
		ComSup::RegisterServer("GMPrint.dll");
	    GMPRINTLib::IGMPrintEZPtr p_prt; 
		HRESULT hr = p_prt.CreateInstance(__uuidof(GMPRINTLib::GMPrintEZ)); 
	    if (FAILED(hr)) {
		MonoPrint ("Failed to load printer stuff\n");
		return 0;
		}

	    // set up some page stuff
		p_prt->title = SysAllocString (L"FreeFalcon-Cobra");
	    p_prt->punch_margin = 0.5;
		p_prt->orientation = GMPRINTLib::GMP_LANDSCAPE;
		p_prt->font_size = GMPRINTLib::GMP_FONT_12;  //or even 15 if needed (or when


	    // now loop printing each line in turn.
		for (_TCHAR *cp = string; cp; ) {
		_TCHAR *dp = strchr(cp, '\n');
		if (dp) {
			CComBSTR pbstr(dp-cp+1, cp);
		    p_prt->write(GMPRINTLib::GMP_LT_BODY, (BSTR)pbstr);
		    dp++;
		}
		else {
			CComBSTR pbstr(cp);
		    p_prt->write(GMPRINTLib::GMP_LT_BODY, (BSTR)pbstr);
		}
		cp = dp;
		}

		CComBSTR bstr(title);
		p_prt->write(GMPRINTLib::GMP_LT_HEAD, (BSTR)bstr);

	    if (retval == 1 && FAILED(p_prt->print(GMPRINTLib::GMP_DEFAULT)))
			retval = 0;
	    p_prt.Release();
		CoUninitialize();
	}
    return retval;
}


// This function was copied from ehandler.cpp
// Print the specified FILETIME to output in a human readable format,
// without using the C run time.

void PrintTime(char *output, FILETIME TimeToPrint)
{
	WORD Date, Time;
	if (FileTimeToLocalFileTime(&TimeToPrint, &TimeToPrint) &&
				FileTimeToDosDateTime(&TimeToPrint, &Date, &Time))
	{
		// What a silly way to print out the file date/time. Oh well,
		// it works, and I'm not aware of a cleaner way to do it.
		if (g_bBriefHTML)
		wsprintf(output, "%d-%02d-%02d_%02d%02d%02d",
					(Date / 512) + 1980, (Date / 32) & 15, Date & 31, 
					(Time / 2048), (Time / 32) & 63, (Time & 31) * 2);
		else
		wsprintf(output, "%d/%d/%d %02d:%02d:%02d",
					(Date / 32) & 15, Date & 31, (Date / 512) + 1980,
					(Time / 2048), (Time / 32) & 63, (Time & 31) * 2);
	}
	else
		output[0] = 0;
}


int WriteBriefingToFile(_TCHAR *string, char *fname)
{
	int retval = 1;
	ShiAssert(IsBadStringPtr(string, 8192) == 0);
	char	fullname[MAX_PATH];
	HANDLE	fileID;
	extern char FalconDataDirectory[_MAX_PATH];
	unsigned long bytes, strsize;
	char tmpString[255];

	FILETIME	CurrentTime;
	GetSystemTimeAsFileTime(&CurrentTime);
	char TimeBuffer[100];
	PrintTime(TimeBuffer, CurrentTime);

	if (g_bBriefHTML)
		sprintf (fullname, "%s\\Briefings\\%s_%s", FalconDataDirectory, TimeBuffer, fname);
	else
		sprintf (fullname, "%s\\%s", FalconDataDirectory, fname);

//It might be better for briefing processing programs to always have only one
//briefing in the file...Make it configurable.

	if (g_bAppendToBriefingFile && !g_bBriefHTML)	//No sense in appending HTML briefings
	{
		fileID = CreateFile( fullname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL );
		if( fileID == INVALID_HANDLE_VALUE ) {
			MonoPrint ("Failed to open briefing output file\n");
			return 0;
		}
		SetFilePointer(fileID, 0, 0, FILE_END);
	}
	else
	{
	    fileID = CreateFile( fullname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, NULL );
		if( fileID == INVALID_HANDLE_VALUE ) {
			MonoPrint ("Failed to open briefing output file\n");
			return 0;
		}
	}

	if (g_bBriefHTML)
		strsize = sprintf(tmpString, "<html><head><title>Falcon 4 Mission Briefing</title><LINK REL=StyleSheet HREF='style.css' TYPE='text/css' MEDIA=screen></head><body>");
	else
		strsize = sprintf(tmpString, "--------------------------------------------------------\r\nBRIEFING RECORD ");
	WriteFile( fileID, tmpString, strsize, &bytes, NULL );

	if (!g_bBriefHTML)
	{
		strsize = sprintf(tmpString, "generated at %s.\r\n", TimeBuffer);
		WriteFile( fileID, tmpString, strsize, &bytes, NULL );
	}
     // loop printing each line in turn. Add \r\n at the end of each line instead of just \n.
     for (_TCHAR *cp = string; cp; )
	{
		_TCHAR *dp = strchr(cp, '\n');
		_TCHAR *from = cp;
		unsigned long size;
		if (dp)
		{
			size = dp - cp;
			dp++;
		}
		else
		{
			size = strchr(cp, '\000') - cp;
		}
		//Write to file and append "\r\n" sequence.
		if ( !WriteFile( fileID, from, size, &bytes, NULL ) ) bytes=-1;
		if ( bytes != size ) {
			MonoPrint ("Failed to write to briefing output file\n");
			CloseHandle( fileID );
			return 0;
		}
		strsize = sprintf(tmpString, "\r\n");
		WriteFile( fileID, tmpString, strsize, &bytes, NULL );
		cp = dp;
     }
	if (g_bBriefHTML)
		strsize = sprintf(tmpString, "</body>");
	else
		strsize = sprintf(tmpString, "END_OF_BRIEFING\r\n\r\n");

	WriteFile( fileID, tmpString, strsize, &bytes, NULL );

	CloseHandle( fileID );

	return retval;
}

