#include "TestGLCommon.h"

#include "fft/glFFT.cpp"
#include "fft/imageHelper.cpp"
#include "fft/OpenCLHelper.cpp"
#include "fft/openCLCore.cpp"
#include "fft/openCLFFT.cpp"
#include "fft/load_shaders.cpp"

const unsigned runNumber = 1;
unsigned textureId;
float* p = nullptr;
unsigned w, h;

void generateInputData()
{
	w = h = 4;
	p = new float[w * h * 4];
	for ( unsigned i = 0; i < h; i++ )
		for ( unsigned j = 0; j < w * 4; j += 4 )
		{
			float a = (i % 2 == 0 || j % 8 == 0) ? 1.f : 0.f;
			p[i*w * 4 + j + 0] = 1.0f;
			p[i*w * 4 + j + 1] = 1.0f;
			p[i*w * 4 + j + 2] = 1.0f;
			p[i*w * 4 + j + 3] = 1.f;
		}
}

void setTextureData( unsigned w, unsigned h, float* pixels = nullptr )
{

	if ( !pixels )
	{
		pixels = p;
	}

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, pixels );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
}

class OpenCLFFTTest: public ::testing::Test 
{
protected:
	virtual void SetUp() override
	{
		OpenCLCore::Get();
		generateInputData();

		glGenTextures( 1, &textureId );
	}

	

	virtual void TearDown() override
	{
		OpenCLCore::Destroy();
	}

};

class OpenGLFFTTest: public ::testing::Test
{
protected:
	virtual void SetUp() override
	{
		generateInputData();

		glGenTextures( 1, &textureId );
	}

};

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


//TEST( FFTTest, SimpleglFFTMonochrome )
//{
//	setTextureData( w, h );
//	GLFFT fft( w, h, textureId );
//	GLFFT ifft( w, h, textureId, true, true );
//	fft.set_input( [] () {} );
//	GLfloat* img;
//	img = new GLfloat[w * h * 4];
//	fft.do_fft();
//
//	printImg( p, w, h );
//	printImg( img, w, h );
//
//	ifft.do_fft();
//
//	//std::cout << "FT finished elapsed time: " << elapsed_seconds.count() * 1000 / runNumber << "ms." << std::endl;
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
//	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
//	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
//	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );
//	printImg(p, w, h);
//	printImg(img, w, h);
//	delete[] img;
//	//printFFT( img, w, h );
//}

TEST_F( OpenGLFFTTest, SimpleglFFT )
{
	setTextureData( w, h );
	GLFFT fft( w, h, textureId );
	GLFFT ifft(w, h, textureId, true, true);
	fft.set_input( [] () {} );


	GLfloat* img;
	img = new GLfloat[w * h * 4];
	
	fft.do_fft();
	
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
	ImageHelper::printImg( img, w, h );

	ifft.do_fft();

	

	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	

	std::chrono::duration<double> elapsedSeconds = std::chrono::duration<double>::zero();
	auto fullStart = std::chrono::system_clock::now();
	for ( unsigned i = 0; i < runNumber; i++ )
	{
		setTextureData( w, h );
		
		auto start = std::chrono::system_clock::now();
		fft.do_fft();
		ifft.do_fft();
		glFinish();
		auto end = std::chrono::system_clock::now();
		
		elapsedSeconds += end - start;
		
	}
	auto fullEnd = std::chrono::system_clock::now();
	std::chrono::duration<double> fullDuration = fullEnd - fullStart;
	std::cout << runNumber << " FFT finished elapsed time: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << runNumber << " FFT finished elapsed time: " << fullDuration.count() * 1000 << "ms." << std::endl;

	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );

	delete[] img;
}

TEST_F( OpenCLFFTTest, SimpleclFFT )
{
	setTextureData( w, h );
	OPENCLFFT fft( w, h, textureId );
	fft.set_input( [] () {} );

	fft.do_fft();
	fft.do_inverse_fft();
	GLfloat* img;
	img = new GLfloat[w * h * 4];
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, img );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	std::chrono::duration<double> elapsedSeconds = std::chrono::duration<double>::zero();
	auto fullStart = std::chrono::system_clock::now();
	for ( unsigned i = 0; i < runNumber; i++ )
	{
		setTextureData( w, h );

		auto start = std::chrono::system_clock::now();
		fft.do_fft();
		fft.do_inverse_fft();
		fft.finish();
		auto end = std::chrono::system_clock::now();

		elapsedSeconds += end - start;
	}

	auto fullEnd = std::chrono::system_clock::now();
	std::chrono::duration<double> fullDuration = fullEnd - fullStart;
	std::cout << runNumber << " FFT finished elapsed time: " << elapsedSeconds.count() * 1000 << "ms." << std::endl;
	std::cout << runNumber << " FFT finished elapsed time: " << fullDuration.count() * 1000 << "ms." << std::endl;

	
	EXPECT_TRUE( mtxIsEqual( img, p, w, h ) );

	delete[] img;
}

GTEST_API_ int main( int argc, char **argv ) {
	initGL();
	printf( "Running tests\n" );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}