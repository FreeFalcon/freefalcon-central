/***************************************************************************\
    DevMgr.h
    Scott Randolph
    November 8, 1996

    This class provides management of the drawing devices in the system.
\***************************************************************************/
#ifndef _DEVMGR_H_
#define _DEVMGR_H_

#include "Device.h"

#include <string>
#include <vector>
#include <ddraw.h>
#include <d3d.h>

class DeviceManager
{
  public:
	DeviceManager()		{ ready = FALSE; };
	~DeviceManager()	{ ShiAssert( !IsReady() ); };

	// OW
	//////////////////

	// DDRAW related
	class DDDriverInfo
	{
		public:
		DDDriverInfo(GUID guid, LPCTSTR Name, LPCTSTR Description);

		GUID m_guid;
		std::string m_strName;
		std::string m_strDescription;

		// D3D related 
		class D3DDeviceInfo
		{
			public:
			D3DDeviceInfo(D3DDEVICEDESC7 &devDesc, LPSTR lpDeviceName, LPSTR lpDeviceDescription);

			D3DDEVICEDESC7 m_devDesc;
			std::string m_strName;
			std::string m_strDescription;

			const char *GetName() { return m_strName.c_str(); }
			LPGUID GetGuid() { return &m_devDesc.deviceGUID; }
			bool IsHardware();
			bool CanFilterAnisotropic();
		};

		DDCAPS m_caps;	// HAL caps
		typedef std::vector<D3DDeviceInfo> D3DDEV_ARRAY;
		D3DDEV_ARRAY m_arrD3DDevices;
		typedef std::vector<DDSURFACEDESC2> SURFACEDESC_ARRAY;
		SURFACEDESC_ARRAY m_arrModes;
		DDDEVICEIDENTIFIER2 devID;

		void EnumD3DDrivers();
		static HRESULT _stdcall CALLBACK EnumD3DDriversCallback(LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpD3DHWDeviceDesc, LPVOID lpContext);
		static HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);

		const char *GetName() { return m_strDescription.c_str(); }
		const char *GetDeviceName(int n);
		LPDDSURFACEDESC2 GetDisplayMode(int n);
		bool CanRenderWindowed();
		D3DDeviceInfo *GetDevice(int n);
		LPGUID GetGuid() { return &m_guid; }
		int FindDisplayMode(int nWidth, int nHeight, int nBPP);
		bool SupportsSRT();
		int FindRGBRenderer();
		bool Is3dfx();
	};

	typedef std::vector<DDDriverInfo> DDDRVINFO_ARRAY;
	DDDRVINFO_ARRAY m_arrDDDrivers;

	protected:
	BOOL			ready;

	public:
	void Setup( int languageNum = 0 );
	void Cleanup( void );
	BOOL IsReady( void ) { return (ready); };
	DDDriverInfo *GetDriver(int driverNum);
	const char *GetDriverName(int driverNum );
	const char *GetDeviceName(int driverNum, int devNum);
	const char *GetModeName(int driverNum, int devNum, int modeNum);
	BOOL ChooseDevice( int *drvNum, int *devNum, int *width );
	int FindPrimaryDisplayDriver();

	// OW
	void EnumDDDrivers(DeviceManager *pThis);
	static BOOL WINAPI EnumDDCallback(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
	static BOOL WINAPI EnumDDCallbackEx(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm);
	static HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);
	bool GetMode(int driverNum, int devNum, int modeNum, UINT *pWidth, UINT *pHeight, UINT *pDepth);
	DXContext *CreateContext(int driverNum, int devNum, int resNum, BOOL fullScreen, HWND hWnd);
};

#endif // _DEVMGR_H_