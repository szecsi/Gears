#ifndef OPENCLFFT_H
#define OPENCLFFT_H

#include <windows.h>  
#include <functional>
#include "FFT.h"
#include <cstring>
#include <GL/glew.h>
#include "openCLCore.h"

class OPENCLFFT: public FFT
{
public:
	OPENCLFFT( unsigned int width, unsigned int height, unsigned int input_tex = 0 );
	~OPENCLFFT();

	virtual void do_fft( FFTChannelMode channelMode = FFTChannelMode::Monochrome ) override;
	virtual void do_inverse_fft();
	virtual unsigned int get_fullTex() const override;
	void get_channels( cl_mem& r, cl_mem& g, cl_mem& b ) const;
	void take_channels( cl_mem& r, cl_mem& g, cl_mem& b );
	virtual unsigned int take_fullTex_ownership() override;
	virtual void redraw_input() override;
	virtual bool storeFrequencyInTexture() const override { return false; }
	void finish() { if ( queue ) clFinish( queue ); }

	const char* separateChannelsProgram =R"(
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

protected:
	unsigned int fullTex;
	unsigned short has_input_tex;
	bool transformed;
	bool ownsChannels;
	FFTChannelMode fftMode = FFTChannelMode::Monochrome;

	cl_mem clImgr = nullptr;
	cl_mem clImgg = nullptr;
	cl_mem clImgb = nullptr;
	cl_mem clImgFull = nullptr;
	
	cl_context ctx;
	cl_device_id device;
	cl_command_queue queue = 0;

	clfftPlanHandle planHandleFFT;
	clfftPlanHandle planHandleIFFT;

	size_t full_img_size;
	size_t channel_img_size;

	//Enqueue the kernel for execution
	cl_uint work_dim = 1;
	size_t global_work_offset = 0;
	size_t global_work_size[1];
	size_t local_work_size[1];
	
	size_t origin[3];
	size_t region[3];

	GLfloat *img = nullptr;

	void init_framebuffer();
	void bakePlans();

	void initCl();
	void separateChannels( FFTChannelMode channelMode );
	void combineChannels();
};

#endif // OPENCLFFT_H
