#include "stdafx.h"
#include <iostream>
#include "openCLFFT.h"
#include <ctime>
#include <chrono>
#include <algorithm>

const char* separateChannelsProgram = R"(
		__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
		__kernel void separateChannels(
		  __read_only image2d_t fullImg,
		  const int w,
		  __global float* imgr,
		  __global float* imgg,
		  __global float* imgb)
		  {
			int i = get_global_id( 0 );
			int channelIndex = (i/w)*(w+2)+(i%w);
			imgr[channelIndex] = read_imagef( fullImg, sampler, (int2)(i / w, i % w) ).x;
			imgg[channelIndex] = read_imagef( fullImg, sampler, (int2)(i / w, i % w) ).y;
			imgb[channelIndex] = read_imagef( fullImg, sampler, (int2)(i / w, i % w) ).z;
		  })";

const char* separateChannelsMonoProgram = R"(
		__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE	| CLK_FILTER_NEAREST;
		__kernel void separateChannelsMono(
		  __read_only image2d_t fullImg,
		  const int w,
		  __global float* imgr)
		  {
			int i = get_global_id( 0 );
			int channelIndex = (i/w)*(w+2)+(i%w);
			imgr[channelIndex] = read_imagef( fullImg, sampler, (int2)(i / w, i % w) ).x;
		  })";

const char* combineChannelsProgram = R"(
		__kernel void combineChannels(
		  __write_only image2d_t fullImg,
		  const int w,
		  __global float* imgr,
		  __global float* imgg,
		  __global float* imgb)
		  {
			int i = get_global_id( 0 );
			int index = i + (i/w)*2;
			write_imagef( fullImg, (int2)(i/w, i%w), (float4)(imgr[index], imgg[index], imgb[index], 1.0f) );
		  })";

const char* combineChannelsMonoProgram = R"(
		__kernel void combineChannelsMono(
		  __write_only image2d_t fullImg,
		  const int w,
		  __global float* imgr)
		  {
			int i = get_global_id( 0 );
			int index = i + (i/w)*2;
			write_imagef( fullImg, (int2)(i/w, i%w), (float4)(imgr[index], imgr[index], imgr[index], 1.0f) );
		  })";

OPENCLFFT::OPENCLFFT( unsigned int width, unsigned int height, unsigned int input_tex ):
	FFT( width, height ), fullTex( input_tex ),
	full_img_size( size[0] * size[1] * 4 ), channel_img_size( size[1] * (size[0] + 2) ), transformed( false ), ownsChannels( true )
{
	ownsTex = false;
	has_input_tex = glIsTexture( input_tex );
	init_framebuffer();
	initCl();

	for ( size_t i = 0; i < 3; i++ )
	{
		origin[i] = 0;
		region[i] = 1;
	}

	global_work_size[0] = full_img_size / 4;
	
	local_work_size[0] = (64 < global_work_size[0] ? 64 : 4);

	region[0] = size[0];
	region[1] = size[1];

	bakePlans();
}

OPENCLFFT::~OPENCLFFT()
{
	clReleaseMemObject(clImgFull);
	if ( ownsTex )
		glDeleteTextures( 1, &fullTex );

	/* Release the plan. */
	clfftDestroyPlan( &planHandleFFT );
	clfftDestroyPlan( &planHandleIFFT );

	if ( ownsChannels )
	{
		if ( clImgr )
			clReleaseMemObject( clImgr );
		if ( clImgg )
			clReleaseMemObject( clImgg );
		if ( clImgb )
			clReleaseMemObject( clImgb );
	}
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

void OPENCLFFT::bakePlans()
{
	using OpenCLHelper::clPrintError;

	cl_int err;
	clfftDim dim = CLFFT_2D;

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

void OPENCLFFT::staticInit()
{
	OpenCLCore::Get()->RegistKernel("separateChannels", separateChannelsProgram, 0, true);
	OpenCLCore::Get()->RegistKernel("combineChannels", combineChannelsProgram, 0, true);
	OpenCLCore::Get()->RegistKernel("separateChannelsMono", separateChannelsMonoProgram, 0, true);
	OpenCLCore::Get()->RegistKernel("combineChannelsMono", combineChannelsMonoProgram, 0, true);
}

void OPENCLFFT::initCl()
{
	using OpenCLHelper::clPrintError;

	cl_int err;

	ctx = OpenCLCore::Get()->ctx;
	device = OpenCLCore::Get()->device;

	queue = OpenCLCore::Get()->createCommandQueue();

	clImgFull = clCreateFromGLTexture(ctx, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE_ARB, 0, fullTex, &err);
	clPrintError(err);

	clImgr = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );
	clImgg = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );
	clImgb = clCreateBuffer( ctx, CL_MEM_READ_ONLY, channel_img_size * sizeof( float ), NULL, &err );
	clPrintError( err );
}

