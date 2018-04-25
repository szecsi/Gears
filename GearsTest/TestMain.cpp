#include "stdafx.h"
#include "TestGLCommon.h"

GTEST_API_ int main( int argc, char **argv ) {
	TestGLHelper helper;
	helper.initGL();
	printf( "Running tests\n" );
	testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}