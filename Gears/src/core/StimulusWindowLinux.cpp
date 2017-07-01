#include "stdafx.h"

#ifdef __linux__

#include <X11/Xatom.h>
#include <assert.h>   // I include this to test return values the lazy way
#include <iostream>
#include <chrono>
#include <ctime>
#include <unistd.h>

#include "StimulusWindow.h"

StimulusWindow::StimulusWindow()
{
	display = nullptr;
	ctx = nullptr;
}

void StimulusWindow::createWindow(bool windowed, uint width, uint height)
{
	// Create display
	display = XOpenDisplay(0);
	if( !display )
	{
		std::cout << "Cannot open Display." << std::endl;
		return;
	}

	Screen*  scrn = DefaultScreenOfDisplay(display);

	screenw = scrn->width;
	screenh = scrn->height;

	vscreenw = scrn->width;
	vscreenh = scrn->height;

	// center position of the window
	int posx = (screenw / 2) - (width / 2);
	int posy = (screenh / 2) - (height / 2);

	static int visual_attribs[] =
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};

	int glx_major, glx_minor;

	// FBConfigs were added in GLX version 1.3.
	if ( !glXQueryVersion( display, &glx_major, &glx_minor ) || 
		( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
	{
		std::cout << "Invalid GLX version";
		return;
	}

	std::cout <<  "Getting matching framebuffer configs" << std::endl;;
	int fbcount;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
	if (!fbc)
	{
		//printf( "Failed to retrieve a framebuffer config\n" );
		return;
	}
	std::cout << "Found" << fbcount << "matching FB configs." << std::endl;

	// Pick the FB config/visual with the most samples per pixel
	std::cout << "Getting XVisualInfos" << std::endl;
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

	for (size_t i = 0; i < fbcount; ++i)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
		if ( vi )
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );
      
			std::cout << "  Matching fbconfig" << i << ", visual ID 0x" << vi->visualid << ": SAMPLE_BUFFERS = " << samp_buf << ", SAMPLES = " << samples << std::endl;

			if ( best_fbc < 0 || samp_buf && samples > best_num_samp )
				best_fbc = i, best_num_samp = samples;
			if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
				worst_fbc = i, worst_num_samp = samples;
		}
		XFree( vi );
	}

	GLXFBConfig bestFbc = fbc[ best_fbc ];

	// Be sure to free the FBConfig list allocated by glXChooseFBConfig()
	XFree( fbc );

	// Get a visual
	XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );
	std::cout << "Chosen visual ID = 0x" << vi->visualid << std::endl;

	std::cout << "Creating colormap" << std::endl;
	XSetWindowAttributes swa;
	swa.colormap = cmap = XCreateColormap( display,
		RootWindow( display, vi->screen ), 
		vi->visual, AllocNone );

	swa.background_pixmap = None ;
	swa.border_pixel      = 0;
	swa.event_mask        = StructureNotifyMask;

	// Create window
	wnd = XCreateWindow( display, 
		RootWindow(display, vi->screen), 
		posx, posy, 
		screenw, screenh, 
		0, 
		vi->depth, 
		InputOutput,
    	vi->visual,
    	CWBorderPixel|CWColormap|CWEventMask, 
    	&swa );

	if ( !wnd )
	{
		std::cout << "Failed to create window." << std::endl;
		return;
	}

	// Done with the visual info data
	XFree( vi );

	XStoreName( display, wnd, "GL 3.0 Window" );

	XSelectInput(display, wnd, eventMask);


	//////////////////////////////////////////////

	// Get the default screen's GLX extension list
	const char *glxExts = glXQueryExtensionsString( display, 
													DefaultScreen( display ) );

	// NOTE: It is not necessary to create or make current to a context before
	// calling glXGetProcAddressARB
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
	glXCreateContextAttribsARB = ( glXCreateContextAttribsARBProc)
								   glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );


	// Install an X error handler so the application won't exit if GL 3.0
	// context allocation fails.
	//
	// Note this error handler is global.  All display connections in all threads
	// of a process use the same error handler, so be sure to guard against other
	// threads issuing X commands while this code is running.
	ctxErrorOccurred = false;
	int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

	// Check for the GLX_ARB_create_context extension string and the function.
	// If either is not present, use GLX 1.3 context creation method.
	if (!isExtensionSupported(glxExts, "GLX_ARB_create_context") ||
		!glXCreateContextAttribsARB )
	{
		//printf( "glXCreateContextAttribsARB() not found"
		//" ... using old-style GLX context\n" );
		ctx = glXCreateNewContext( display, bestFbc, GLX_RGBA_TYPE, 0, True );
	}

	  // If it does, try to get a GL 3.0 context!
	else
	{
		int context_attribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 0,
			//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			None
		};

		//printf( "Creating context\n" );
		ctx = glXCreateContextAttribsARB( display, bestFbc, 0,
                                      True, context_attribs );

		// Sync to ensure any errors generated are processed.
		XSync( display, False );
		if ( !ctxErrorOccurred && ctx )
			std::cout << "Created GL 3.0 context" << std::endl;
		else
		{
			// Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
			// When a context version below 3.0 is requested, implementations will
			// return the newest context version compatible with OpenGL versions less
			// than version 3.0.
			// GLX_CONTEXT_MAJOR_VERSION_ARB = 1
			context_attribs[1] = 1;
			// GLX_CONTEXT_MINOR_VERSION_ARB = 0
			context_attribs[3] = 0;

			ctxErrorOccurred = false;

			//printf( "Failed to create GL 3.0 context"
    	          //" ... using old-style GLX context\n" );
			ctx = glXCreateContextAttribsARB( display, bestFbc, 0, 
    	                                    True, context_attribs );
    	}
	}
	// Sync to ensure any errors generated are processed.
	XSync( display, False );

 	// Restore the original error handler
 	XSetErrorHandler( oldHandler );

	if ( ctxErrorOccurred || !ctx )
	{
	  std::cout << "Failed to create an OpenGL context\n" << std::endl;
	  return;
	}

	// Verifying that context is a direct context
	if ( ! glXIsDirect ( display, ctx ) )
	{
	  std::cout << "Indirect GLX rendering context obtained\n" << std::endl;
	}
	else
	{
	  std::cout << "Direct GLX rendering context obtained\n" << std::endl;
	}

	//printf( "Making context current\n" );
	makeCurrent();

	///////////////////////////////////////////////

	GLenum err = glGetError();
	err = glewInit();

	std::stringstream specs;

	if(GLEW_OK != err)	{
		std::stringstream ss;
		ss << "GLEW/OpenGL Error: " << glewGetErrorString(err) << std::endl;
		std::cerr << ss.str();
		PyErr_SetString(PyExc_RuntimeError, ss.str().c_str());
		boost::python::throw_error_already_set();
	}
	else {
		specs << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
		specs << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
		specs << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
		specs << "Version: " << glGetString(GL_VERSION) << std::endl;
		specs << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

		std::cout << specs.str() << std::endl;
	}
	err = glGetError();
	if(err == GL_NO_ERROR)
		std::cout << "GLEW initialized.\n";
	else
	{
		std::cerr << "OpenGL error: " << glewGetErrorString(err) << "\n";
	}

}

