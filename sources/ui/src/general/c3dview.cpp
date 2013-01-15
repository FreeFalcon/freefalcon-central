#include <windows.h>
#include "sim/include/phyconst.h"
#include "graphics/include/TimeMgr.h"
#include "graphics/include/imagebuf.h"
#include "graphics/include/renderow.h"
#include "graphics/include/RViewPnt.h"
#include "graphics/include/drawbsp.h"
#include "objectiv.h"
#include "dispcfg.h"
#include "classtbl.h"
#include "graphics/include/loader.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "playerop.h"
#include "TexBank.h"
#include "FalcLib\include\dispopts.h"

#include "Sim/Include/navsystem.h"		//Wombat778 11-3-2003
		
extern C_Handler *gMainHandler;

#include "Graphics/DXEngine/DXVBManager.h"
extern	bool g_bUse_DX_Engine;

extern bool g_bReconLatLong;			//Wombat778 11-3-2003
extern OBJECTINFO Recon;				//Wombat778 11-3-2003

//JAM 21Nov03
#include "RealWeather.h"

// From OTWDrive.cpp
void PositandOrientSetData (float x, float y, float z, float pitch, float roll, float yaw,
                            Tpoint* simView, Trotation* viewRotation);


BOOL C_3dViewer::Setup()
{
	BSPLIST *cur;
	// COBRA - DX - Switching btw Old and New Engine - Initialize DX Engine and VB Manager
	if(g_bUse_DX_Engine)	TheVbManager.Setup((gMainHandler->GetFront())->GetDisplayDevice()->GetDefaultRC()->m_pD3D);

	if(objects_)
	{
		if(viewPoint_)
		{
			cur=objects_->GetFirst();
			while(cur)
			{
				if(((DrawableBSP*)cur->object)->InDisplayList())
					viewPoint_->RemoveObject(cur->object);
				cur=cur->Next;
			}
		}

		objects_->Cleanup();
	}
	else
		objects_=new C_BSPList;

	objects_->Setup();

	TheTimeManager.SetTime((unsigned long)(12l * 60l * 60l * 1000l));

	sw=(float)gMainHandler->GetFront()->targetXres();
	sh=(float)gMainHandler->GetFront()->targetYres();

	return(FALSE);
}

BOOL C_3dViewer::Init3d(float ViewAngle)
{
	if(rend3d_ || rendOTW_)
		return(FALSE);


	rend3d_=new Render3D;
	rend3d_->Setup(gMainHandler->GetFront());
	// The Near Z must be at least 10.0feet having a so tight angle
	rend3d_->SetFOV(ViewAngle * DTR, 10.0f);
	rend3d_->SetViewport(l,t,r,b);
	// Need to setup nearest Z up to 10.0 feet to compensate tight FOV angle
	rend3d_->SetFOV(ViewAngle * DTR, 10.0f);
	rend3d_->SetCamera(&currentPos_,&currentRot_);
//	rend3d_->SetTerrainTextureLevel( PlayerOptions.TextureLevel() );
//	rend3d_->SetSmoothShadingMode( PlayerOptions.ObjectShadingOn() );

//	rend3d_->SetHazeMode(PlayerOptions.HazingOn());
//	rend3d_->SetFilteringMode( PlayerOptions.FilteringOn() );
	rend3d_->SetObjectDetail(PlayerOptions.ObjectDetailLevel() );
//	rend3d_->SetAlphaMode(PlayerOptions.AlphaOn());
	rend3d_->SetObjectTextureState(TRUE);//PlayerOptions.ObjectTexturesOn());


	return(TRUE);
}

BOOL C_3dViewer::InitOTW(float,BOOL Preload)
{
	RViewPoint *tempVP=NULL;

	if(rend3d_ || rendOTW_)
		return(FALSE);

// This preloads a SMALL portion of the terrain for speedy viewing
	if(Preload)
	{
		tempVP=new RViewPoint;
		tempVP->Setup(2.0f * FEET_PER_KM,1,1,0.0f);

		tempVP->Update(&currentPos_);
		TheLoader.WaitLoader();
	}

	viewPoint_=new RViewPoint;
	rendOTW_=new RenderOTW;

	viewPoint_->Setup(ViewDistance_,MinTexture_,MaxTexture_,DisplayOptions.bZBuffering);
	rendOTW_->Setup(gMainHandler->GetFront(),viewPoint_);

	rendOTW_->SetViewport(l,t,r,b);

//	rendOTW_->SetTerrainTextureLevel(TextureLevel_);
//	rendOTW_->SetSmoothShadingMode(SmoothShading_);

	viewPoint_->Update(&currentPos_);

	if(Preload && tempVP)
	{
		tempVP->Cleanup();
		delete tempVP;
	}

	return(TRUE);
}

