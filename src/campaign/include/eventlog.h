// ======================
// Event Reporting Module
// ======================

// -----------------
// User Descriptions
// -----------------

// ***** PENDING *****

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

   #define LogEventsToScreen "ScreenEvents"

   typedef enum {EventUnknown,
                 ObjectiveCaptured,
                 RequestForCAS,
                 }
                 CampaignEventCode;

   extern void InitCampaignEvents (char* LogFileName);

   extern void ReportCampaignEvent (CampaignEventCode C, char* EventText);

   extern void ProcessCampaignEvents (void);