void StimulusWindow::run()
{
	Atom atoms[2] = { XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False), None };
	XChangeProperty(
		display, 
		wnd, 
		XInternAtom(display, "_NET_WM_STATE", False),
		XA_ATOM, 32, PropModeReplace, (unsigned char *)atoms, 1
	);
	XMapWindow(display, wnd);

	glViewport(0, 0, screenw, screenh);

	if(sequenceRenderer->getSequence()->getUsesBusyWaitingThreadForSingals())
		ticker = sequenceRenderer->startTicker();

	quit = false;
	XEvent e;
	while(!quit)
	{
		if( XCheckMaskEvent(display, eventMask, &e) )
		{
			/* exit on key press */
			if(e.type==KeyPress)
			{
				quit = true;
			}
		}

		makeCurrent();
		glViewport(0, 0, screenw, screenh);

		glClearColor(0.5, 0.1, 0.2, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		if(sequenceRenderer->exporting())
			setSwapInterval( 0 );
		else
		{
			setSwapInterval( sequenceRenderer->getSequence()->frameRateDivisor );
		}
		sequenceRenderer->setScreenResolution(screenw, screenh);
		if(! sequenceRenderer->renderFrame(0) )
			quit = true;
			//TODO finished 

		glXSwapBuffers ( display, wnd );
		glFinish();
		if(ticker)
			ticker->onBufferSwap();

		//if(GetAsyncKeyState(VK_ESCAPE))
		//{
		//	quit = true;
		//}
	}
	if(sequenceRenderer->getSequence()->getUsesBusyWaitingThreadForSingals())
		ticker->stop();
	ticker.reset();
	sequenceRenderer->reset();
	XUnmapWindow(display, wnd);
}

void StimulusWindow::closeWindow()
{
	XDestroyWindow( display, wnd );
	XFreeColormap( display, cmap );
	XCloseDisplay( display );
}

void StimulusWindow::setGLFormat(void)
{
	//TODO: Make implementation in Linux
}

int StimulusWindow::setSwapInterval(int swapInterval)
{
	//TODO: Make implementation in Linux
}

void StimulusWindow::makeCurrent()
{
	if((glXMakeCurrent(display, wnd, ctx)) == false)
	{
		//TODO error
		//MessageBox(hwnd, "Failed to make OpenGL Rendering Context current",
		//					 "OpenGL Rendering Context Error", MB_OK);
		closeWindow();
	}
}

void StimulusWindow::shareCurrent()
{
	//TODO: Make implementation in Linux
}

void StimulusWindow::setCursorPos()
  {
    uint screenw = DefaultScreenOfDisplay(display)->width;
		uint screenh = DefaultScreenOfDisplay(display)->height;
    XWarpPointer(display, None, wnd, 0, 0, 0, 0, screenw, screenh);

    XFlush(display);
  }

StimulusWindow::P StimulusWindow::instanceCreated(nullptr);

#endif // __linux__