#ifndef VU_GC_H
#define VU_GC_H

/** @file vu_gc.h vu garbage collector delcaration. */
#include <memory>

/** garbage collector class. All entities inserted here are removed when flush is called.
* The insertion functions are thread safe.
*/
class VuGC {
public:
	/** creates the garbage collector and its internal data. */
	explicit VuGC();
	/** destroys the garbage collector, flushing before destruction. */
	~VuGC();

	/** inserts a list node to be removed. */
	void insert(class VuEntity *e);

	/** flushes the garbage collector, freeing everything in it. Call this somewhere SAFE when no thread is using VU (IE sleeping) */
	void flush();

private:
	struct Data;
	/** garbage collector internal data. */
	std::auto_ptr<Data> d;
};


#endif

