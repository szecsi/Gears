#pragma once

#include <iostream>
#include <windows.h>  
#include <tchar.h>
#include <memory>

class TestWindow
{
public:
	HDC hdc;			// device context handle
	HGLRC hglrc;		// OpenGL rendering context
	HWND hWnd;			// window handle

	std::string glSpecs;
	int indexPixelFormat;

	WNDCLASSEX ex;
	
	TestWindow() {}

	static LRESULT CALLBACK WindowProc(
		_In_ HWND   hwnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	)
	{
		TestWindow *c = (TestWindow*) GetWindowLong( hwnd, GWLP_USERDATA );

		if ( c == NULL )
				return DefWindowProc( hwnd, uMsg, wParam, lParam );

		return c->winProc( hwnd, uMsg, wParam, lParam );
	}

	void setGLFormat( void )
	{
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof( PIXELFORMATDESCRIPTOR ),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			16,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // useless parameters
			16,
			0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
		};

		// Choose the closest pixel format available
		if ( !(indexPixelFormat = ChoosePixelFormat( hdc, &pfd )) )
		{
			//MessageBox(hwnd, "Failed to find pixel format", "Pixel Format Error", MB_OK);
			//todo error
			return;
		}

		// Set the pixel format for the provided window DC
		if ( !SetPixelFormat( hdc, indexPixelFormat, &pfd ) )
		{
			//TODO error
			//MessageBox(hwnd, "Failed to Set Pixel Format", "Pixel Format Error", MB_OK);
			return;
		}
	}

	LRESULT winProc( HWND   hwnd, UINT   uMsg, WPARAM wParam, LPARAM lParam )
	{
		switch ( uMsg )
		{
			case WM_CREATE:
			{
				if ( (hdc = GetDC( hwnd )) == NULL )	// get device context
				{
					//TODO error
					return FALSE;
				}
				setGLFormat();			// select pixel format
				if ( (hglrc = wglCreateContext( hdc )) == NULL )	// create the rendering context
				{
					//TODO error
					//MessageBox(hwnd, "Failed to Create the OpenGL Rendering Context",
					//	"OpenGL Rendering Context Error", MB_OK);
					return FALSE;
				}
				if ( (wglMakeCurrent( hdc, hglrc )) == false )		// make hglrc current rc
				{
					//TODO error
					//MessageBox(hwnd, "Failed to make OpenGL Rendering Context current",
					//					 "OpenGL Rendering Context Error", MB_OK);
					return FALSE;
				}
				glDisable( GL_DEPTH_TEST );
				glDepthMask( GL_FALSE );
				GLenum err = glGetError();

				//glewSequenceal = GL_TRUE;
				err = glewInit();

				std::stringstream specs;

				if ( GLEW_OK != err )
				{
					std::stringstream ss;
					ss << "GLEW/OpenGL Error: " << glewGetErrorString( err ) << std::endl;
					std::cerr << ss.str();
				}
				else
				{
					specs << "Using GLEW " << glewGetString( GLEW_VERSION ) << std::endl;
					specs << "Vendor: " << glGetString( GL_VENDOR ) << std::endl;
					specs << "Renderer: " << glGetString( GL_RENDERER ) << std::endl;
					specs << "Version: " << glGetString( GL_VERSION ) << std::endl;
					specs << "GLSL: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << std::endl;

					std::cout << specs.str();
				}
				err = glGetError();
				if ( err == GL_NO_ERROR )
					std::cout << "GLEW initialized.\n";
				else
					std::cerr << "OpenGL error: " << err << "\n";

				glSpecs = specs.str();

				break;
			}
		}
		return TRUE;
	}

	void registerClass() 
	{
		ex.cbSize = sizeof( WNDCLASSEX );
		ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		ex.lpfnWndProc = TestWindow::WindowProc;
		ex.cbClsExtra = 0;
		ex.cbWndExtra = 0;
		ex.hInstance = 0;
		ex.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		ex.hCursor = LoadCursor( NULL, IDC_CROSS );
		ex.hbrBackground = NULL;
		ex.lpszMenuName = NULL;
		ex.lpszClassName = "GearsStimulusWindow";
		ex.hIconSm = NULL;

		RegisterClassEx( &ex );
	}
	void createWindow()
	{
		long wndStyle = WS_OVERLAPPEDWINDOW;
		hWnd = CreateWindowEx( NULL,
			"GearsStimulusWindow",
			"Stimulus window",
			wndStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0,
			100, 100,
			NULL,
			NULL,
			0,
			NULL );

		SetWindowLong( hWnd, GWLP_USERDATA, (long)this );
		ShowWindow( hWnd, SW_SHOW );
	}
};
