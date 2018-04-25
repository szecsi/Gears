#include "TestGLCommon.h"

#include "fft/glFFT.cpp"
#include "fft/imageHelper.cpp"
#include "fft/OpenCLHelper.cpp"
#include "fft/openCLCore.cpp"
#include "fft/openCLFFT.cpp"
#include "fft/load_shaders.cpp"

const unsigned runNumber = 1;
const size_t texNum = 3;
unsigned textures[texNum];
float* p = nullptr;
unsigned w, h;

const float gauss8x8[64] =
{
	0.0000f,    0.0000f,    0.0000f,    0.0001f,    0.0001f,    0.0000f,    0.0000f,    0.0000f,
	0.0000f,    0.0001f,    0.0010f,    0.0036f,    0.0036f,    0.0010f,    0.0001f,    0.0000f,
	0.0000f,    0.0010f,    0.0122f,    0.0420f,    0.0420f,    0.0122f,    0.0010f,    0.0000f,
	0.0001f,    0.0036f,    0.0420f,    0.1443f,    0.1443f,    0.0420f,    0.0036f,    0.0001f,
	0.0001f,    0.0036f,    0.0420f,    0.1443f,    0.1443f,    0.0420f,    0.0036f,    0.0001f,
	0.0000f,    0.0010f,    0.0122f,    0.0420f,    0.0420f,    0.0122f,    0.0010f,    0.0000f,
	0.0000f,    0.0001f,    0.0010f,    0.0036f,    0.0036f,    0.0010f,    0.0001f,    0.0000f,
	0.0000f,    0.0000f,    0.0000f,    0.0001f,    0.0001f,    0.0000f,    0.0000f,    0.0000f
};

void separateRGBA( float* pixels, float* rg, float* ba )
{
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			rg[i*w * 4 + j + 0] = pixels[i*w * 4 + j + 0];
			rg[i*w * 4 + j + 1] = 0.0f;
			rg[i*w * 4 + j + 2] = pixels[i*w * 4 + j + 1];
			rg[i*w * 4 + j + 3] = 0.0f;

			ba[i*w * 4 + j + 0] = pixels[i*w * 4 + j + 2];
			ba[i*w * 4 + j + 1] = 0.0f;
			ba[i*w * 4 + j + 2] = pixels[i*w * 4 + j + 3];
			ba[i*w * 4 + j + 3] = 0.0f;
		}
}

void combineRGBA( float* full, float* rg, float* ba )
{
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			full[i*w * 4 + j + 0] = rg[i * w * 4 + j + 0];
			full[i*w * 4 + j + 1] = rg[i * w * 4 + j + 2];
			full[i*w * 4 + j + 2] = ba[i * w * 4 + j + 0];
			full[i*w * 4 + j + 3] = ba[i * w * 4 + j + 2];
		}
}

void generateInputData()
{
	w = h = 8;
	p = new float[w * h * 4];
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			p[i*w * 4 + j + 0] = gauss8x8[i*w + j / 4];
			p[i*w * 4 + j + 1] = 1.0f;
			p[i*w * 4 + j + 2] = 1.0f;
			p[i*w * 4 + j + 3] = 1.0f;
		}
}

enum ImageChannel
{
	R = 4,
	G = 2,
	B = 1
};

void initTexture(unsigned t)
{
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, t);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

void initTextures( size_t tNumber, unsigned* t )
{
	for ( size_t i = 0; i < tNumber; i++ )
	{
		initTexture(t[i]);
	}
}

void setTextureData( unsigned w, unsigned h, float* pixels = nullptr, unsigned tex = 0 )
{

	if ( !pixels )
	{
		pixels = p;
	}

	if ( !tex )
	{
		tex = textures[0];
	}

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, tex );
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, pixels );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
}

class FFTTest: public ::testing::Test
{
protected:
	OPENCLFFT* clfft;
	GLFFT* glfft[2];
	GLFFT* glifft[2];

	virtual void SetUp() override
	{
		glGenTextures( 3, textures );
		generateInputData();
		initTextures( texNum, textures );
		setTextureData( w, h );

		OpenCLCore::Get();

		glfft[0] = new GLFFT( w, h, textures[1] );
		glfft[1] = new GLFFT( w, h, textures[2] );
		glifft[0] = new GLFFT( w, h, textures[1], true, true );
		glifft[1] = new GLFFT( w, h, textures[2], true, true );

		clfft = new OPENCLFFT( w, h, textures[0] );
	}

