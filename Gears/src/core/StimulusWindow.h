#pragma once

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
#include <string>
#include "SequenceRenderer.h"
#include "wglext.h"

class StimulusWindow
{
	HDC hdc;			// device context handle
	HGLRC hglrc;		// OpenGL rendering context
	HWND hwnd;			// window handle
	int screenw;		// when window is resized, the new dimensions...
	int screenh;		// ...are stored in these variables
	bool fullscreen;
	bool quit;			// indicates the state of application
	int indexPixelFormat;	// number of available pixel formats

	std::string glSpecs;

	static WNDCLASSEX ex;

	static PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
	static PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;
	SequenceRenderer::P	sequenceRenderer;
	Ticker::P ticker;
	StimulusWindow();

	boost::python::object onHideCallback;
public:
	GEARS_SHARED_CREATE_WITH_GETSHAREDPTR(StimulusWindow);
	static StimulusWindow::P instanceCreated;
	void createWindow(bool windowed, uint width, uint height);
	void run();
	void closeWindow();
	static LRESULT CALLBACK WindowProc(
	  _In_ HWND   hwnd,
	  _In_ UINT   uMsg,
	  _In_ WPARAM wParam,
	  _In_ LPARAM lParam
	);
	LRESULT winProc(HWND   hwnd, UINT   uMsg, WPARAM wParam, LPARAM lParam);
	static void registerClass();
	void setGLFormat (void);
	std::string getSpecs(){return glSpecs;}

	int setSwapInterval(int swapInterval)
	{
		wglSwapIntervalEXT(swapInterval);
		return wglGetSwapIntervalEXT();
	}

	void makeCurrent();
	void shareCurrent();

	void setSequenceRenderer(SequenceRenderer::P	sequenceRenderer)
	{
		this->sequenceRenderer = sequenceRenderer;
	}

	void onHide(boost::python::object onHide)
	{
		onHideCallback = onHide;
	}
};
