#pragma once

#include "gtest/gtest.h"
#include <iostream>
#include <GL/glew.h>
#include "fft/load_shaders.h"
#include "fft/FFT.h"
#include "fft/openCLFFT.h"
#include "fft/glFFT.h"

class TestGLHelper
{
	HDC hDC;				/* device context */
	HGLRC hRC;				/* opengl context */
	HWND  hWnd;				/* window */
	MSG   msg;				/* message */

	int indexPixelFormat;
	PIXELFORMATDESCRIPTOR pfd;

	bool setGLFormat( void );

	HWND CreateOpenGLWindow( char* title, int x, int y, int width, int height, BYTE type, DWORD flags );

	void closeWnd();
public:
	bool initGL();
};