void OPENCLFFT::separateChannels( FFTChannelMode channelMode )
{

	using OpenCLHelper::clPrintError;

	cl_int err;

	//float* res;
	//res = new float[full_img_size];

	//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fullTex );
	//glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, res );
	//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	//ImageHelper::printImg( res, size[0], size[1] );

	//delete[] res;

	cl_kernel separatorKernel = nullptr; 
	switch ( channelMode )
	{
		case FFTChannelMode::Monochrome:
			separatorKernel = OpenCLCore::Get()->GetKernel( "separateChannelsMono" );
		break;
		case FFTChannelMode::Multichrome:
			separatorKernel = OpenCLCore::Get()->GetKernel( "separateChannels" );
		break;
		default:
			return;
	}
	if ( !separatorKernel )
		return;

	err = clSetKernelArg( separatorKernel, 1, sizeof( int ), (void*) &size[0] );
	clPrintError( err );
	err = clSetKernelArg( separatorKernel, 2, sizeof( cl_mem ), &clImgr );
	clPrintError( err );
	if ( channelMode == FFTChannelMode::Multichrome )
	{
		err = clSetKernelArg( separatorKernel, 3, sizeof( cl_mem ), &clImgg );
		clPrintError( err );
		err = clSetKernelArg( separatorKernel, 4, sizeof( cl_mem ), &clImgb );
		clPrintError( err );
	}
	clEnqueueAcquireGLObjects( queue, 1, &clImgFull, 0, 0, NULL );
	hasImageObject = true;

	//Kenels argument settings
	err = clSetKernelArg( separatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	clPrintError( err );
	err = clEnqueueNDRangeKernel( queue, separatorKernel, work_dim, &global_work_offset, global_work_size, local_work_size, 0, NULL, NULL );
	clPrintError( err );
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
	using OpenCLHelper::clPrintError;

	cl_int err = 0;

	/*float* res;
	res = new float[full_img_size];*/

	/*err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, size[0] * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1] );
	clPrintError( err );

	// Read inverse fourier transform per channel
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "r channel", 1, false, 2 );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "g channel", 1, false, 2 );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "b channel", 1, false, 2 );
	clPrintError( err );*/
	
	auto start = std::chrono::system_clock::now();

	cl_kernel combinatorKernel = nullptr;
	switch ( fftMode )
	{
		case FFTChannelMode::Monochrome:
		combinatorKernel = OpenCLCore::Get()->GetKernel( "combineChannelsMono" );
		break;
		case FFTChannelMode::Multichrome:
			combinatorKernel = OpenCLCore::Get()->GetKernel( "combineChannels" );
		break;
	}
	if ( !combinatorKernel )
		return;

	//Kenels argument settings
	err = clSetKernelArg( combinatorKernel, 1, sizeof( int ), (void*) &size[0] );
	clPrintError( err );
	err = clSetKernelArg( combinatorKernel, 2, sizeof( cl_mem ), &clImgr );
	clPrintError( err );

	if ( fftMode == FFTChannelMode::Multichrome )
	{
		err = clSetKernelArg( combinatorKernel, 3, sizeof( cl_mem ), &clImgg );
		clPrintError( err );
		err = clSetKernelArg( combinatorKernel, 4, sizeof( cl_mem ), &clImgb );
		clPrintError( err );
	}

	err = clSetKernelArg( combinatorKernel, 0, sizeof( cl_mem ), &clImgFull );
	clPrintError( err );
	err = clEnqueueNDRangeKernel( queue, combinatorKernel, work_dim, &global_work_offset, global_work_size, local_work_size, 0, NULL, NULL );
	clPrintError( err );

	// Read result
	/*err = clEnqueueReadImage( queue, clImgFull, CL_TRUE, origin, region, size[0] * 4 * sizeof( float ), 0, res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "Full image after combine channels in cl mem" );
	clPrintError( err );*/

	// delete[] res;

	auto clEnd = std::chrono::system_clock::now();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsedSeconds = clEnd - start;
	/*std::cout << "          Combined channels time until release gl: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	elapsedSeconds = finishEnd - start;
	std::cout << "          Combined channels time after clFinish: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	elapsedSeconds = releaseEnd - start;
	std::cout << "          Combined channels time until release mem obj: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	elapsedSeconds = end - start;
	std::cout << "          Combined channels time: "<< elapsedSeconds.count() * 1000 << "ms." << std::endl;*/

	/*glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fullTex );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, res );
	printImg( res, size[0], size[1], "Full image after combine channels in texture" );*/
}

void OPENCLFFT::do_inverse_fft()
{
	if ( !transformed )
		return;
	auto start = std::chrono::system_clock::now();
	clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgr, NULL, NULL );
	if ( fftMode == FFTChannelMode::Multichrome )
	{
		clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgg, NULL, NULL );
		clfftEnqueueTransform( planHandleIFFT, CLFFT_BACKWARD, 1, &queue, 0, NULL, NULL, &clImgb, NULL, NULL );
	}
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
	auto enqueueEnd = std::chrono::system_clock::now();
	combineChannels();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsedSeconds = enqueueEnd - start;
	/*std::cout << "      Inverse FFT before combined time for clFFT: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	elapsedSeconds = end - start;
	std::cout << "      Full inverse FFT time for clFFT: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;*/
}