BOOL C_3dViewer::Cleanup()
{
	BSPLIST *cur;
	if(objects_)
	{
		if(viewPoint_)
		{
			cur=objects_->GetFirst();
			while(cur)
			{
				if(((DrawableBSP*)cur->object)->InDisplayList())
					viewPoint_->RemoveObject(cur->object);
				cur=cur->Next;
			}
		}
		objects_->Cleanup();
		delete objects_;
		objects_=NULL;
	}
	if(viewPoint_)
	{
		viewPoint_->Cleanup();
		delete viewPoint_;
		viewPoint_=NULL;
	}

	// OW
	if(m_pImgGray)
	{
		m_pImgGray->Cleanup();
		delete m_pImgGray;
		m_pImgGray = NULL;
	}

	if(rend3d_)
	{
		rend3d_->Cleanup();
		delete rend3d_;
		rend3d_=NULL;
	}
	if(rendOTW_)
	{
		rendOTW_->Cleanup();
		delete rendOTW_;
		rendOTW_=NULL;
	}
	//JAM 19Nov03
	if(realWeather)
		realWeather->Cleanup();

	ObjectLOD::ReleaseLodList();
	TheVbManager.Release();

	return(TRUE);
}

void C_3dViewer::Viewport(C_Window *win,long client)
{
	viewport.left=win->GetX()+win->ClientArea_[client].left;
	viewport.top=win->GetY()+win->ClientArea_[client].top;
	viewport.right=win->GetX()+win->ClientArea_[client].right;
	viewport.bottom=win->GetY()+win->ClientArea_[client].bottom;

	l=static_cast<float>(-1.0f+((float)(viewport.left) / (sw * .5)));
	t=static_cast<float>( 1.0f-((float)(viewport.top) / (sh * .5)));
	r=static_cast<float>( 1.0f-((float)(sw-viewport.right) / (sw * .5)));
	b=static_cast<float>(-1.0f+((float)(sh-viewport.bottom) / (sh * .5)));

	if(rend3d_)
		rend3d_->SetViewport(l,t,r,b);
	if(rendOTW_)
		rendOTW_->SetViewport(l,t,r,b);
}

BSPLIST *C_3dViewer::Load(long ID,int objID)
{
	BSPLIST *obj;

	obj=objects_->Load(ID,objID);
	if(obj)
	{
		objects_->Add(obj);
		return(obj);
	}
	return(NULL);
}

BSPLIST *C_3dViewer::LoadBSP(long ID,int objID,BOOL aircraft)
{
	BSPLIST *obj;

	obj=objects_->Load(ID,objID);
	if(obj)
	{
		if(aircraft) // turn on canopy
			((DrawableBSP*)obj->object)->SetSwitchMask(5, TRUE);

		objects_->Add(obj);
		return(obj);
	}
	return(NULL);
}

BSPLIST *C_3dViewer::LoadBuilding(long ID,int objID,Tpoint *pos,float heading)
{
	BSPLIST *obj;

	obj=objects_->LoadBuilding(ID,objID,pos,heading);
	if(obj)
	{
		objects_->Add(obj);
		return(obj);
	}
	return(NULL);
}

BSPLIST *C_3dViewer::LoadBridge(long ID,int objID)
{
	BSPLIST *obj;

	obj=objects_->LoadBridge(ID,objID);
	if(obj)
	{
		objects_->Add(obj);
		return(obj);
	}
	return(NULL);
}

BSPLIST *C_3dViewer::LoadDrawableFeature(long ID,Objective obj,short f,short fid,Falcon4EntityClassType *classPtr,FeatureClassDataType*	fc,Tpoint *objPos,BSPLIST *Parent)
{
	BSPLIST *bspobj;

	if(!Parent)
	{
		bspobj=objects_->CreateContainer(ID,obj,f,fid,classPtr,fc);
		if(bspobj)
		{
			objects_->Add(bspobj);
		}
		Parent=bspobj;
	}

	bspobj=objects_->LoadDrawableFeature(ID,obj,f,fid,classPtr,fc,objPos,Parent);
	if(bspobj)
	{
		objects_->Add(bspobj);
		return(bspobj);
	}
	return(NULL);
}

BSPLIST *C_3dViewer::LoadDrawableUnit(long ID,long visType,Tpoint *objPos,float facing,uchar domain,uchar type,uchar stype)
{
	BSPLIST *bspobj;

	bspobj=objects_->LoadDrawableUnit(ID,visType,objPos,facing,domain,type,stype);
	if(bspobj)
	{
		objects_->Add(bspobj);
		return(bspobj);
	}
	return(NULL);
}