	virtual void TearDown() override
	{
		delete clfft;
		delete glfft[0];
		delete glfft[1];
		delete glifft[0];
		delete glifft[1];

		OpenCLCore::Destroy();
	}

	void do_glfft( float* full, float* rg, float* ba )
	{
		separateRGBA( full, rg, ba );

		setTextureData( w, h, rg, textures[1] );
		setTextureData( w, h, ba, textures[2] );

		glfft[0]->do_fft();
		EXPECT_EQ( textures[1], glfft[0]->get_fullTex() );
		glfft[1]->do_fft();
		EXPECT_EQ( textures[2], glfft[1]->get_fullTex() );
		float* res;
		res = new float[w * h * 4];
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[1] );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, res );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
		delete[] res;

	}

	void do_inverse_glfft( float* full, float* rg, float* ba )
	{
		glifft[0]->do_fft();
		EXPECT_EQ( textures[1], glfft[0]->get_fullTex() );
		glifft[1]->do_fft();
		EXPECT_EQ( textures[2], glfft[1]->get_fullTex() );

		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[1] );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, rg );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[2] );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, ba );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

		combineRGBA( full, rg, ba );
	}

	void printChannelName(ImageChannel channel)
	{
		if (channel & ImageChannel::R)
			std::cout << "r";
		if (channel & ImageChannel::G)
			std::cout << "g";
		if (channel & ImageChannel::B)
			std::cout << "b";
	}

	bool checkPixelChannel(float* c1, float* c2, unsigned i, unsigned j, ImageChannel channel, bool conj = false)
	{
		//std::cout << "Check at ";
		//printChannelName(channel);
		float c1im = (conj ? -1 : 1) * (*(c1 + 1));
		//std::cout << "(" << i << ", " << j << "): " << *c1 << (c1im >= 0.f ? "+" : "") << c1im << " ?= " << *c2 << (*(c2 + 1) >= 0.f ? "+" : "") << *(c2 + 1) << std::endl;
		if ( !floatIsEqual( *c1, *c2 ) || !floatIsEqual( c1im, *(c2 + 1) ) )
		{
			//std::cout << "At (" << i << ", " << j << ") values are not match in channel ";
			//printChannelName(channel);
			//std::cout << "! Diff: (" << *c1 - *c2 << ", " << c1im - *(c2+1) << ")" << std::endl;
			//std::cout << *c1 << "+" << *(c1 + 1) << " != " << *c2 << "+" << *(c2 + 1) << std::endl;
			return false;
		}
		return true;
	};

	bool checkclAndglFFTEqual(float** cldata, float** gldata, unsigned channel)
	{
		size_t cw = w * 2; // complex w

		for (unsigned i = 0; i < h; i++)
		{
			// step by 2 because it's complex
			for (unsigned j = 0; j <= cw / 2; j += 2)
			{
				if (channel & ImageChannel::R)
					if (!checkPixelChannel(&cldata[0][i*(w + 2) + j], &gldata[0][i*cw*2 + j * 2], i, j / 2, ImageChannel::R))
						return false;
				if (channel & ImageChannel::G)
					if (!checkPixelChannel(&cldata[1][i*(w + 2) + j], &gldata[0][i*cw*2 + j * 2 + 2], i, j / 2, ImageChannel::G))
						return false;
				if (channel & ImageChannel::B)
					if (!checkPixelChannel(&cldata[2][i*(w + 2) + j], &gldata[1][i*cw*2 + j * 2], i, j / 2, ImageChannel::B))
						return false;
			}
			// cl - complex conjugate
			int clj = cw / 2 - 2; // -2 because it's complex
			for (unsigned j = cw / 2 + 2; j < cw; j += 2)
			{
				if (channel & ImageChannel::R)
					if (!checkPixelChannel(&cldata[0][i*(w + 2) + clj], &gldata[0][i*cw * 2 + j * 2], i, j / 2, ImageChannel::R, true))
						;// return false;
				if (channel & ImageChannel::G)
					if (!checkPixelChannel(&cldata[1][i*(w + 2) + clj], &gldata[0][i*cw * 2 + j * 2 + 2], i, j / 2, ImageChannel::G, true))
						;// return false;
				if (channel & ImageChannel::B)
					if (!checkPixelChannel(&cldata[2][i*(w + 2) + clj], &gldata[1][i*cw * 2 + j * 2], i, j / 2, ImageChannel::B, true))
						;// return false;
				clj-=2;
			}
		}
		return true;
	}

	bool IsclAndglFFTEqual(unsigned channel = 7)
	{
		// opencl data
		cl_command_queue queue = OpenCLCore::Get()->queue;
		cl_mem channels[3];
		float* cldata[3];
		clfft->get_channels( channels[0], channels[1], channels[2] );
		for ( size_t i = 0; i < 3; i++ )
		{
			cldata[i] = new float[h * (w + 2)];
			clEnqueueReadBuffer( queue, channels[i], CL_TRUE, 0, (h * (w + 2)) * sizeof( float ), cldata[i], 0, NULL, NULL );
			//ImageHelper::printImg( cldata[i], w/2, h, "cldata", 1, true, 1 );
		}

		// opengl data
		float* gldata[2];
		for ( size_t i = 0; i < 2; i++ )
		{
			gldata[i] = new float[h * w * 4];
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[i+1] );
			glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, gldata[i] );
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
		}

		bool equal = checkclAndglFFTEqual(cldata, gldata, channel);

		for ( size_t i = 0; i < 3; i++ )
			delete[] cldata[i];
		for ( size_t i = 0; i < 2; i++ )
			delete[] gldata[i];

		return equal;
	}
};

