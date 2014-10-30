/***************************************************************************\
    Mono2D.cpp
    Scott Randolph
    February 5, 1996

    This class provides 2D drawing functions for a Hercules monochrome
 display.
\***************************************************************************/
#include <cISO646>
#include <conio.h>
#include <math.h>
#include "Mono2D.h"


/***************************************************************************\
 Constants which are used to manipulate the hardware
\***************************************************************************/
#define CONTROL_REG 0x3B8
#define CONFIG_REG 0x3BF
#define STATUS_REG 0x3BA
#define DATA_REG 0x3B5
#define ADDRESS_REG 0x3B4
#define FRAME_BUFFER_ADDR 0xB0000

// Hardware intialization settings
static int graph_settings[] = { 53, 45, 46,  7, 91, 2, 87, 87, 2,  3,  0,  0, 0, 0, 0, 0 };
static int text_settings[]  = { 97, 80, 82, 15, 25, 6, 25, 25, 2, 13, 11, 12, 0, 0, 0, 0 };

// Macros for writing data to hardware I/O ports
#ifdef _MSC_VER
#define OUT_BYTE(a, b) _outp((a), (b))
#define OUT_WORD(a, b) _outpw ((a), (b))
#else
#define OUT_BYTE(a, b) outp((a), (b))
#define OUT_WORD(a, b) outpw ((a), (b))
#endif

// Offscreen drawing buffer management stuff
static char *screen_buffer[2];
static int page = 0;
static int monoDisplayCount = 0;
static BOOL enabled = FALSE;


/***************************************************************************\
 Setup hardware for graphics display
\***************************************************************************/
void MonochromeDisplay::Setup(void)
{

    // Set the base class's member variables
    xRes = 720;
    yRes = 348;

    // Call our base classes setup function (must come AFTER xRes and Yres have been set)
    VirtualDisplay::Setup();


    // Increment the global reference count
    // (Should probably use InterlockedIncrement() here, but for now we'll assume
    //  no two renderers will be setup on different threads at the same time)
    monoDisplayCount++;

    // If we're the first instance, setup the shared resources
    if (monoDisplayCount == 1)
    {

        // Allocate memory for our offscreen buffers
        screen_buffer[0] = new char[ 0x8000 ];
        screen_buffer[1] = new char[ 0x8000 ];

        // Clear both of the drawing buffers
        page = 0;
        ClearDraw();
        page = 1;
        ClearDraw();
    }
}



/***************************************************************************\
    Shutdown the display.
\***************************************************************************/
void MonochromeDisplay::Cleanup(void)
{
    // Decrement the global reference count
    // (Should probably use InterlockedDecrement() here, but for now we'll assume
    //  no two renderers will be cleaned up on different threads at the same time)
    ShiAssert(monoDisplayCount > 0);
    monoDisplayCount--;

    // If we're the last instance, release the shared resources
    if (monoDisplayCount == 0)
    {
        // Release our off screen buffers
        ShiAssert(screen_buffer[0]);
        delete[] screen_buffer[0];
        screen_buffer[0] = NULL;

        ShiAssert(screen_buffer[1]);
        delete[] screen_buffer[1];
        screen_buffer[1] = NULL;

        // Reset to text mode
        if (enabled)
        {
            OUT_BYTE(CONFIG_REG, 0x0);

            for (int i = 0; i < 16; i++)
            {
                OUT_WORD(ADDRESS_REG, (short)((text_settings[i] << 8) + i));
            }

            OUT_BYTE(CONTROL_REG, 0x8);
            memset((void *)FRAME_BUFFER_ADDR, 0, (160 * 25));
        }
    }

    // Call the base class's cleanup routine
    VirtualDisplay::Cleanup();
}



/***************************************************************************\
    Clear the display to black.
\***************************************************************************/
void MonochromeDisplay::ClearDraw(void)
{
    if (enabled)
    {
        // Set our drawing buffer to black (won't hit display until next FinishFrame)
        memset((void *)screen_buffer[page], 0, 0x8000);
    }
}



