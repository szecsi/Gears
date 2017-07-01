#pragma once

#ifdef __linux__
#	include <X11/Xlib.h> // Every Xlib program must include this
#	include <X11/Xutil.h>
#endif

#include <algorithm>
#include <string>
#include <map>
#include "math/math.h"
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <iomanip>
#include <list>
#include <set>
#include <vector>
#include "SequenceRenderer.h"
#ifdef _WIN32
#	include "wglext.h"
#elif __linux__
#	include <GL/gl.h> //OS x libs
#	include <GL/glu.h>
#	include <GL/glx.h>

#	include <unistd.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
static bool isExtensionSupported(const char *extList, const char *extension)
{
  const char *start;
  const char *where, *terminator;
  
  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if (where || *extension == '\0')
    return false;

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for (start=extList;;) {
    where = strstr(start, extension);

    if (!where)
      break;

    terminator = where + strlen(extension);

    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return true;

    start = terminator;
  }

  return false;
}

static bool ctxErrorOccurred = false;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = true;
    return 0;
}
#endif // __linux__

class StimulusWindow
{
#ifdef _WIN32
	HDC hdc;			// device context handle
	HGLRC hglrc;		// OpenGL rendering context
	HWND hwnd;			// window handle
#elif __linux__
	Display *display;
	Window wnd;
	GLXContext ctx;
	Colormap cmap;
#endif
	int screenw;		// when window is resized, the new dimensions...
	int screenh;		// ...are stored in these variables
	int vscreenw;		// when window is resized, the new dimensions...
	int vscreenh;		// ...are stored in these variables
	bool quit;			// indicates the state of application
#ifdef _WIN32
	bool fullscreen;
	int indexPixelFormat;	// number of available pixel formats

	HCURSOR arrowCursor;
	HCURSOR crossCursor;
	HCURSOR handCursor ;

	static WNDCLASSEX ex;

	static PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
	static PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;
#endif

	std::string glSpecs;
	SequenceRenderer::P	sequenceRenderer;
	Ticker::P ticker;
	StimulusWindow();

	boost::python::object onHideCallback;
#ifdef _WIN32
	struct Monitor{
		HMONITOR hMonitor;
		HDC      hdcMonitor;
		RECT	rcMonitor;
	};
	std::vector<Monitor> monitors;
	uint currentMonitor;
#elif __linux__
	long eventMask = KeyPressMask;
#endif

public:
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(StimulusWindow);
	static StimulusWindow::P instanceCreated;
	void createWindow(bool windowed, uint width, uint height);
	void run();
	void closeWindow();
#ifdef _WIN32
	static LRESULT CALLBACK WindowProc(
	  _In_ HWND   hwnd,
	  _In_ UINT   uMsg,
	  _In_ WPARAM wParam,
	  _In_ LPARAM lParam
	);
	LRESULT winProc(HWND   hwnd, UINT   uMsg, WPARAM wParam, LPARAM lParam);
	static void registerClass();
#endif
	void setGLFormat (void);
	std::string getSpecs(){return glSpecs;}

	int setSwapInterval(int swapInterval);
	void makeCurrent();
	void shareCurrent();
	void setCursorPos();

	void setSequenceRenderer(SequenceRenderer::P	sequenceRenderer)
	{
		this->sequenceRenderer = sequenceRenderer;
	}

	void onHide(boost::python::object onHide)
	{
		onHideCallback = onHide;
	}
#ifdef _WIN32
	void addMonitor(
	  HMONITOR hMonitor,
	  HDC      hdcMonitor,
	  LPRECT   lprcMonitor);
#endif
};