TEST_F( FFTTest, SimpleGLFFT )
{
	float* img[2];
	img[0] = new float[w * h * 4];
	img[1] = new float[w * h * 4];

	do_glfft( p, img[0], img[1] );

	float* full = new float[w * h * 4];

	do_inverse_glfft( full, img[0], img[1] );
	glFinish();

	EXPECT_TRUE( mtxIsEqualPart( p, full, w, h, 3 ) );

	delete[] img[0];
	delete[] img[1];
	delete[] full;
}

TEST_F( FFTTest, SimpleCLFFT )
{
	clfft->do_fft();
	clfft->do_inverse_fft();
	OpenCLCore::Get()->finish();

	float* img = new float[w*h * 4];

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[0] );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	EXPECT_TRUE( mtxIsEqualPart( img, p, w, h, 3 ) );

	delete[] img;
}

TEST_F( FFTTest, CLFFTSequenceTest )
{
	float* t = new float[w * h * 4];
	for ( size_t i = 0; i < h; i++ )
	{
		for ( size_t j = 0; j < w * 4; j += 4 )
		{
			t[i*w * 4 + j + 0] = 1.0f;
			t[i*w * 4 + j + 1] = 0.0f;
			t[i*w * 4 + j + 2] = 0.0f;
			t[i*w * 4 + j + 3] = 1.0f;
		}
	}

	clfft->do_fft( FFTChannelMode::Multichrome );
	clfft->do_inverse_fft();
	OpenCLCore::Get()->finish();

	float* img = new float[w * h * 4];

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[0] );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );

	setTextureData( w, h, t );

	clfft->do_fft( FFTChannelMode::Multichrome );
	clfft->do_inverse_fft();
	OpenCLCore::Get()->finish();

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[0] );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	EXPECT_TRUE( mtxIsEqual( img, t, w, h ) );

	delete[] img;
}

TEST_F( FFTTest, clAndglFFT )
{
	clfft->do_fft( FFTChannelMode::Multichrome );
	OpenCLCore::Get()->finish();
	float* img[2];
	img[0] = new float[w * h * 4];
	img[1] = new float[w * h * 4];

	do_glfft( p, img[0], img[1] );
	glFinish();
	EXPECT_TRUE(IsclAndglFFTEqual(ImageChannel::R));
}

