#include "stdafx.h"
#include <iostream>
#include "openCLFFT.h"
#include <ctime>
#include <chrono>
#include <algorithm>

cl_device_id OPENCLFFT::device = 0;
cl_context OPENCLFFT::ctx = 0;

OPENCLFFT::OPENCLFFT( unsigned int width, unsigned int height, unsigned int input_tex ):
	FFT( width, height ), fullTex( input_tex ), getChannelsProgramSize( strlen( getChannelsProgram ) ), getFullImageProgramSize( strlen( getFullImageProgram ) ),
	full_img_size( size[0] * size[1] * 4 ), channel_img_size( size[1] * (size[0] + 2) ), transformed( false ), ownsChannels(true)
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
	initCl();
	bakePlans();
	//std::cout << "group: " << local_work_size << std::endl;
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
	if ( err < 0 )
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

		std::cout << "Error::clBuildProgram" << err << std::endl;
		std::cout << program_log << std::endl;
		free( program_log );
		exit( 0 );
	}
	kernel = clCreateKernel( program, "getChannels", &err );
}

void OPENCLFFT::bakePlans()
{
	cl_int err;
	clfftDim dim = CLFFT_2D;

	/* Setup clFFT. */
	clfftSetupData fftSetup;
	err = clfftInitSetupData( &fftSetup );
	err = clfftSetup( &fftSetup );

	/* Create a default plan for a complex FFT. */
	err = clfftCreateDefaultPlan( &planHandleFFT, ctx, dim, size );
	err = clfftCreateDefaultPlan( &planHandleIFFT, ctx, dim, size );

	/* Set plan parameters. */
	err = clfftSetPlanPrecision( planHandleFFT, CLFFT_SINGLE );
	err = clfftSetLayout( planHandleFFT, CLFFT_REAL, CLFFT_HERMITIAN_INTERLEAVED );
	size_t strides[2] = { 1, size[0] + 2 };
	clfftSetPlanInStride( planHandleFFT, dim, strides );
	strides[1] = (size[0] / 2 + 1);
	clfftSetPlanOutStride( planHandleFFT, dim, strides );
	err = clfftSetResultLocation( planHandleFFT, CLFFT_INPLACE );

	err = clfftSetPlanPrecision( planHandleIFFT, CLFFT_SINGLE );
	err = clfftSetLayout( planHandleIFFT, CLFFT_HERMITIAN_INTERLEAVED, CLFFT_REAL );
	strides[1] = size[0] / 2 + 1 ;
	clfftSetPlanInStride( planHandleIFFT, dim, strides );
	strides[1] = size[0] + 2;
	clfftSetPlanOutStride( planHandleIFFT, dim, strides );
	err = clfftSetResultLocation( planHandleIFFT, CLFFT_INPLACE );

	/* Bake the plan. */
	err = clfftBakePlan( planHandleFFT, 1, &queue, NULL, NULL );
	err = clfftBakePlan( planHandleIFFT, 1, &queue, NULL, NULL );
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

		size_t ret_param_size = 0;
		err = clGetPlatformInfo( platform, CL_PLATFORM_NAME,
			sizeof( platform_name ), platform_name,
			&ret_param_size );
		//std::cout << "Platform found: " << platform_name << std::endl;

		err = clGetDeviceIDs( platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL );

		err = clGetDeviceInfo( device, CL_DEVICE_NAME,
			sizeof( device_name ), device_name,
			&ret_param_size );
		//std::cout << "Device found on the above platform: " << device_name << std::endl;

		char device_extensions[1024];
		err = clGetDeviceInfo( device, CL_DEVICE_EXTENSIONS,
			sizeof( device_extensions ), device_extensions,
			&ret_param_size );
		// std::cout << ret_param_size << "  " << device_extensions << std::endl;


		err = 0;
		props[1] = (cl_context_properties) platform;
		ctx = clCreateContext( props, 1, &device, NULL, NULL, &err );
	}
	queue = clCreateCommandQueue( ctx, device, 0, &err );

	createKernelFromSource( getChannelsProgram, getChannelsProgramSize, separatorKernel );
	createKernelFromSource( getFullImageProgram, getFullImageProgramSize, combinatorKernel );
	err = 0;
	clImgFull = clCreateFromGLTexture( ctx, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE_ARB, 0, fullTex, &err );
	if ( err < 0 )
	{
		std::cout << err << std::endl;
		switch ( err )
		{
			case CL_INVALID_CONTEXT:
			std::cout << "Error: CL_INVALID_CONTEXT for FFT" << std::endl;
			exit( 0 );
			break;
			default:
			std::cout << err << std::endl;
		}
	}
	
	//clImgFull = clCreateBuffer( ctx, CL_MEM_WRITE_ONLY, full_img_size * sizeof( float ), NULL, &err );
	clImgr = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clImgg = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clImgb = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
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
	std::cout << "******************" << std::endl;
	for ( unsigned i = 0; i < size[1] * (size[0] + 2); i++ )
		std::cout << imgToPrint[i] << " ";
	std::cout << std::endl << "******************" << std::endl;
	std::cout << std::endl;
}

