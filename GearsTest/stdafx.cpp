// stdafx.cpp : source file that includes just the standard includes
// GearsTest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <algorithm>
#include <iostream>

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

bool mtxIsEqual( float* mtx1, float* mtx2, unsigned w, unsigned h )
{
	for(unsigned i = 0; i < h; i++)
		for ( unsigned j = 0; j < w*4; j++ )
		{
			unsigned index = i*w * 4 + j;
			if ( abs(mtx1[index] - mtx2[index]) > 0.000001 )
			{
				std::cout << abs( mtx1[index] - mtx2[index] ) << std::endl;;
				std::cout << mtx1[index] << " != " << mtx2[index] << std::endl;
				return false;
			}
		}
	return true;
}