#ifndef CPMIRROR_H
#define CPMIRROR_H

#include <memory>
#include "cpobject.h"

// forward declarations
class Render2D;
class ImageBuffer;

/** a mirror object */
class CPMirror : public CPObject {
public:
	/** constructor receiving parsed information */
	explicit CPMirror(const ObjectInitStr &ois);

	// CPObject virtual interface
	virtual void Exec(SimBaseClass*);
	virtual void DisplayBlit3D();
private:
	/** renders the 3d mirror image */
	void RenderMirror(float left, float top, float right, float bottom);
	std::auto_ptr<Render2D> mRend;
	std::auto_ptr<ImageBuffer> mBuffer;
};

#endif