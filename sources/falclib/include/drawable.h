#ifndef _DRAWABLE_H
#define _DRAWABLE_H

class VirtualDisplay;
class RViewPoint;
class ImageBuffer;

class DrawableClass
{
   protected:
      enum DrawableFlags {SOI = 0x1};
      DrawableClass(void) {privateDisplay = display = NULL; viewPoint = NULL; drawFlags = 0;};
      int MFDOn;
      int drawFlags;

   public:
      virtual ~DrawableClass (void)				{};

      enum DisplayTypes {ThreeDIR, ThreeDVis, ThreeDColor, MonoChrome, NumDisplayTypes};

      virtual void Display(VirtualDisplay*)		{ ShiWarning( "No Display!" ); };
      virtual void DisplayInit (ImageBuffer*)	{};
      virtual void DisplayExit (void);
      virtual VirtualDisplay* GetDisplay (void)	{return privateDisplay;};

      int IsSOI (void) {return (drawFlags & SOI ? TRUE : FALSE);};
      void SetSOI (int newVal) {if (newVal) drawFlags |= SOI; else drawFlags &= ~SOI; };

      void	SetMFD (int newMFD)		{MFDOn = newMFD;};
      int	OnMFD (void)			{return MFDOn;};

      void LabelButton (int idx, char* str1, char* str2 = NULL, int inverse = 0);	// Last argument tells if its an INVERSE label or not...
      virtual void PushButton (int, int)	{};								// Override to get button messages in subclasses

      RViewPoint		*viewPoint;
      VirtualDisplay	*display;
      VirtualDisplay	*privateDisplay;
};
#endif
