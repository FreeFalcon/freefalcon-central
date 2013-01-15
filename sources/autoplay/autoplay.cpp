/*****************************************************************************************\
	Auto play front end for Falcon 4.0

	Scott Randolph
	January 29, 1998
	Microprose

  Assumptions:
    The Disk ID and Setup program are in the root directory of the CD
	The Disk ID files are named "DISK.ID"
	The disk id file on the CD will be copied to the HDD at install time for version comparison
	The file names in the list of global strings below is correct.
	The WindowTitle string exactly matches the title of the main window of the game.

\*****************************************************************************************/
#include <stdio.h>
#include <windows.h>
#include "resource.h"


// Global strings used to centralize filename and arugment dependencies
char	*disk_id		= "DISK.ID";
char	*executable		= "FALCON4.EXE";
char	*installer		= "SETUP.EXE";
char	*WindowTitle	= "Falcon 4.0";


// String array used to decend the registry tree to find our one entry
// (NOTE:  We always start from HKEY_LOCAL_MACHINE)
char	*keyStrings[] = { "Software", "Microprose", "Falcon", "4.0" };
char	*dirString      = "baseDir";
char	*disableString  = "disableAutoplay";


// Global resource used to ensure a single instance of this application
ATOM	exclusionAtom;


// Used to tell the dialog box if it should offer "Play" as an option
BOOL	GamePlayable;

// Used to store the path to the installed game (if available)
char	TargetDir[_MAX_PATH];


// Convert a windows error number into a printable string
#define PutErrorString(buf)  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,					\
                                           NULL, GetLastError(),       					\
                                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	\
                                           buf, sizeof(buf), NULL)


//
// Function to lookup the Game's path in the system registry.
//
BOOL GetKeyHandle( HKEY *pKey )
{   
	HKEY		child;
	HKEY		parent;
	
	LONG		retval;


	parent = HKEY_LOCAL_MACHINE;


	// One iteration for each entry in the key tree array
	for (int i = 0; i < (sizeof(keyStrings)/sizeof(char*)); i++) {

		retval = RegOpenKeyEx( parent, keyStrings[i], 0, KEY_ALL_ACCESS, &child );

		// We're done with the parent, so close it
		RegCloseKey( parent );

		// See if we got what we asked for
		if (retval != ERROR_SUCCESS) {

			// We couldn't open the key we wanted, so return failure
			return FALSE;

		}

		// Use this child as the parent for the next iteration
		parent = child;
	}

	*pKey = child;

	return TRUE;
}



//
// Function to create an etnry for the Game's path in the system registry.
//
BOOL CreateKeyHandle( HKEY *pKey )
{   
	HKEY		child;
	HKEY		parent;
	
	LONG		retval;
	DWORD		result;

	parent = HKEY_LOCAL_MACHINE;


	// One iteration for each entry in the key tree array
	for (int i = 0; i < (sizeof( keyStrings )>>2); i++) {

		retval = RegCreateKeyEx( parent, keyStrings[i], 
								 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
								 &child, &result );

		// We're done with the parent, so close it
		RegCloseKey( parent );

		// See if we got what we asked for
		if (retval != ERROR_SUCCESS) {

			// We couldn't open OR create the key we wanted, so return failure
			return FALSE;

		}

		// Use this child as the parent for the next iteration
		parent = child;
	}

	*pKey = child;

	return TRUE;
}



//
//	Compare the ID file on HDD with that on the CD and
//  return a value indicating their relationship
//
BOOL MatchDiskID( char *HDDpath )
{
	FILE	*HDDfile;
	FILE	*CDfile;

	char	HDDfilename[MAX_PATH];

	char	HDDverString[256];
	char	HDDid[256];

	char	CDid[256];
 	char	CDverString[256];

	char	*c;
	int		result;


	// Construct the fully qualified pathname for the id file of interest
	strcpy( HDDfilename, HDDpath );
	if (HDDfilename[strlen(HDDfilename)-1] != '\\') {
		strcat( HDDfilename, "\\" );
	}
	strcat( HDDfilename, disk_id );

	// Open and read the id file on the hard disk
    HDDfile = fopen( HDDfilename, "r" );
    if (!HDDfile) {
		return FALSE;
	}
	c = fgets( HDDverString, sizeof(HDDverString), HDDfile );
	c = fgets( HDDid, sizeof(HDDid), HDDfile );
	fclose( HDDfile );
	if ( c == NULL ) {
		return FALSE;
	}


    // Open and read the id file on the CD
    CDfile = fopen( disk_id, "r" );
    if (!CDfile) {
		return FALSE;
	}
	
	c = fgets( CDverString, sizeof(CDverString), CDfile );
	c = fgets( CDid, sizeof(CDid), CDfile );
	fclose( CDfile );
	if ( c == NULL ) {
		return FALSE;
	}


	// Compare the versions represented by the two files
	result = strcmp( HDDverString, CDverString );
	if (result != 0) {
		return FALSE;
	}
	result = strcmp( HDDid, CDid );
	if (result != 0) {
		return FALSE;
	}

	return TRUE;
}


//
//	The dialog procedure used to ask the user if they want to install, disable, or cancel
//
BOOL CALLBACK DialogProcedure( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam )
{
	HWND	control;

	switch (uMsg) {

	  case WM_INITDIALOG:
		control = GetDlgItem( hwndDlg, IDC_PLAY );
		if (GamePlayable) {
			EnableWindow( control, TRUE );
		} else {
			EnableWindow( control, FALSE );
		}
		return 1;

	  case WM_COMMAND:
		EndDialog( hwndDlg, wParam );
		return 1;

	}

	return 0;
}


