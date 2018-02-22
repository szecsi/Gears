#include "stdafx.h"
#include <iostream>
#include "openCLFFT.h"
#include <ctime>
#include <chrono>
#include <algorithm>

cl_device_id OPENCLFFT::device = 0;
cl_context OPENCLFFT::ctx = 0;

#ifdef _DEBUG
void printPixel( float* img, unsigned& idx, unsigned channels, bool complex )
{
	std::cout << "(";
	for ( unsigned k = 0; k < channels; k++ )
	{
		std::cout << img[idx++];
		if ( complex )
			std::cout << "+" << img[idx++] << "i";
		if ( k < channels - 1 )
			std::cout << ", ";
	}
	std::cout << ") ";
}
#endif

void printImgTest( float* img, unsigned w, unsigned h, const char* name = "FFT", unsigned channels = 4, bool complex = false, unsigned pad = 0 )
{
#ifdef _DEBUG
	if ( w > 10 )
		return;
	unsigned idx = 0;
	unsigned rowWidth = complex ? w / 2 + 1 : w;
	std::cout << name << ": " << std::endl;
	for ( unsigned i = 0; i < h; i++ )
	{
		for ( unsigned j = 0; j < rowWidth; j++ )
		{
			printPixel( img, idx, channels, complex );
		}
		if ( pad )
		{
			std::cout << "| ";
			for ( unsigned k = 0; k < pad; k++ )
			{
				printPixel( img, idx, channels, complex );
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << std::endl;
#endif
}

void clPrintError(cl_int& errorCode)
{
#ifdef _DEBUG
	if ( !errorCode )
		return;

	std::cout <<  std::endl << "*********************************************************************************************" << std::endl << "** Error: ";
	switch ( errorCode )
	{
		case CL_INVALID_CONTEXT:
			std::cout << "Invalid cl context";
			break;
		case CL_INVALID_MEM_OBJECT:
			std::cout << "Invalid cl memory object";
			break;
		case CL_INVALID_WORK_GROUP_SIZE:
			std::cout << "Invalid cl work group size";
			break;
		case CL_INVALID_KERNEL_ARGS:
			std::cout << "Invalid cl kernel args";
			break;
		case CL_INVALID_VALUE:
			std::cout << "Invalid cl value";
			break;
		case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
			std::cout << "Invalid gl sharegroup reference";
			break;
			
	}
	std::cout << " (" << errorCode << ") " << std::endl << "*********************************************************************************************" << std::endl;
	errorCode = 0;
#endif
}

OPENCLFFT::OPENCLFFT( unsigned int width, unsigned int height, unsigned int input_tex ):
	FFT( width, height ), fullTex( input_tex ), getChannelsProgramSize( strlen( getChannelsProgram ) ), getFullImageProgramSize( strlen( getFullImageProgram ) ),
	full_img_size( size[0] * size[1] * 4 ), channel_img_size( size[1] * (size[0] + 2) ), transformed( false ), ownsChannels( true )
{
	has_input_tex = glIsTexture( input_tex );
	init_framebuffer();
	img = new GLfloat[full_img_size];
	props[0] = CL_CONTEXT_PLATFORM;
	props[1] = 0;
	props[2] = CL_GL_CONTEXT_KHR;
	props[3] = (cl_context_properties) wglGetCurrentContext();
	props[4] = CL_WGL_HDC_KHR;
	props[5] = (cl_context_properties) GetDC(GetActiveWindow());
	props[6] = 0;

	for ( size_t i = 0; i < 3; i++ )
	{
		origin[i] = 0;
		region[i] = 1;
	}

	global_work_size[0] = full_img_size / 4 ;
	local_work_size[0] = global_work_size[0] / 4;

	region[0] = size[0];
	region[1] = size[1];

	initCl();
	bakePlans();
}

OPENCLFFT::~OPENCLFFT()
{
	if ( ownsTex )
		glDeleteTextures( 1, &fullTex );
	delete[] img;

	/* Release the plan. */
	clfftDestroyPlan( &planHandleFFT );
	clfftDestroyPlan( &planHandleIFFT );

	/* Release clFFT library. */
	clfftTeardown();

	if ( clImgFull )
		clReleaseMemObject( clImgFull );
	if ( ownsChannels )
	{
		if ( clImgr )
			clReleaseMemObject( clImgr );
		if ( clImgg )
			clReleaseMemObject( clImgg );
		if ( clImgb )
			clReleaseMemObject( clImgb );
	}
	/* Release OpenCL working objects. */
	clReleaseCommandQueue( queue );
}

void OPENCLFFT::init_framebuffer()
{
	if ( !has_input_tex )
	{
		glGenTextures( 1, &fullTex );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fullTex );
		glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, size[0], size[1], 0, GL_RGBA, GL_FLOAT, 0 );
	}
	glGenFramebuffersEXT( 1, &fbo );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, fullTex, 0 );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
}