/***************************************************************************\
   Copy the drawing buffer to the display.
\***************************************************************************/
void MonochromeDisplay::EndDraw(void)
{
    if (enabled)
    {
        // For each byte in the drawing buffer
        for (int i = 0; i < 0x8000; i++)
        {

            // Copy only those bytes which have changed (since display is accross the slow ISA bus)
            if (*((char *)(screen_buffer[page] + i)) not_eq *((char *)(screen_buffer[1 - page] + i)))
            {
                *((char *)(FRAME_BUFFER_ADDR + i)) = *((char *)(screen_buffer[page] + i));
            }
        }
    }

    // Switch to the alternate draw buffer
    page = 1 - page;
}



/***************************************************************************\
 Put a pixel on the display.
\***************************************************************************/
void MonochromeDisplay::Render2DPoint(float x, float y)
{
    int x1, y1;
    int the_byte;
    int the_bit;
    char *currentValue;

    x1 = FloatToInt32(x);
    y1 = FloatToInt32(y);

    the_byte = 0x2000 * (y1 bitand 0x3) + 90 * (y1 >> 2) + (x1 >> 3);
    the_bit  = 7 - (x1 bitand 0x7);

    currentValue   = (char *)(screen_buffer[page] + the_byte);
    *currentValue or_eq (char)(1 << the_bit);
}



/***************************************************************************\
 Put a pixel on the display.
\***************************************************************************/
void MonochromeDisplay::Render2DPoint(int x1, int y1)
{
    int the_byte;
    int the_bit;
    char *currentValue;

    the_byte = 0x2000 * (y1 bitand 0x3) + 90 * (y1 >> 2) + (x1 >> 3);
    the_bit  = 7 - (x1 bitand 0x7);

    currentValue   = (char *)(screen_buffer[page] + the_byte);
    *currentValue or_eq (char)(1 << the_bit);
}



/***************************************************************************\
 Put a straight line on the display.
\***************************************************************************/
void MonochromeDisplay::Render2DLine(float x0in, float y0in, float x1in, float y1in)
{
    int x0, y0, x1, y1;
    int dx, dy, ince, incne, d, x, y, pixcount;


    // Swap the endpoint as necessary to get x0 left of x1
    if (x0in <= x1in)
    {
        x0 = FloatToInt32(x0in);
        y0 = FloatToInt32(y0in);
        x1 = FloatToInt32(x1in);
        y1 = FloatToInt32(y1in);
    }
    else
    {
        x0 = FloatToInt32(x1in);
        y0 = FloatToInt32(y1in);
        x1 = FloatToInt32(x0in);
        y1 = FloatToInt32(y0in);
    }

    // Setup for our forward differencing
    dx = x1 - x0;
    dy = y1 - y0;
    x = x0;
    y = y0;
    pixcount = 0;

    // Select which of four possible octants we're in (0, 1, 6, or 7)
    if (dy >= 0 and dy <= dx)
    {
        d = 2 * dy - dx;
        ince = 2 * dy;
        incne = 2 * (dy - dx);
        MonochromeDisplay::Render2DPoint(x, y);

        while (x < x1)
        {
            if (d <= 0)
            {
                d += ince;
                x ++;
            }
            else
            {
                d += incne;
                x ++;
                y ++;
            }

            MonochromeDisplay::Render2DPoint(x, y);
            pixcount ++;
        }
    }
    else if (dy < 0 and -dy < dx)
    {
        d = -2 * dy - dx;
        ince = -2 * dy;
        incne = 2 * (-dy - dx);

        while (x < x1)
        {
            if (d <= 0)
            {
                d += ince;
                x ++;
            }
            else
            {
                d += incne;
                x ++;
                y --;
            }

            MonochromeDisplay::Render2DPoint(x, y);
            pixcount ++;
        }
    }
    else if (dx >= 0 and dy >= 0)
    {
        d = 2 * dx - dy;
        ince = 2 * dx;
        incne = 2 * (dx - dy);
        MonochromeDisplay::Render2DPoint(x, y);

        while (y < y1)
        {
            if (d <= 0)
            {
                d += ince;
                y ++;
            }
            else
            {
                x ++;
                d += incne;
                y ++;
            }

            MonochromeDisplay::Render2DPoint(x, y);
            pixcount ++;
        }
    }
    else
    {
        d = 2 * dx + dy;
        ince = 2 * dx;
        incne = 2 * (dx + dy);
        MonochromeDisplay::Render2DPoint(x, y);

        while (y > y1)
        {
            if (d <= 0)
            {
                d += ince;
                y --;
            }
            else
            {
                d += incne;
                y --;
                x ++;
            }

            MonochromeDisplay::Render2DPoint(x, y);
            pixcount ++;
        }
    }
}



