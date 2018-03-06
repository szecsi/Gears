#include "TestGLCommon.h"

#include "fft/glFFT.cpp"
#include "fft/imageHelper.cpp"
#include "fft/OpenCLHelper.cpp"
#include "fft/openCLCore.cpp"
#include "fft/openCLFFT.cpp"
#include "fft/load_shaders.cpp"

const unsigned runNumber = 1;
unsigned textures[3];
float* p = nullptr;
unsigned w, h;

void separateRGBA(float* pixels, float* rg, float* ba)
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
	w = h = 1024;
	p = new float[w * h * 4];
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			p[i*w * 4 + j + 0] = 1.0f;
			p[i*w * 4 + j + 1] = 1.0f;
			p[i*w * 4 + j + 2] = 1.0f;
			p[i*w * 4 + j + 3] = 1.0f;
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
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
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
		glfft[1]->do_fft();
	}

	void do_inverse_glfft( float* full, float* rg, float* ba )
	{
		glifft[0]->do_fft();
		glifft[1]->do_fft();

		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[1] );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, rg );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[2] );
		glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, ba );
		glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

		combineRGBA( full, rg, ba );
	}

};

TEST_F( FFTTest, SimpleGLFFT )
{
	float* img[2];
	img[0] = new float[w * h * 4];
	img[1] = new float[w * h * 4];
	
	do_glfft(p, img[0], img[1]);

	float* full = new float[w * h * 4];

	do_inverse_glfft( full, img[0], img[1] );

	EXPECT_TRUE( mtxIsEqual( p, full, w, h ) );

	delete[] img[0];
	delete[] img[1];
	delete[] full;
}

TEST_F( FFTTest, SimpleCLFFT )
{
	clfft->do_fft();
	clfft->do_inverse_fft();

	float* img = new float[w*h * 4];

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textures[0] );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );

	delete[] img;
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

GTEST_API_ int main( int argc, char **argv ) {
	initGL();
	printf( "Running tests\n" );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}