BOOL C_3dViewer::Remove(long ID)
{
	objects_->Remove(ID);
	return(TRUE);
}

BOOL C_3dViewer::RemoveAll()
{
	objects_->RemoveAll();
	return(TRUE);
}

void C_3dViewer::SetPosition(float x,float y,float z)
{
	viewPos_.x = x;
	viewPos_.y = y;
	viewPos_.z = z;

	currentPos_.x = viewPos_.x + CameraPos_.x;
	currentPos_.y = viewPos_.y + CameraPos_.y;
	currentPos_.z = viewPos_.z + CameraPos_.z;
}

void C_3dViewer::SetCamera(float x,float y,float z,float heading,float Pitch,float roll)
{
	float tmpz;
	CameraPos_.x = x;
	CameraPos_.y = y;
	CameraPos_.z = z;
	CameraHeading_ = heading;
	CameraPitch_ = Pitch;
	CameraRoll_ = roll;
	if(rendOTW_) // Only care for Ground... not Models
	{
		tmpz=rendOTW_->viewpoint->GetGroundLevel( viewPos_.x + x, viewPos_.y + y );
		if((viewPos_.z + z) > tmpz)
		{
			z=tmpz - viewPos_.z - 10;
		}
	}
	PositandOrientSetData (viewPos_.x + x,viewPos_.y + y,viewPos_.z + z, Pitch * DTR, roll * DTR, heading * DTR, &currentPos_, &currentRot_);
}


BOOL C_3dViewer::AddToView(BSPLIST *obj)
{
	if(obj)
	{
		if(!((DrawableBSP*)obj->object)->InDisplayList())
		{
			viewPoint_->InsertObject(obj->object);
			return(TRUE);
		}
	}
	return(FALSE);
}

BOOL C_3dViewer::AddToView(long ID)
{
	return(AddToView(objects_->Find(ID)));
}

BOOL C_3dViewer::AddAllToView()
{
	BSPLIST *cur;

	cur=objects_->GetFirst();
	while(cur)
	{
		if(cur->type >= 0)
			AddToView(cur);
		cur=cur->Next;
	}
	return(TRUE);
}

BOOL C_3dViewer::View3d(long ID)
{
	BSPLIST *obj;

	if(rend3d_)
	{
		obj=objects_->Find(ID);
		if(obj)
		{
			gMainHandler->Unlock();
			rend3d_->SetCamera(&currentPos_,&currentRot_);
//			rend3d_->SetTime(Time_+(GetCurrentTime() % 60000l));

			//JAM 16Dec03
			if(DisplayOptions.bZBuffering)
				rend3d_->context.SetZBuffering(TRUE);

			// Initalize the Frame
			rend3d_->context.StartFrame();
			// and the 3D display
			rend3d_->StartDraw();

			((DrawableBSP*)obj->object)->Draw(rend3d_);

			// ok, now fill object and texture banks
			ObjectLOD::WaitUpdates();
			TheTextureBank.WaitUpdates();

			if(DisplayOptions.bZBuffering)
				rend3d_->context.FlushPolyLists();

			rend3d_->EndDraw();
			// CLose the Frame
			rend3d_->context.FinishFrame(NULL);

			gMainHandler->Lock();
			return(TRUE);
		}
	}
	return(FALSE);
}

BOOL C_3dViewer::ViewOTW()
{
	if(rendOTW_ && viewPoint_)
	{
		viewPoint_->Update( &currentPos_);
		gMainHandler->Unlock();

		//JAM 16Dec03
		if(DisplayOptions.bZBuffering)
			rendOTW_->context.SetZBuffering(TRUE);

		rendOTW_->context.StartFrame();
		rendOTW_->StartDraw();
		rendOTW_->DrawScene(&zeroPos_,&currentRot_);

		if(DisplayOptions.bZBuffering)
			rendOTW_->context.FlushPolyLists();

		rendOTW_->EndDraw();
		rendOTW_->context.FinishFrame(NULL);
		//JAM

		gMainHandler->Lock();
		return(TRUE);
	}
	return(FALSE);
}

