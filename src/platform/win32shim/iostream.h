// Artscout - 2026 (#104, Linux Ф1): <iostream.h> -- the pre-standard header name (sim/helo). Forwards to <iostream>
// and lifts the names into the global namespace, which is what the old header did.
#ifndef FF_WIN32SHIM_IOSTREAM_OLD_H
#define FF_WIN32SHIM_IOSTREAM_OLD_H
#include <iostream>
using std::cerr;
using std::cin;
using std::clog;
using std::cout;
using std::endl;
using std::ends;
using std::flush;
using std::ios;
using std::iostream;
using std::istream;
using std::ostream;
using std::streambuf;
#endif
