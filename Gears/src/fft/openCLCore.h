#pragma once

#include <iostream>
#include <windows.h>
#include <cstring>
#include <GL/glew.h>
#include <CL/cl_gl.h>
#include <map>
#include <string>

struct OpenCLKernelEntry
{
	const char* source;
	size_t sourceSize;
	cl_kernel kernel = nullptr;
};

namespace ImageHelper
{
	void printImg( float* img, unsigned w, unsigned h, const char* name = "FFT", unsigned channels = 4, bool complex = false, unsigned pad = 0 );
}

namespace OpenCLHelper
{
	void clPrintError( cl_int& errorCode );
}

class OpenCLCore
{
	OpenCLCore();
	static OpenCLCore* _instance;

	std::map<std::string, OpenCLKernelEntry> kernels;

public:
	cl_context ctx = nullptr;
	cl_device_id device = nullptr;
	cl_command_queue queue = 0;

	static OpenCLCore* Get();
	static cl_kernel GetKernel( std::string name );
	static void RegistKernel( std::string name, const char* source, size_t sourceSize = 0, bool compile = false );
	static cl_kernel CompileKernel( const char* name, const char* source, size_t sourceSize = 0 );
	static void MultiplyFFT( cl_mem lhs, cl_mem rhs, size_t* global_work_size, size_t* local_work_size = nullptr );
	static void MultiplyFFT( cl_mem lhsr, cl_mem lhsg, cl_mem lhsb, cl_mem rhsr, cl_mem rhsg, cl_mem rhsb, size_t* global_work_size, size_t* local_work_size = nullptr );
	static void Destroy();
};