BOOL C_3dViewer::ViewGreyOTW()
{
	WORD *mem;
	//long i,j,x; // JB 010118 unreferenced variable
	if(rendOTW_ && viewPoint_)
	{
		viewPoint_->Update( &currentPos_);
		gMainHandler->Unlock();

		rendOTW_->context.SetZBuffering(TRUE);

		// RED - As Model Loadings are deferred to Scene drawing, continue to draw the scene till
		// The loader is loading models
/*		do{*/
			rendOTW_->context.StartFrame();
			rendOTW_->StartDraw();
			rendOTW_->PreLoadScene(&zeroPos_,&currentRot_);
			rendOTW_->DrawScene(&zeroPos_,&currentRot_);
			rendOTW_->context.FlushPolyLists();
			rendOTW_->EndDraw();
			rendOTW_->context.FinishFrame(NULL);

/*			// now wait for Loader to end it's work
			TheLoader.WaitForLoader();

			// Draw again the scene, all should ready to be drawn
			rendOTW_->context.StartFrame();
			rendOTW_->StartDraw();
			rendOTW_->DrawScene(&zeroPos_,&currentRot_);
			rendOTW_->context.FlushPolyLists();
			rendOTW_->EndDraw();
			rendOTW_->context.FinishFrame(NULL);
		} while(!TheLoader.LoaderQueueEmpty());*/


		//Wombat778 11-3-2003 Added to allow Latitude/Longitude to be drawn on the display.  A bit of a hack but works ok.
		if (g_bReconLatLong) 
		{
			float latitude, longitude;
			char latstr[25], longstr[25], tempstr[60];		

			ApproxLatLong(Recon.PosX, Recon.PosY, &latitude, &longitude);
			BuildLatLongStr(latitude, longitude, latstr, longstr);
		
			sprintf(tempstr,"%s        %s", latstr, longstr);		
			
			int TempFont = rendOTW_->CurFont();		//Added to be able to restore the font.
			int TempColor = rendOTW_->Color();		//Added to be able to restore the color
			rendOTW_->SetFont(2);					//Set a bigger font
			rendOTW_->SetColor(0xFF00FFFF);			//Yellow.  Seemed to be the best color for visibility.
			rendOTW_->ScreenText((float)viewport.left+4,(float)viewport.top+1,tempstr);
			rendOTW_->SetFont(TempFont);			//Added to restore the font
			rendOTW_->SetColor(TempColor);			//Added to restore the color
			

		}

		//Wombat778 11-3-2003 End of Added Lat/Long code
		

#if 0
		int nWidth = viewport.right - viewport.left;
		int nHeight = viewport.bottom - viewport.top;
		int nSize = nWidth * nHeight;
		RECT rcSrc = { viewport.left, viewport.top, viewport.right, viewport.bottom };
		RECT rcDst = { 0, 0, nWidth, nHeight };

		// Cleanup old buffer if it doesnt fit
		if(m_pImgGray && (m_pImgGray->targetXres() != nWidth || m_pImgGray->targetYres() != nHeight))
		{
			m_pImgGray->Cleanup();
			delete m_pImgGray;
			m_pImgGray = NULL;
		}

		if(!m_pImgGray)
		{
			// Create new work buffer
			m_pImgGray = new ImageBuffer;
			m_pImgGray->Setup(&FalconDisplay.theDisplayDevice, nWidth, nHeight, SystemMem, None);
		}

		// Blit to work buffer
		m_pImgGray->Compose(gMainHandler->GetFront(), &rcSrc, &rcDst);

		// Convert to grey
		mem = (WORD *) m_pImgGray->Lock();
		DWORD col;

		for(int i=0;i<nSize;i++)
		{
			col = m_pImgGray->Pixel16toPixel32(mem[i]);
			col = (RGBA_GETRED(col) + RGBA_GETGREEN(col) + RGBA_GETBLUE(col)) / 3;
			col |= (col << 8) | (col << 16);
			mem[i] = m_pImgGray->Pixel32toPixel16(col);
		}

		m_pImgGray->Unlock();

		// and blit back
		gMainHandler->GetFront()->Compose(m_pImgGray, &rcDst, &rcSrc);

		mem=(WORD*)gMainHandler->Lock();
#else
		mem=(WORD*)gMainHandler->Lock();

// OW FIXME: implement this by blitting to a temp sysmem surface, convert and blitting back
#if 0
		// Convert to greyscale
		for(i=viewport.top;i<viewport.bottom;i++)
		{
			x=static_cast<long>(i*sw);
			for(j=viewport.left;j<viewport.right;j++)
				mem[x+j]=UI95_ScreenToGrey(mem[x+j]);
		}
#endif
#endif

		return(TRUE);
	}
	return(FALSE);
}

BSPLIST *C_3dViewer::Find(long ID)
{
	if(objects_)
		return(objects_->Find(ID));
	return(NULL);
}
