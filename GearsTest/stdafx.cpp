// stdafx.cpp : source file that includes just the standard includes
// GearsTest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <algorithm>
#include <iostream>

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

bool floatIsEqual(float n1, float n2)
{
	return abs(n1 - n2) <= 0.000001;
}

bool mtxIsEqual( float* mtx1, float* mtx2, unsigned w, unsigned h )
{
	return mtxIsEqualPart( mtx1, mtx2, w, h, 1 );
}

bool mtxIsEqualPart(float* mtx1, float* mtx2, unsigned w, unsigned h, unsigned miss)
{
	for (unsigned i = 0; i < h; i++)
		for (unsigned j = 0; j < w * 4; j+=(miss+1))
		{
			unsigned index = i * w * 4 + j;
			if ( !floatIsEqual(mtx1[index], mtx2[index]) )
			{
				std::cout << "At (" << i << ", " << j << "): ";
				std::cout << abs(mtx1[index] - mtx2[index]) << std::endl;;
				std::cout << mtx1[index] << " != " << mtx2[index] << std::endl;
				return false;
			}
		}
	return true;
}