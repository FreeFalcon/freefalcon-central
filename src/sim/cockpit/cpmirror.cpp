/** @file CPMirror 
* 2d cockpit mirror implementation 
*/
#include "cpmirror.h"
#include "Graphics/include/Render3d.h"
#include "Sim/Include/Otwdrive.h"
#include "dispcfg.h"
#include "renderow.h"

CPMirror::CPMirror(const ObjectInitStr &ois) : CPObject(&ois){
	// always update... should go away, may help FPS
	mCycleBits = 0xffffffff;
	mBuffer.reset(new ImageBuffer());
	mBuffer->Setup(&FalconDisplay.theDisplayDevice, 
		ois.destRect.right - ois.destRect.left + 1, 
		ois.destRect.bottom - ois.destRect.top + 1,
		SystemMem, None
	);
	mRend.reset(new Render2D());
	mRend->Setup(mBuffer.get());
}

void CPMirror::Exec(SimBaseClass *simbase){
	float xPos = 0.0F;
	float yPos = 0.6F;
	RenderMirror((float)mDestRect.left, (float)mDestRect.top, (float)mDestRect.right, (float)mDestRect.bottom);
}

// sfr: mirrors here. @TODO move to some place more appropriate
void CPMirror::RenderMirror(float left, float top, float right, float bottom){
	mRend->StartDraw();	
	mRend->SetColor(0xFFFFFFFF);
	// white triagle pointing down
	float w = right - left - 1.0f;
	float h = bottom - top - 1.0f;
	mRend->Render2DTri(
		1.0f, 1.0f, 
		w/2.0f, h,
		w, 1.0f
	);
	mRend->EndDraw();
	mBuffer->SwapBuffers(false);
	return;

#if 0
	// Save off the current renderer info
	//float prevFOV, prevLeft, prevRight, prevTop, prevBottom;
	//mRend.GetViewport(&prevLeft, &prevTop, &prevRight, &prevBottom);
	//prevFOV = renderer->GetFOV();

	// get platform position and rotation
	Trotation prot, mview;
	TransformMatrix &dmx = mpOwnship->dmx;
	prot.M11 = dmx[0][0];
	prot.M21 = dmx[0][1];
	prot.M31 = dmx[0][2];
	prot.M12 = dmx[1][0];
	prot.M22 = dmx[1][1];
	prot.M32 = dmx[1][2];
	prot.M13 = dmx[2][0];
	prot.M23 = dmx[2][1];
	prot.M33 = dmx[2][2];

	// pan camera backwards
	Trotation pan = IMatrix;
	pan.M11 = -1.0f;
	pan.M12 = 0.0f;
	pan.M21	= 0.0f;
	pan.M22 = -1.0f;
	MatrixMult(&prot, &pan, &mview);
	memcpy(&prot, &mview, sizeof(prot));
	// mirror, a bit higher
	Trotation mirror = IMatrix;
	mirror.M22 = -1.0f;
	MatrixMult(&prot, &mirror, &mview);

	// Set mirror position
	mRend.SetViewport(left, top, right, bottom);
	// temporary hack to position until we can figure out how to make translation in this system
	Tpoint zeroPos = {0.0F, 0.0F, 1.0F};
	mRend.DrawScene(&zeroPos, &mview);
	mRend.context.FlushPolyLists();

	// restore renderer
	//mRend.SetFOV(prevFOV);
	//mRend.SetViewport(prevLeft, prevTop, prevRight, prevBottom);
	//mRend.SetCamera(&ppos, &cameraRot);
	mRend.EndDraw();
#endif
}

void CPMirror::DisplayBlit3D(){
	DWORD *buf = static_cast<DWORD*>(mBuffer->Lock());
	RenderOTW *r = OTWDriver.renderer;
	r->StartDraw();
	int w = mDestRect.right - mDestRect.left + 1, h = mDestRect.bottom - mDestRect.top + 1;
	r->Render2DBitmap(0, 0, mDestRect.left, mDestRect.top, w, h, w, buf);
	r->EndDraw();
	mBuffer->Unlock();
	//int w = mDestRect.right - mDestRect.left + 1, h = mDestRect.bottom - mDestRect.top + 1;
	//r->Render2DBitmap(0, 0, mDestRect.left, mDestRect.top, w, h, w, mBuffer->Ge 
	//mRend->GetRttCanvas();
}


