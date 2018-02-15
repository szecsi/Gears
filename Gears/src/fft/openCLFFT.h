#ifndef OPENCLFFT_H
#define OPENCLFFT_H

#include <windows.h>  
#include <functional>
#include "FFT.h"
#include <cstring>
#include <GL/glew.h>
#include "CL/cl_gl.h"
#include <clFFT.h>

class OPENCLFFT: public FFT
{
public:
	OPENCLFFT( unsigned int width, unsigned int height, unsigned int input_tex = 0 );
	~OPENCLFFT();

	virtual void do_fft() override;
	virtual void do_inverse_fft();
	virtual unsigned int get_fullTex() const override;
	void get_channels(cl_mem& r, cl_mem& g, cl_mem& b) const;
	void take_channels( cl_mem& r, cl_mem& g, cl_mem& b );
	virtual unsigned int take_fullTex_ownership() override;
	virtual void redraw_input() override;
	virtual bool storeFrequencyInTexture() const override { return false; }
	void finish() { if ( queue ) clFinish( queue ); }

	const char* getChannelsProgram =
		"__kernel void getChannels(\n"
		"__global float* fullImg,\n"
		"__global float* imgr,\n"
		"__global float* imgg,\n"
		"__global float* imgb,\n"
		"const int w)\n"
		"{\n"
		"int i = get_global_id( 0 );\n"
		"int ci = i / 4;"
		"int channelIndex = ci/w*(w+2)+(ci%w);\n"
		"imgr[channelIndex] = fullImg[i/4*4];\n"
		"imgg[channelIndex] = fullImg[i/4*4 + 1];\n"
		"imgb[channelIndex] = fullImg[i/4*4 + 2];\n"
		"}\n";
	const unsigned getChannelsProgramSize;

	const char* getFullImageProgram =
		"__kernel void getChannels(\n"
		"__global float* fullImg,\n"
		"__global float* imgr,\n"
		"__global float* imgg,\n"
		"__global float* imgb,\n"
		"const int w)\n"
		"{\n"
		"int i = get_global_id( 0 );\n"
		"int index = i + (i/w)*2;\n"
		"fullImg[i*4] = imgr[index];\n"
		"fullImg[i*4 + 1] = imgg[index];\n"
		"fullImg[i*4 + 2] = imgb[index];\n"
		"fullImg[i*4 + 3] = 1;\n"
		"}\n";
	const unsigned getFullImageProgramSize;

protected:
	unsigned int fullTex;
	bool has_input_tex;
	bool transformed;
	bool ownsChannels;

	static cl_device_id device;
	cl_context_properties props[7];
	static cl_context ctx;
	cl_command_queue queue = 0;
	cl_mem clImgr = nullptr;
	cl_mem clImgg = nullptr;
	cl_mem clImgb = nullptr;
	cl_mem clImgFull = nullptr;

	cl_kernel separatorKernel;
	cl_kernel combinatorKernel;

	clfftPlanHandle planHandleFFT;
	clfftPlanHandle planHandleIFFT;

	size_t full_img_size;
	size_t channel_img_size;

	//Enqueue the kernel for execution
	cl_uint work_dim = 1;
	size_t global_work_offset = 0;
	size_t local_work_size = CL_DEVICE_MAX_WORK_GROUP_SIZE / 16 * 16;

	GLfloat *img = nullptr;

	void init_framebuffer();
	void printImg( float* imgToPrint, const char* imgname, unsigned channelNum = 4 );
	void createKernelFromSource( const char* kernelSource, size_t kernelSourceSize, cl_kernel& kernel );
	void bakePlans();

	void initCl();
	void separateChannels();
	void combineChannels();
};

#endif // OPENCLFFT_H