void OPENCLFFT::separateChannels()
{
	cl_int err;
	//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fullTex );
	//glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	//printImg(img, "Full image");
	//err = clEnqueueWriteBuffer( queue, clImgFull, CL_TRUE, 0, full_img_size * sizeof( float ), img, 0, NULL, NULL );

	//Kenels argument settings
	err = clSetKernelArg( separatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	err = clSetKernelArg( separatorKernel, 1, sizeof( cl_mem ), &clImgr );
	err = clSetKernelArg( separatorKernel, 2, sizeof( cl_mem ), &clImgg );
	err = clSetKernelArg( separatorKernel, 3, sizeof( cl_mem ), &clImgb );
	err = clSetKernelArg( separatorKernel, 4, sizeof( int ), (void*) &size[0] );

	err = clEnqueueNDRangeKernel( queue, separatorKernel, work_dim, &global_work_offset, &full_img_size, &local_work_size, 0, NULL, NULL );
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
	cl_int err;

	//Kenels argument settings
	err = clSetKernelArg( combinatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	err = clSetKernelArg( combinatorKernel, 1, sizeof( cl_mem ), &clImgr );
	err = clSetKernelArg( combinatorKernel, 2, sizeof( cl_mem ), &clImgg );
	err = clSetKernelArg( combinatorKernel, 3, sizeof( cl_mem ), &clImgb );
	err = clSetKernelArg( combinatorKernel, 4, sizeof( int ), (void*) &size[0] );
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
	err = clEnqueueNDRangeKernel( queue, combinatorKernel, work_dim, &global_work_offset, &channel_img_size, &local_work_size, 0, NULL, NULL );

	// Read result
	//err = clEnqueueReadBuffer( queue, clImgFull, CL_TRUE, 0, full_img_size * sizeof( float ), img, 0, NULL, NULL );
	//printImg( img, "Combined Image" );
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

	/* Prepare OpenCL memory objects and place data inside them. */
	//bufX = clCreateFromGLTexture( ctx, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE_ARB, 0, fullText, &glerr );
	//bufX = clCreateBuffer( ctx, CL_MEM_READ_WRITE, img_size, NULL, &err );

	//err = clEnqueueWriteBuffer( queue, bufX, CL_TRUE, 0, img_size, img, 0, NULL, NULL );



	/* Fetch results of calculations. */
	//err = clEnqueueReadBuffer( queue, bufX, CL_TRUE, 0, buffer_size, X, 0, NULL, NULL );

	/*for ( unsigned i = 0; i < size[0] * 4 * size[1]; i += 4 )
	{
	if ( i % (4 * size[0]) == 0 )
	std::cout << std::endl;
	std::cout << "(" << X[i] << ", " << X[i + 1] << ", " << X[i + 2] << ", " << X[i + 3] << ")";
	}
	std::cout << std::endl;
	std::cout << std::endl;*/

	// doesn't need to copy from host memory
	//err = clEnqueueWriteBuffer( queue, bufX, CL_TRUE, 0, buffer_size, X, 0, NULL, NULL );

	/* FFT library realted declarations */
	

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
	
	//auto start = std::chrono::system_clock::now();
	
	separateChannels();
	
	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgr, NULL, NULL );
	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgg, NULL, NULL );
	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgb, NULL, NULL );
	
	//clFinish( queue );
	transformed = true;

	// Read result
	/*res = new float[channel_img_size];
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "r channel", 1 );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "g channel", 1 );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	printImg( res, "b channel", 1 );
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