TEST_F(FFTTest, convolutionWithclFFT)
{
	// Kernel
	clfft->do_fft(FFTChannelMode::Monochrome);
	OpenCLCore::Get()->finish();

	float* t = new float[w * h * 4];
	for (size_t i = 0; i < h; i++)
	{
		for (size_t j = 0; j < w * 4; j += 4)
		{
			t[i*w * 4 + j + 0] = 0.0f;
			if(i >= 0 && i <= 1 && j <= 4 && j >= 0 )
				t[i*w * 4 + j + 0] = 1.0f;
			t[i*w * 4 + j + 1] = 0.0f;
			t[i*w * 4 + j + 2] = 0.0f;
			t[i*w * 4 + j + 3] = 1.0f;
		}
	}

	ImageHelper::printImgChannel(t, w, h, 4, 1, "Img for conv");

	unsigned imgTex;
	glGenTextures(1, &imgTex);
	initTexture(imgTex);
	setTextureData(w, h, t, imgTex);

	OPENCLFFT imgFFT{w, h, imgTex};
	imgFFT.do_fft(FFTChannelMode::Monochrome);

	glFinish();

	cl_mem filterr;
	cl_mem filterg;
	cl_mem filterb;
	cl_mem imager;
	cl_mem imageg;
	cl_mem imageb;

	clfft->get_channels(filterr, filterg, filterb);
	imgFFT.get_channels(imager, imageg, imageb);

	float* cldata = new float[h * (w + 2)];
	clEnqueueReadBuffer(OpenCLCore::Get()->queue, imager, CL_TRUE, 0, (h * (w + 2)) * sizeof(float), cldata, 0, NULL, NULL);
	ImageHelper::printImg( cldata, w/2, h, "Image ft", 1, true, 1 );

	clEnqueueReadBuffer(OpenCLCore::Get()->queue, filterr, CL_TRUE, 0, (h * (w + 2)) * sizeof(float), cldata, 0, NULL, NULL);
	ImageHelper::printImg(cldata, w / 2, h, "Filter ft", 1, true, 1);

	size_t size[1] = { (16 / 2) * 16 };
	OpenCLCore::Get()->MultiplyFFT(imager, filterr, size);

	clEnqueueReadBuffer(OpenCLCore::Get()->queue, imager, CL_TRUE, 0, (h * (w + 2)) * sizeof(float), cldata, 0, NULL, NULL);
	ImageHelper::printImg(cldata, w / 2, h, "Convolution ft", 1, true, 1);

	imgFFT.do_inverse_fft();

	clEnqueueReadBuffer(OpenCLCore::Get()->queue, imager, CL_TRUE, 0, (h * (w + 2)) * sizeof(float), cldata, 0, NULL, NULL);
	ImageHelper::printImg(cldata, w / 2, h, "Convolution", 1);

	delete[] cldata;
	
	//EXPECT_TRUE(IsclAndglFFTEqual(ImageChannel::R));
}

//TEST_F( FFTTest, SimpleglFFTTime )
//{
//	setTextureData( w, h );
//	GLFFT fft( w, h, textureId );
//	GLFFT ifft(w, h, textureId, true, true);
//	fft.set_input( [] () {} );
//
//
//	GLfloat* img;
//	img = new GLfloat[w * h * 4];
//	
//	fft.do_fft();
//	
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
//	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
//	ImageHelper::printImg( img, w, h );
//
//	ifft.do_fft();
//
//	
//
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
//	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
//
//	
//
//	std::chrono::duration<double> elapsedSeconds = std::chrono::duration<double>::zero();
//	auto fullStart = std::chrono::system_clock::now();
//	for ( unsigned i = 0; i < runNumber; i++ )
//	{
//		setTextureData( w, h );
//		
//		auto start = std::chrono::system_clock::now();
//		fft.do_fft();
//		ifft.do_fft();
//		glFinish();
//		auto end = std::chrono::system_clock::now();
//		
//		elapsedSeconds += end - start;
//		
//	}
//	auto fullEnd = std::chrono::system_clock::now();
//	std::chrono::duration<double> fullDuration = fullEnd - fullStart;
//	std::cout << runNumber << " FFT finished elapsed time: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
//	std::cout << runNumber << " FFT finished elapsed time: " << fullDuration.count() * 1000 << "ms." << std::endl;
//
//	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );
//
//	delete[] img;
//}
//
//TEST_F( FFTTest, SimpleclFFTTime )
//{
//	setTextureData( w, h );
//	OPENCLFFT fft( w, h, textureId );
//	fft.set_input( [] () {} );
//
//	fft.do_fft();
//	fft.do_inverse_fft();
//	GLfloat* img;
//	img = new GLfloat[w * h * 4];
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
//	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
//
//	std::chrono::duration<double> elapsedSeconds = std::chrono::duration<double>::zero();
//	auto fullStart = std::chrono::system_clock::now();
//	for ( unsigned i = 0; i < runNumber; i++ )
//	{
//		setTextureData( w, h );
//
//		auto start = std::chrono::system_clock::now();
//		fft.do_fft();
//		fft.do_inverse_fft();
//		fft.finish();
//		auto end = std::chrono::system_clock::now();
//
//		elapsedSeconds += end - start;
//	}
//
//	auto fullEnd = std::chrono::system_clock::now();
//	std::chrono::duration<double> fullDuration = fullEnd - fullStart;
//	std::cout << runNumber << " FFT finished elapsed time: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
//	std::cout << runNumber << " FFT finished elapsed time: " << fullDuration.count() * 1000 << "ms." << std::endl;
//
//	
//	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );
//
//	delete[] img;
//}

