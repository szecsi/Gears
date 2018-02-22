#include "gtest/gtest.h"
#include <iostream>
#include <GL/glew.h>
#include "fft/load_shaders.h"
#include "fft/FFT.h"
#include "fft/openCLFFT.h"
#include "fft/glFFT.h"

#include "TestWindow.h"

#include "fft/glFFT.cpp"
#include "fft/openCLFFT.cpp"
#include "fft/load_shaders.cpp"

HDC hDC;				/* device context */
HGLRC hRC;				/* opengl context */
HWND  hWnd;				/* window */
MSG   msg;				/* message */

int indexPixelFormat;
PIXELFORMATDESCRIPTOR pfd;

const int runNumber = 10;

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

LONG WINAPI
WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND
CreateOpenGLWindow( char* title, int x, int y, int width, int height,
	BYTE type, DWORD flags )
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


void printImg( float* img, unsigned w, unsigned h )
{
	if ( w > 10 )
		return;
	std::cout << "FFT: " << std::endl;
	for ( unsigned i = 0; i < w * 4 * h; i += 4 )
	{
		if ( i % (4 * w) == 0 )
			std::cout << std::endl;
		std::cout << "(" << img[i] << ", " << img[i + 1] << ", " << img[i + 2] << ", " << img[i + 3] << ")";
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

bool mtxIsEqual( float* mtx1, float* mtx2, unsigned w, unsigned h )
{
	for(unsigned i = 0; i < h; i++)
		for ( unsigned j = 0; j < w*4; j++ )
		{
			unsigned index = i*w * 4 + j;
			if ( abs(mtx1[index] - mtx2[index]) > 0.000001 )
			{
				std::cout << abs( mtx1[index] - mtx2[index] ) << std::endl;;
				std::cout << mtx1[index] << " != " << mtx2[index] << std::endl;
				return false;
			}
		}
	return true;
}
float* p = nullptr;
unsigned w, h;
//float p[] = {
//	0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 1.f, 1.f, 1.f, 1.0f, 0.f, 0.f, 0.f, 1.f,
//	0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.0f, 0.f, 0.f, 0.f, 1.f
//};
//float p[] = {
//	0.f, 0.f, 0.f, 1.f,   1.f, 2.f, 3.f, 1.f,
//	0.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f
//};

void initTexture(unsigned* tex, unsigned w, unsigned h, float* pixels = 0)
{
	
	if ( !pixels )
	{
		pixels = p;
	}

	glGenTextures( 1, tex );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, *tex );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, pixels );
}

void generateInputData()
{
	w = h = 10;
	p = new float[w * h * 4];
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			float a = (i % 2 == 0 || j % 2 == 0) ? 1.f : 0.f;
			p[i*w * 4 + j + 0] = a;
			p[i*w * 4 + j + 1] = a * 0.3f;
			p[i*w * 4 + j + 2] = a * 0.7f;
			p[i*w * 4 + j + 3] = 1.f;
		}
}

TEST( FFTTest, SimpleglFFTMonochrome )
{
	unsigned tex;
	initTexture( &tex, w, h );
	GLFFT fft( w, h, tex );
	GLFFT ifft( w, h, tex, true, true );
	fft.set_input( [] () {} );
	GLfloat* img;
	img = new GLfloat[w * h * 4];
	fft.do_fft();

	printImg( p, w, h );
	printImg( img, w, h );

	ifft.do_fft();

	//std::cout << "FT finished elapsed time: " << elapsed_seconds.count() * 1000 / runNumber << "ms." << std::endl;
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );
	printImg(p, w, h);
	printImg(img, w, h);
	delete[] img;
	//printFFT( img, w, h );
}

TEST( FFTTest, SimpleglFFT ) 
{
	unsigned tex;
	initTexture( &tex, w, h );
	GLFFT fft( w, h, tex );
	GLFFT ifft(w, h, tex, true, true);
	fft.set_input( [] () {} );
	GLfloat* img;
	img = new GLfloat[w * h * 4];
	auto start = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds;
	
	for ( int i = 0; i < runNumber; i++ )
	{
		elapsed_seconds = start - start;
		start = std::chrono::system_clock::now();
		fft.do_fft();
		ifft.do_fft();
		auto end = std::chrono::system_clock::now();
		elapsed_seconds += end - start;
		std::cout << "FT finished elapsed time: " << elapsed_seconds.count() * 1000 / (i + 1) << "ms." << std::endl;
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	}

	//std::cout << "FT finished elapsed time: " << elapsed_seconds.count() * 1000 / runNumber << "ms." << std::endl;
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB , 0, GL_RGBA, GL_FLOAT, img);
	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );
	delete[] img;
	//printFFT( img, w, h );
}

TEST( FFTTest, SimpleclFFT )
{
	unsigned tex;
	initTexture( &tex, w, h );
	OPENCLFFT fft( w, h, tex );
	fft.set_input( [] () {} );
	auto start = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds;
	GLfloat* img;
	img = new GLfloat[w * h * 4];

	for ( int i = 0; i < runNumber; i++ )
	{
		elapsed_seconds = start - start;
		//printImg( img, w, h );
		start = std::chrono::system_clock::now();
		fft.do_fft();
		//initTexture( tex, w, h, alma );
		//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
		//glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
		auto end = std::chrono::system_clock::now();
		elapsed_seconds += end - start;
		fft.finish();
		start = std::chrono::system_clock::now();
		//printImg( img, w, h );
		fft.do_inverse_fft();
		end = std::chrono::system_clock::now();
		elapsed_seconds += end - start;
		std::cout << "FT finished elapsed time: " << elapsed_seconds.count() * 1000 / (i + 1) << "ms." << std::endl;
		fft.finish();
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fft.get_fullTex() );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	}
	
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	EXPECT_TRUE( mtxIsEqual(img, p, w, h) );
	delete[] img;
	//printFFT( img, w, h );
}

bool init() 
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

	generateInputData();

	return true;
}

GTEST_API_ int main( int argc, char **argv ) {
	init();
	printf( "Running tests\n" );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}