void OPENCLFFT::createKernelFromSource( const char* kernelSource, size_t kernelSourceSize, cl_kernel& kernel )
{
	// Build opencl program
	cl_int err;
	char *program_log;
	size_t log_size;

	cl_program program = clCreateProgramWithSource(
		ctx, 1, (const char**) &kernelSource, &kernelSourceSize, &err );
	if ( err < 0 )
	{
		std::cout << "Error::clCreateProgramWithSource" << err << std::endl;
		exit( 0 );
	}

	err = clBuildProgram( program, 0, NULL, NULL, NULL, NULL );
	if ( err )
	{
		clGetProgramBuildInfo(
			program,
			device,
			CL_PROGRAM_BUILD_LOG,
			0,
			NULL,
			&log_size
		);

		program_log = (char*) malloc( log_size + 1 );

		program_log[log_size] = '\0';

		clGetProgramBuildInfo(
			program,
			device,
			CL_PROGRAM_BUILD_LOG,
			log_size + 1,
			program_log,
			NULL
		);

		clPrintError( err );
		std::cout << program_log << std::endl;
		free( program_log );
		exit( 0 );
	}
	kernel = clCreateKernel( program, "getChannels", &err );
	if ( err )
	{
		clPrintError( err );
		exit( 0 );
	}
}

void OPENCLFFT::bakePlans()
{
	cl_int err;
	clfftDim dim = CLFFT_2D;

	float* res;
	res = new GLfloat[full_img_size];

	/* Setup clFFT. */
	clfftSetupData fftSetup;
	err = clfftInitSetupData( &fftSetup );
	clPrintError( err );
	err = clfftSetup( &fftSetup );
	clPrintError( err );

	/* Create a default plan for a complex FFT. */
	err = clfftCreateDefaultPlan( &planHandleFFT, ctx, dim, size );
	clPrintError( err );
	err = clfftCreateDefaultPlan( &planHandleIFFT, ctx, dim, size );
	clPrintError( err );

	/* Set plan parameters. */
	err = clfftSetPlanPrecision( planHandleFFT, CLFFT_SINGLE );
	clPrintError( err );
	err = clfftSetLayout( planHandleFFT, CLFFT_REAL, CLFFT_HERMITIAN_INTERLEAVED );
	clPrintError( err );
	size_t strides[2] = { 1, size[0] + 2 };
	clfftSetPlanInStride( planHandleFFT, dim, strides );
	strides[1] = (size[0] / 2 + 1);
	clfftSetPlanOutStride( planHandleFFT, dim, strides );
	err = clfftSetResultLocation( planHandleFFT, CLFFT_INPLACE );
	clPrintError( err );

	err = clfftSetPlanPrecision( planHandleIFFT, CLFFT_SINGLE );
	clPrintError( err );
	err = clfftSetLayout( planHandleIFFT, CLFFT_HERMITIAN_INTERLEAVED, CLFFT_REAL );
	clPrintError( err );
	strides[1] = size[0] / 2 + 1 ;
	clfftSetPlanInStride( planHandleIFFT, dim, strides );
	strides[1] = size[0] + 2;
	clfftSetPlanOutStride( planHandleIFFT, dim, strides );
	err = clfftSetResultLocation( planHandleIFFT, CLFFT_INPLACE );
	clPrintError( err );

	/* Bake the plan. */
	err = clfftBakePlan( planHandleFFT, 1, &queue, NULL, NULL );
	clPrintError( err );
	err = clfftBakePlan( planHandleIFFT, 1, &queue, NULL, NULL );
	clPrintError( err );

}