//
// Use these with caution -- they will disrupt normal use of the graphics function above
//

/***************************************************************************\
 Put the hardware into text only mode ( 8 x 25 characters )
\***************************************************************************/
void MonochromeDisplay::EnterTextOnlyMode(void)
{
    if (enabled)
    {
        // Put the hardware in text mode
        OUT_BYTE(CONFIG_REG, 0x0);

        for (int i = 0; i < 16; i++)
        {
            OUT_WORD(ADDRESS_REG, (short)((text_settings[i] << 8) + i));
        }

        OUT_BYTE(CONTROL_REG, 0x8);
    }

    // Clear the display surface
    ClearTextOnly();
}



/***************************************************************************\
 Put the display back into the expected graphics mode
\***************************************************************************/
void MonochromeDisplay::LeaveTextOnlyMode(void)
{
    // Clear both of the drawing buffers
    page = 0;
    ClearDraw();
    page = 1;
    ClearDraw();

    if (enabled)
    {
        // Select graphics mode
        OUT_BYTE(CONFIG_REG,  0x3);

        for (int i = 0; i < 16; i++)
        {
            OUT_WORD(ADDRESS_REG, (short)((graph_settings[i] << 8) + i));
        }

        OUT_BYTE(CONTROL_REG, 0xe);

        // Clear the surface of the display
        for (int i = 0; i < 0x8000; i++)
        {
            *((char *)(FRAME_BUFFER_ADDR + i)) = 0;
        }
    }
}



/***************************************************************************\
 Clear the display while in text only mode
\***************************************************************************/
void MonochromeDisplay::ClearTextOnly(void)
{
    if (enabled)
    {
        memset((void *)FRAME_BUFFER_ADDR, 0, (160 * 25));
    }

    textX = 0;
    textY = 0;
}



/***************************************************************************\
 Put a string on the display in a fashion similar to "printf()"
\***************************************************************************/
void MonochromeDisplay::PrintTextOnly(char *string, ...)
{
    va_list params;   /* watcom manual 'Library' p.470 */
    unsigned char *addr;
    int i = 0;
    int check;
    static char output_buffer[120];

    // Process the variable length parameter list to get a single string
    va_start(params, string);
    check = vsprintf(output_buffer, string, params);
    va_end(params);
    ShiAssert(check < sizeof(output_buffer));


    // Get the address at which to start writing the string to the display
    addr = (unsigned char *)(textY * 160 + textX * 2 + FRAME_BUFFER_ADDR);

    if (enabled)
    {
        // Write each character directly to the frame buffer
        while (output_buffer[i])
        {
            if (output_buffer[i] == '\n')
            {
                addr = NewLineTextOnly();
                i++;
            }
            else
            {
                *(addr++) = output_buffer[i++]; // Character code
                *(addr++) = (unsigned char)0x7; // Pen attribute
            }

            if ((textX++) >= 79)
            {
                addr = NewLineTextOnly();
            }
        }
    }
}



/***************************************************************************\
 Do the processing for a line wrap in text only mode (includes scrolling)
\***************************************************************************/
unsigned char * MonochromeDisplay::NewLineTextOnly(void)
{
    // Update the current text output location
    textX = 0;
    textY++;

    // Scroll the display if we've falling off the bottom
    if (textY > 24)
    {

        if (enabled)
        {
            // Copy the lower 24 lines of the display up one line
            memmove((void *)FRAME_BUFFER_ADDR, (void *)(FRAME_BUFFER_ADDR + 160), (160 * 24));

            // Clear the bottom (25th) line of the display
            memset((void *)(FRAME_BUFFER_ADDR + (160 * 24)), 0, 160);
        }

        // Reset the current line to the bottom of the screen
        textY = 24;
    }

    // Return the address of the new "current point" for text to be written upon
    return((unsigned char *)(textY * 160 + textX * 2 + FRAME_BUFFER_ADDR));
}