//
//	 See if we think the game is already installed and playable.  Return FALSE if it
//   couldn't be found.
//
BOOL Validate()
{
	HKEY	key;
	DWORD	targetDirLen;
	DWORD	retval;


	// Get the game's path entry from the registry (if its there)
	if ( GetKeyHandle( &key ) ) {

		// We got the key handle, so now get the key value
		targetDirLen = sizeof(TargetDir);
		retval = RegQueryValueEx( key, dirString, NULL, NULL, (unsigned char*)TargetDir, &targetDirLen );
		if (retval == ERROR_SUCCESS) {

			// We got the path, so compare the versions
			if ( MatchDiskID( TargetDir ) ) {
				return TRUE;
			}
		}
	}

	TargetDir[0] = '\0';
	return FALSE;
}


//
//	Call the setup program
//
BOOL Install()
{
	char					fullPath[_MAX_PATH];
	PROCESS_INFORMATION		procInfo;
	STARTUPINFO				startInfo;


	// Construct a fully qualifed path to the setup program
	GetCurrentDirectory( sizeof(fullPath), fullPath );
	if (fullPath[strlen(fullPath)-1] != '\\') {
		strcat( fullPath, "\\" );
	}
	strcat( fullPath, installer );


	// Get this process's startup info to be used for our child
	GetStartupInfo( &startInfo );

	// Run the installer program
	if ( !CreateProcess( NULL, fullPath, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo) ) {

		char string[256];
		PutErrorString( string );
		MessageBox( NULL, string, "Error Launching the Installer", MB_OK );
		return FALSE;

	}

	// Wait for the installer to finish
	WaitForSingleObject( procInfo.hProcess, INFINITE );

	// TODO:  Check registry here???

	return TRUE;
}



//
//	Call the game program
//
BOOL Play()
{
	PROCESS_INFORMATION		procInfo;
	STARTUPINFO				startInfo;
	char					fullPath[_MAX_PATH];


	// Make sure we HAVE a path to the installed game
	if ((!GamePlayable) || (TargetDir[0] == '\0')) {
		return FALSE;
	}

	// Construct the fully qualified path to the game executable
	strcpy( fullPath, TargetDir );
	if (TargetDir[strlen(TargetDir)-1] != '\\') {
		strcat( fullPath, "\\" );
	}
	strcat( fullPath, executable );
	
	// Get this process's startup info to be used for our child
	GetStartupInfo( &startInfo );

	// Run the installer program
	if ( !CreateProcess( NULL, fullPath, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo) ) {

		char string[256];
		PutErrorString( string );
		MessageBox( NULL, string, "Error Launching the Game", MB_OK );
		return FALSE;

	}

	// We're done!
	return TRUE;
}



//
//	Callback function to handel each window we are told about
//
BOOL CALLBACK EnumWindowsProc( HWND win, BOOL* found )
{
	char title[256];

	// Get the title of this window	and see if it matches our magic value
	if ( GetWindowText( win, title, sizeof(title)-1 ) > 0) {
		if (strncmp( title, WindowTitle, strlen(WindowTitle) ) == 0) {
			*found = TRUE;
			return FALSE;
		}
	}

	// Continue enumerating the rest of the windows
	return TRUE;
}


//
//	Ensure that this AutoPlay application or the game is not already running
//
BOOL EnsureExclusion( void )
{
	BOOL GameRunning = FALSE;
	
	
	// Make sure we're the only copy running by checking for an entry in the global atom table
	exclusionAtom = GlobalFindAtom( WindowTitle );
	if ( exclusionAtom ) {
		// The global atom already exists, which means we're already running elsewhere
		return FALSE;
	}


	// Now make sure no copy of the game is already running
	EnumWindows( (WNDENUMPROC)EnumWindowsProc, (LPARAM)&GameRunning );
	if (GameRunning) {
		// We found a window with the magic title, so we won't run
		return FALSE;
	}


	// We're the first, so create the global atom to dissuade any others from following...
	// NOTE:  These is a potential race condition here.  If two copies of the app hit FindAtom
	//		  before either hit AddAtom, we'de end up with two copies running.
	exclusionAtom = GlobalAddAtom( WindowTitle );
	if ( !exclusionAtom ) {
		MessageBox( NULL, "We had trouble ensuring that only one copy of the autoloader was running", "Error", MB_OK );
	}

	return TRUE;
}


//
//  Release the global system resource used to ensure only one instance of
//  the AutoRun application was active.
//
void ReleaseExclusion( void )
{
	// Release the global atom to allow a future instance of autoloader to run
	GlobalDeleteAtom( exclusionAtom );
}



/*
 * Main auto play routine.  Checks registry and disk id versions to decide if we should
 * launch the game or run the installer.
 */
int PASCAL WinMain (HANDLE this_inst, HANDLE prev_inst, LPSTR cmdline, int cmdshow)
{
	int		done;
	int		retval;


	// Quit if the game or this AutoRun app is already running
	if ( !EnsureExclusion() ) {
		return 0;
	}


	done = 0;
	do {
		
		// See if we can offer the "play" option
		GamePlayable = Validate();


		// Put up a three choice dialog box:  Play, Setup, Cancel
		retval = DialogBox( this_inst, MAKEINTRESOURCE( IDD_INSTALL_OPTIONS ), NULL, (DLGPROC)DialogProcedure );
		if (retval == -1) {
			done = 2;
			break;
		}


		switch ( retval ) {

		  case IDC_PLAY:
			if ( Play() )		done = 3;
			break;

		  case IDC_SETUP:
			if ( Install() )	done = 4;
			break;

		  case IDCANCEL:
			done = 5;
			break;

		}

	} while ( !done );


	// Release our lock to allow a future instance of autoloader to run
	ReleaseExclusion();


	return done;	
}

