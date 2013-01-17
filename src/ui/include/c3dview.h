#ifndef _UI_VIEWER_H_
#define _UI_VIEWER_H_

//
// NOTE: All angles are in degrees (0-360 or -90 to 90) and converted to radians internally
//

class C_3dViewer
{
	private:
		Render3D  *rend3d_; // 3d Renderer... use rend3d OR rendOTW NOT both at same time
		RenderOTW *rendOTW_; // Terrain Renderer
		RViewPoint *viewPoint_; // Terrain viewpoint
		Tpoint zeroPos_; // Used ONLY for Terrain Viewing
		Tpoint viewPos_; // Center point of the view
		Tpoint CameraPos_; // Camera offset from center point
		Tpoint currentPos_; // Center Point + Camera offset (ie the actual point)
		Trotation currentRot_; // Camera angles for ALL viewing
		C_BSPList *objects_; // loaded objects list

		ImageBuffer *m_pImgGray;		// OW


		float CameraHeading_,CameraPitch_,CameraRoll_;
		float ViewDistance_;
		unsigned long Time_;
//		long TextureLevel_;
//		long SmoothShading_;
		long MinTexture_,MaxTexture_;
		float l,t,r,b; // Viewport borders
		float sw,sh; // ImageBuffer W,H
		float Weather_;
		long LockOnView_;
		UI95_RECT viewport;

	public:
		C_3dViewer()
		{
			rend3d_=NULL;
			rendOTW_=NULL;
			viewPoint_=NULL;
			objects_=NULL;
			l=-1.0f; t=1.0f;r=1.0f;b=-1.0f;
			viewPos_.x=0.0f;
			viewPos_.y=0.0f;
			viewPos_.z=0.0f;
			zeroPos_=viewPos_;
			currentPos_=viewPos_;
			CameraPos_=viewPos_;
			currentRot_=IMatrix;
			CameraHeading_=0.0f;
			CameraPitch_=0.0f;
			CameraRoll_=0.0f;
			ViewDistance_=10.0f * FEET_PER_KM;
			Time_=12l * 60l * 60l * 1000l;
			MinTexture_=0;
			MaxTexture_=1;
			Weather_=0;
//			TextureLevel_=2;
//			SmoothShading_=TRUE;
			sw=800; sh=600;
			LockOnView_=0;
			m_pImgGray = NULL;	// OW
		}
		~C_3dViewer() { Cleanup(); }
		BOOL Setup();
		BOOL Cleanup();
		void SetWeather(float w) { Weather_=w; }
		void SetViewDistance(float d) { ViewDistance_=d; }
		void SetMinMaxTexture(long min,long max) { MinTexture_=min; MaxTexture_=max; }
		void Viewport(C_Window *win,long client); // Calulates the left,top,right,bottom offsets for viewport
		void SetPosition(float x,float y,float z);
		// VERY IMPORTANT: Camera Position is RELATIVE to Position
		// for Object Viewing: 0,0,0 is the assumed object position... camera is relative to that
		void SetCamera(float x,float y,float z,float heading,float climb,float roll);
//		void SetTextureLevel(long tl) { TextureLevel_=tl; }
//		void SetSmoothShading(long ss) { SmoothShading_=ss; }
		void SetTextureLevels(long min,long max) { MinTexture_=min; MaxTexture_=max; }
		void SetCameraHeading(float h) { SetCamera(CameraPos_.x,CameraPos_.y,CameraPos_.z,h,CameraPitch_,CameraRoll_); }
		void SetCameraPitch(float y) { SetCamera(CameraPos_.x,CameraPos_.y,CameraPos_.z,CameraHeading_,y,CameraRoll_); }
		void SetCameraRoll(float r) { SetCamera(CameraPos_.x,CameraPos_.y,CameraPos_.z,CameraHeading_,CameraPitch_,r); }
		void SetCameraX(float x) { SetCamera(x,CameraPos_.y,CameraPos_.z,CameraHeading_,CameraPitch_,CameraRoll_); }
		void SetCameraY(float y) { SetCamera(CameraPos_.x,y,CameraPos_.z,CameraHeading_,CameraPitch_,CameraRoll_); }
		void SetCameraZ(float z) { SetCamera(CameraPos_.x,CameraPos_.y,z,CameraHeading_,CameraPitch_,CameraRoll_); }
		float GetCameraHeading() { return(CameraHeading_); }
		float GetCameraPitch() { return(CameraPitch_); }
		float GetCameraRoll() { return(CameraRoll_); }
		float GetCameraX() { return(CameraPos_.x); }
		float GetCameraY() { return(CameraPos_.y); }
		float GetCameraZ() { return(CameraPos_.z); }
		Render3D *GetRend3d() { return(rend3d_); }
		RenderOTW *GetRendOTW() { return(rendOTW_); }
		RViewPoint *GetVP() { return(viewPoint_); }
		BOOL InitOTW(float FOV,BOOL Preload=FALSE); // This loads terrain (ie SLOW)
		BOOL Init3d(float FOV); // This just sets viewport
		BSPLIST *Load(long ID,int objID);
		BSPLIST *LoadBSP(long ID,int objID,BOOL aircraft=FALSE);
		BSPLIST *LoadBuilding(long ID,int objID,Tpoint *pos,float heading);
		BSPLIST *LoadBridge(long ID,int objID);
		BSPLIST *LoadDrawableFeature(long ID,Objective obj,short f,short fid,Falcon4EntityClassType *classPtr,FeatureClassDataType*	fc,Tpoint *objPos,BSPLIST *Parent);
		BSPLIST *LoadDrawableUnit(long ID,long visType,Tpoint *objPos,float facing,uchar domain,uchar type,uchar stype);
		BOOL Remove(long ID);
		BOOL RemoveAll();
		BOOL AddToView(BSPLIST *obj);
		BOOL AddToView(long ID);
		BOOL AddAllToView();
		BOOL View3d(long ID);
		BOOL ViewOTW();
		BOOL ViewGreyOTW();
		BSPLIST *Find(long ID);
		C_BSPList *GetBSPList() { return(objects_); }
};

extern C_3dViewer *gUIViewer;

#endif