void OPENCLFFT::finishConv()
{
	clFinish(queue);
	clEnqueueReleaseGLObjects(queue, 1, &clImgFull, 0, 0, NULL);
	hasImageObject = false;
}

void OPENCLFFT::do_fft( FFTChannelMode channelMode )
{
	if ( !fullTex || transformed )
		return;
	if ( !redrawn )
		redraw_input();

	auto start = std::chrono::system_clock::now();
	separateChannels( channelMode );

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> selapsedSeconds = end - start;

	start = std::chrono::system_clock::now();

	clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgr, NULL, NULL );
	if ( channelMode == FFTChannelMode::Multichrome )
	{
		clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgg, NULL, NULL );
		clfftEnqueueTransform( planHandleFFT, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &clImgb, NULL, NULL );
	}
	fftMode = channelMode;
	transformed = true;

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsedSeconds = end - start;

	/*std::cout << "        Length of separateChannels: " << selapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << "        Length of fft: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;*/

	/*cl_int err;
	float* res = new float[full_img_size];
	// Read result
	err = clEnqueueReadBuffer( queue, clImgr, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "r channel", 1, true );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgg, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "g channel", 1, true );
	clPrintError( err );
	err = clEnqueueReadBuffer( queue, clImgb, CL_TRUE, 0, channel_img_size * sizeof( float ), res, 0, NULL, NULL );
	if ( !err )
		printImg( res, size[0], size[1], "b channel", 1, true );
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
	redrawn = true;
	if ( draw_input )
	{
		int vp[4];
		glGetIntegerv( GL_VIEWPORT, vp );
		glViewport( 0, 0, size[0] + 1, size[1] + 1 );

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo );
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
		draw_input(1);
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

		glViewport( vp[0], vp[1], vp[2], vp[3] );
		redrawn = true;
	}
}