void OPENCLFFT::initCl()
{
	cl_int err;
	cl_platform_id platform = 0;

	char platform_name[128];
	char device_name[128];

	/* Setup OpenCL environment. */
	if ( ctx == 0 )
	{
		err = clGetPlatformIDs( 1, &platform, NULL );
		clPrintError( err );

		size_t ret_param_size = 0;
		err = clGetPlatformInfo( platform, CL_PLATFORM_NAME,
			sizeof( platform_name ), platform_name,
			&ret_param_size );
		clPrintError( err );
		//std::cout << "Platform found: " << platform_name << std::endl;

		err = clGetDeviceIDs( platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL );
		clPrintError( err );

		err = clGetDeviceInfo( device, CL_DEVICE_NAME,
			sizeof( device_name ), device_name,
			&ret_param_size );
		clPrintError( err );
		//std::cout << "Device found on the above platform: " << device_name << std::endl;

		char device_extensions[1024];
		err = clGetDeviceInfo( device, CL_DEVICE_EXTENSIONS,
			sizeof( device_extensions ), device_extensions,
			&ret_param_size );
		clPrintError( err );
		// std::cout << ret_param_size << "  " << device_extensions << std::endl;

		props[1] = (cl_context_properties) platform;
		ctx = clCreateContext( props, 1, &device, NULL, NULL, &err );
		clPrintError( err );
	}
	queue = clCreateCommandQueue( ctx, device, 0, &err );
	clPrintError( err );

	createKernelFromSource( getChannelsProgram, getChannelsProgramSize, separatorKernel );
	createKernelFromSource( getFullImageProgram, getFullImageProgramSize, combinatorKernel );

	clImgFull = clCreateFromGLTexture( ctx, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE_ARB, 0, fullTex, &err );
	clPrintError( err );

	/*float* res = new float[400];
	glFinish();
	err = clEnqueueAcquireGLObjects( queue, 1, &clImgFull, 0, 0, NULL );
	clPrintError( err );
	err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, 10 * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, 10, 10, "Full Image in cl mem" );
	clPrintError( err );

	
	clImgr = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );
	clImgg = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );
	clImgb = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );

	err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, 10 * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if( !err )
		printImgTest( res, 10, 10, "Full img in cl_mem" );
	clPrintError( err );

	clFinish( queue );
	clEnqueueReleaseGLObjects( queue, 1, &clImgFull, 0, 0, NULL );

	delete[] res;*/
}

