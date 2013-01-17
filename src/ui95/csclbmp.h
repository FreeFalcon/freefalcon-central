#ifndef _SCALEBITMAP_UI_H_
#define _SCALEBITMAP_UI_H_

class C_ScaleBitmap : public C_Base
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		// Save from Here
		long ImageID_;
		long DefaultFlags_;
		BOOL UseOverlay_;

		// Don't Save from here down
		short Rows_[800];
		short Cols_[800];
		WORD r_shift_;
		WORD g_shift_;
		WORD b_shift_;

		BYTE *Overlay_;
		WORD *Palette_[16];
		O_Output *Image_;

	public:
		C_ScaleBitmap();
		C_ScaleBitmap(char **stream);
		C_ScaleBitmap(FILE *fp);
		~C_ScaleBitmap();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		// Initialization Function
		void Setup(long ID,short Type,long ImageID);
		void InitOverlay();
		BYTE *GetOverlay() { return(Overlay_); }
		void UseOverlay() { if(Overlay_) UseOverlay_=TRUE; }
		void NoOverlay() { UseOverlay_=FALSE; }
		void ClearOverlay();
		void PreparePalette(COLORREF color);
		void SetImage(long ID);
		void SetImage(IMAGE_RSC *image);
		void SetSrcRect(UI95_RECT *rect) { if(Image_ != NULL) Image_->SetSrcRect(rect); }
		void SetDestRect(UI95_RECT *rect) { if(Image_ != NULL) Image_->SetDestRect(rect); }
		void SetScaleInfo(long scale) { if(Image_ != NULL) Image_->SetScaleInfo(scale); }
		UI95_RECT *GetSrcRect() { if(Image_ != NULL) return(Image_->GetSrcRect()); return(NULL); }
		UI95_RECT *GetDestRect() { if(Image_ != NULL) return(Image_->GetDestRect()); return(NULL); }

		IMAGE_RSC *GetImage(void) { if(Image_ != NULL) return(Image_->GetImage()); return(NULL); }
		// Free Function
		void Cleanup(void);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void SetFlags(long flags);

		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

#ifdef _UI95_PARSER_
		
		short LocalFind(char *token);
		void LocalFunction(short ID,long P[],_TCHAR *,C_Handler *);
		void SaveText(HANDLE ,C_Parser *)	{ ; }

#endif // PARSER
};

#endif
