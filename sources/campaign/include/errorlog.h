// ======================
// Error Reporting System
// ======================

// -----------------
// User Descriptions
// -----------------

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void InitSystemErrors (char* LogFileName, Boolean ImmediateReports) บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Initializes the Error Reporting System. Errors can be directed to   บ
   บ either a file or the screen by specifying either the file name or   บ
   บ the defined value "LogErrorsToScreen" as the "LogFileName" parameterบ
   บ (repectively). The "ImmediateReports" parameter determines whether  บ
   บ Errors are immediately reported to file/screen upon receipt (i.e.   บ
   บ ImmediateReports = TRUE) or whether they are queued until a call to บ
   บ "ProcessSystemErrors" is made (i.e. ImmediateReports = FALSE). The  บ
   บ mnemonics "ImmediateErrors" (equal to TRUE) and "QueueErrors"       บ
   บ (equal to FALSE) are provided for code readability.                 บ
   บ                                                                     บ
   บ Example calls:                                                      บ
   บ           InitSystemErrors(LogErrorsToScreen,QueueErrors)           บ
   บ           InitSystemErrors("ErrorLog.Dmp",ImmediateErrors)          บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void ReportSystemError (SystemErrorCode C, char* ErrorText)         บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Allows an Error Code to be reported, along with a text information  บ
   บ string. The SystemErrorCode type is enumerated and redefinable on a บ
   บ project by project basis. However, many of the more generically     บ
   บ defined codes are required by existing library routines.            บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void ProcessSystemErrors (void)                                     บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ If System Errors are being queued (see InitSystemErrors) then a     บ
   บ call to ProcessSystemErrors will cause all queued Error Reports to  บ
   บ be processed and sent to File/Screen. If System Errors are not      บ
   บ being queued (i.e. they are being reported immediately) then a call บ
   บ to this routine accomplishes nothing.                               บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

   #define LogErrorsToScreen "ScreenErrors"
   #define ImmediateErrors   TRUE
   #define QueueErrors       FALSE

   typedef enum {ErrorReportingError,
                 ProgramInitFailure,
                 SystemInitFailure,
                 ModuleInitFailure,
                 FileOpenFailure,
                 ADTUseageError,
                 SystemLevelFailure,
                 SystemUseageError,
                 ModuleLevelFailure,
                 ModuleUseageError
                 }
                 SystemErrorCode;

   extern void InitSystemErrors (char* LogFileName, unsigned char ImmediateReports);

   extern void ReportSystemError (SystemErrorCode C, char* ErrorText);

   extern void ProcessSystemErrors (void);
