/***************************************************************************\
    Device.h
    Scott Randolph
    November 12, 1996

    This class provides management of a single drawing device in a system.
\***************************************************************************/
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "ImageBuf.h"

class DisplayDevice
{
  public:
	DisplayDevice();
	~DisplayDevice();

	void			Setup( int driverNum, int devNum, int width, int height, int nDepth, BOOL fullScreen, BOOL dblBuffer, HWND win, BOOL bWillCallSwapBuffer);
	void			Cleanup( void );

	BOOL			IsReady( void )					{ return (m_DXCtx != NULL); };

	BOOL			IsHardware( void )				{ ShiAssert(IsReady());  return m_DXCtx->m_eDeviceCategory > DXContext::D3DDeviceCategory_Software; };

	HWND			GetAppWin( void )				{ return appWin; };
	IDirectDraw7 *GetMPRdevice( void )			{ return m_DXCtx->m_pDD; };
	ImageBuffer*	GetImageBuffer( void )			{ return &image; };
	DXContext *GetDefaultRC( void )			{ return m_DXCtx; };

  protected:
	DXContext *m_DXCtx;

	HWND						appWin;
	BOOL						privateWindow;
	ImageBuffer					image;
	int							driverNumber;

	DisplayDevice				*next;

  friend class DeviceManager;
};

#endif // _DEVICE_H_