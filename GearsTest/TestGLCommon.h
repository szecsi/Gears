#pragma once

#include "gtest/gtest.h"
#include <iostream>
#include <GL/glew.h>
#include "fft/load_shaders.h"
#include "fft/FFT.h"
#include "fft/openCLFFT.h"
#include "fft/glFFT.h"

#include "TestWindow.h"

HDC hDC;				/* device context */
HGLRC hRC;				/* opengl context */
HWND  hWnd;				/* window */
MSG   msg;				/* message */

int indexPixelFormat;
PIXELFORMATDESCRIPTOR pfd;

bool setGLFormat( void )
{
	pfd =
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
	if ( !(indexPixelFormat = ChoosePixelFormat( hDC, &pfd )) )
	{
		//MessageBox(hwnd, "Failed to find pixel format", "Pixel Format Error", MB_OK);
		//todo error
		return false;
	}

	// Set the pixel format for the provided window DC
	if ( !SetPixelFormat( hDC, indexPixelFormat, &pfd ) )
	{
		//TODO error
		//MessageBox(hwnd, "Failed to Set Pixel Format", "Pixel Format Error", MB_OK);
		return false;
	}
	return true;
}

LONG WINAPI WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateOpenGLWindow( char* title, int x, int y, int width, int height, BYTE type, DWORD flags )
{
	WNDCLASS    wc;
	static HINSTANCE hInstance = 0;

	/* only register the window class once - use hInstance as a flag. */
	if ( !hInstance )
	{
		hInstance = GetModuleHandle( NULL );
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC) WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon( NULL, IDI_WINLOGO );
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "OpenGL";

		if ( !RegisterClass( &wc ) )
		{
			MessageBox( NULL, "RegisterClass() failed:  "
				"Cannot register window class.", "Error", MB_OK );
			return NULL;
		}
	}

	hWnd = CreateWindow( "OpenGL", title, WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		x, y, width, height, NULL, NULL, hInstance, NULL );

	if ( hWnd == NULL )
	{
		MessageBox( NULL, "CreateWindow() failed:  Cannot create a window.",
			"Error", MB_OK );
		return NULL;
	}

	hDC = GetDC( hWnd );

	/* there is no guarantee that the contents of the stack that become
	the pfd are zeroed, therefore _make sure_ to clear these bits. */
	memset( &pfd, 0, sizeof( pfd ) );

	if ( !setGLFormat() )
		return false;

	DescribePixelFormat( hDC, indexPixelFormat, sizeof( PIXELFORMATDESCRIPTOR ), &pfd );

	ReleaseDC(hWnd, hDC );

	return hWnd;
}

void closeWnd()
{
	// Kill window:
	DestroyWindow( hWnd );
	UnregisterClass( "Test", GetModuleHandle( NULL ) );
	std::cout << "Killed window... Take care now. Bye, bye then." << std::endl;
}

bool initGL() 
{
	hWnd = CreateOpenGLWindow( "minimal", 0, 0, 256, 256, PFD_TYPE_RGBA, 0 );
	if ( hWnd == NULL )
		exit( 1 );

	hDC = GetDC( hWnd );
	if ( !(hRC = wglCreateContext( hDC )) )
	{
		return false;
	}
	if ( !wglMakeCurrent( hDC, hRC ) )
		return false;

	glewInit();

	GLenum err = glGetError();
	std::stringstream specs;

	if ( GLEW_OK != err )
	{
		specs << "GLEW/OpenGL Error: " << glewGetErrorString( err ) << std::endl;
	}
	else
	{
		specs << "Using GLEW " << glewGetString( GLEW_VERSION ) << std::endl;
		specs << "Vendor: " << glGetString( GL_VENDOR ) << std::endl;
		specs << "Renderer: " << glGetString( GL_RENDERER ) << std::endl;
		specs << "Version: " << glGetString( GL_VERSION ) << std::endl;
		specs << "GLSL: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << std::endl;
	}

	ShowWindow( hWnd, SW_SHOW );

	return true;
}