void OPENCLFFT::printImg( float* imgToPrint, const char* imgName, unsigned channelNum )
{
	size_t imgsize = (size[0] + (channelNum == 1 ? 2 : 0)) * size[1] * channelNum;
	std::cout << std::endl << imgName << ":" << std::endl;
	size_t index = 0;
	for ( size_t i = 0; index < imgsize; i += channelNum, index++ )
	{
		if ( transformed )
		{
			std::cout << "(";
			for ( size_t k = 0; k < channelNum * 2; k+=2 )
			{
				std::cout << imgToPrint[index + k] << "+" << imgToPrint[index + k + 1] << "i   ";
			}
			std::cout << ") ";
			i+=channelNum*2-1;
			index+=channelNum*2-1;
		}
		else
		{
			std::cout << "(";
			for ( size_t k = 0; k < channelNum; ++k )
			{
				std::cout << imgToPrint[index + k] << " ";
			}
			std::cout << ") ";
			if ( channelNum == 1 && ((i % size[0]) == size[0] - 1) )
			{
				index += 2;
				std::cout << std::endl;
			}
		}
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

void OPENCLFFT::separateChannels()
{
	cl_int err;
	glFinish();
	clEnqueueAcquireGLObjects( queue, 1, &clImgFull, 0, 0, NULL );

	//Kenels argument settings
	err = clSetKernelArg( separatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	clPrintError( err );
	err = clSetKernelArg( separatorKernel, 1, sizeof( cl_mem ), &clImgr );
	clPrintError( err );
	err = clSetKernelArg( separatorKernel, 2, sizeof( cl_mem ), &clImgg );
	clPrintError( err );
	err = clSetKernelArg( separatorKernel, 3, sizeof( cl_mem ), &clImgb );
	clPrintError( err );
	err = clSetKernelArg( separatorKernel, 4, sizeof( int ), (void*) &size[0] );
	clPrintError( err );
	err = clEnqueueNDRangeKernel( queue, separatorKernel, work_dim, &global_work_offset, global_work_size, local_work_size, 0, NULL, NULL );
	clPrintError( err );

	clFinish( queue );
	clEnqueueReleaseGLObjects( queue, 1, &clImgFull, 0, 0, NULL );
	// Read result
	/*float* res;
	res = new float[channel_img_size];
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "r channel", 1 );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "g channel", 1 );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "b channel", 1 );

	delete[] res;*/
}

void OPENCLFFT::combineChannels()
{
	cl_int err = 0;

	/*float* res;
	res = new float[full_img_size];*/

	glFinish();
	clEnqueueAcquireGLObjects( queue, 1, &clImgFull, 0, 0, NULL );

	/*err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, size[0] * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1] );
	clPrintError( err );

	// Read inverse fourier transform per channel
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "r channel", 1, false, 2 );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "g channel", 1, false, 2 );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "b channel", 1, false, 2 );
	clPrintError( err );*/

	//Kenels argument settings
	err = clSetKernelArg( combinatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	clPrintError( err );
	err = clSetKernelArg( combinatorKernel, 1, sizeof( cl_mem ), &clImgr );
	clPrintError( err );
	err = clSetKernelArg( combinatorKernel, 2, sizeof( cl_mem ), &clImgg );
	clPrintError( err );
	err = clSetKernelArg( combinatorKernel, 3, sizeof( cl_mem ), &clImgb );
	clPrintError( err );
	err = clSetKernelArg( combinatorKernel, 4, sizeof( int ), (void*) &size[0] );
	clPrintError( err );

	//delete[] res;
	
	err = clEnqueueNDRangeKernel( queue, combinatorKernel, work_dim, &global_work_offset, global_work_size, local_work_size, 0, NULL, NULL );
	clPrintError( err );

	// Read result
	/*err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, size[0] * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "Full image after combine channels in cl mem" );
	clPrintError( err );*/

	clFinish( queue );
	clEnqueueReleaseGLObjects( queue, 1, &clImgFull, 0, 0, NULL );

	/*glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fullTex );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, res );
	printImgTest( res, size[0], size[1], "Full image after combine channels in texture" );*/
}

void OPENCLFFT::do_inverse_fft()
{
	if ( !transformed )
		return;
	clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgr, NULL, NULL );
	clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgg, NULL, NULL );
	clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgb, NULL, NULL );
	
	//clFinish( queue );
	transformed = false;

	// Read result
	/*float* res;
	res = new float[channel_img_size];
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "r channel", 1 );
	for ( unsigned i = 0; i < size[1] * (size[0] + 2); i++ )
		std::cout << res[i] << " ";
	std::cout << std::endl;
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "g channel", 1 );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "b channel", 1 );
	delete[] res;*/

	/* Combine channels and write it two texture */
	combineChannels();
	//glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, size[0], size[1], 0, GL_RGBA, GL_FLOAT, img );
	//auto iend = std::chrono::system_clock::now();
}

void OPENCLFFT::do_fft()
{
	if ( !fullTex || transformed )
		return;
	if ( !redrawn )
		redraw_input();
	
	separateChannels();

	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgr, NULL, NULL );
	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgg, NULL, NULL );
	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgb, NULL, NULL );
	
	transformed = true;

	/*cl_int err;
	float* res = new float[full_img_size];
	// Read result
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "r channel", 1, true );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "g channel", 1, true );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImgTest( res, size[0], size[1], "b channel", 1, true );
	clPrintError( err );

	delete[] res;*/
}

void OPENCLFFT::get_channels( cl_mem& r, cl_mem& g, cl_mem& b ) const
{
	r = clImgr;
	g = clImgg;
	b = clImgb;
}

unsigned int OPENCLFFT::get_fullTex() const
{
	return fullTex;
}

unsigned int OPENCLFFT::take_fullTex_ownership()
{
	ownsTex = false;
	return fullTex;
}

void OPENCLFFT::take_channels( cl_mem& r, cl_mem& g, cl_mem& b )
{
	r = clImgr;
	g = clImgg;
	b = clImgb;
	ownsChannels = false;
}

void OPENCLFFT::redraw_input()
{
	if ( draw_input )
	{
		int vp[4];
		glGetIntegerv( GL_VIEWPORT, vp );
		glViewport( 0, 0, size[0] + 1, size[1] + 1 );

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo );
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
		draw_input();
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

		glViewport( vp[0], vp[1], vp[2], vp[3] );
		redrawn = true;
	}
}